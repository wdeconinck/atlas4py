#pragma once

#include <memory>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dlpack/dlpack.h>
#include "atlas/array.h"

namespace pybind11 {
namespace detail {
template <>
struct type_caster<atlas::array::ArrayStrides>
    : public type_caster<std::vector<atlas::array::ArrayStrides::value_type>> {};
template <>
struct type_caster<atlas::array::ArrayShape> : public type_caster<std::vector<atlas::array::ArrayShape::value_type>> {};

}  // namespace detail
}  // namespace pybind11

namespace atlas4py {
namespace py = ::pybind11;
namespace {
template <typename Value>
std::unique_ptr<atlas::array::Array> create_array(void* data, const atlas::array::ArraySpec& spec) {
    return std::unique_ptr<atlas::array::Array>(atlas::array::Array::wrap<Value>(static_cast<Value*>(data), spec));
}

DLDataType get_dlpack_dtype(atlas::array::DataType const& datatype) {
    DLDataType dl_dtype;
    dl_dtype.lanes = 1;
    dl_dtype.bits  = 8 * datatype.size();
    switch (datatype.kind()) {
        case atlas::array::DataType::KIND_INT32:
        case atlas::array::DataType::KIND_INT64:
            dl_dtype.code = kDLInt;
            break;
        case atlas::array::DataType::KIND_REAL32:
        case atlas::array::DataType::KIND_REAL64:
            dl_dtype.code = kDLFloat;
            break;
        default:
            throw std::runtime_error("Unsupported data type for DLPack conversion");
    }
    return dl_dtype;
}

void dl_tensor_deleter(DLManagedTensor* self) {
  delete[] self->dl_tensor.shape;
  delete[] self->dl_tensor.strides;
  // Invalidate tensor
  self->dl_tensor.data = nullptr;
  self->dl_tensor.ndim = 0;
  self->dl_tensor.shape = nullptr;
  self->dl_tensor.strides = nullptr;
  self->dl_tensor.byte_offset = 0;
}

void dl_capsule_deleter(PyObject *capsule) {
  void *raw_ptr = nullptr;
  // Can be original name if unused "dltensor"
  if (strcmp("dltensor", PyCapsule_GetName(capsule)) == 0) {
    raw_ptr = PyCapsule_GetPointer(capsule, "dltensor");
  }
  else { // "used_dltensor if capsule is consumed
    raw_ptr = PyCapsule_GetPointer(capsule, "used_dltensor");
  }

  if (raw_ptr) { // Else, unknown capsule or already freed capsule
    DLManagedTensor* tensor_ptr = static_cast<DLManagedTensor*>(raw_ptr);
    if (tensor_ptr->deleter) // Execute custom deleter, here delete[] shape, strides
      tensor_ptr->deleter(tensor_ptr);
  }
}

}

std::unique_ptr<atlas::array::Array> from_dlpack( const py::object& object, const py::kwargs& kwargs ) {
    py::object dlpack_capsule = object.attr("__dlpack__")(**kwargs);
    auto* tensor = reinterpret_cast<DLManagedTensor*>(PyCapsule_GetPointer(dlpack_capsule.ptr(), "dltensor"));
    if (tensor) {
        DLTensor& dl_tensor = tensor->dl_tensor;
        atlas::array::ArrayShape shape;
        atlas::array::ArrayStrides strides;
        shape.resize(dl_tensor.ndim);
        strides.resize(dl_tensor.ndim);
        for (int i = 0; i < dl_tensor.ndim; ++i) {
            shape[i] = dl_tensor.shape[i];
            strides[i] = dl_tensor.strides[i];
        }
        auto spec = atlas::array::ArraySpec(std::move(shape), std::move(strides));
        switch (dl_tensor.dtype.code) {
            case kDLInt:
                switch (dl_tensor.dtype.bits) {
                    case 32: return create_array<int>(dl_tensor.data, spec);
                    case 64: return create_array<long>(dl_tensor.data, spec);
                    default: throw std::runtime_error("Unsupported integer bit-width for DLPack conversion");
                }
            case kDLFloat:
                switch (dl_tensor.dtype.bits) {
                    case 32: return create_array<float>(dl_tensor.data, spec);
                    case 64: return create_array<double>(dl_tensor.data, spec);
                    default: throw std::runtime_error("Unsupported float bit-width for DLPack conversion");
                }
            default:
                throw std::runtime_error("Unsupported data type for DLPack conversion");
        }
    }
    throw std::runtime_error("Invalid DLPack capsule");
}

py::capsule to_dlpack(atlas::array::Array& array) {
    // Note this is hardcoding the CPU device for now
    DLTensor dl_tensor;
    dl_tensor.data = array.data();
    dl_tensor.device.device_type = DLDeviceType::kDLCPU;
    dl_tensor.device.device_id = 0;
    dl_tensor.dtype = get_dlpack_dtype(array.datatype());
    dl_tensor.ndim = array.rank();
    auto shape_ptr = std::make_unique<int64_t[]>(dl_tensor.ndim);
    dl_tensor.shape = shape_ptr.get();
    std::copy(array.shape().begin(), array.shape().end(), dl_tensor.shape); // Init with correct shape
    auto strides_ptr = std::make_unique<int64_t[]>(dl_tensor.ndim);
    dl_tensor.strides = strides_ptr.get();
    std::copy(array.strides().begin(), array.strides().end(), dl_tensor.strides); // Init with correct shape
    dl_tensor.byte_offset = 0;

    auto tensor = std::make_unique<DLManagedTensor>();
    tensor->dl_tensor = dl_tensor;
    tensor->manager_ctx = &array;
    tensor->deleter = &dl_tensor_deleter;

    // Release unique pointer to capsule, as we transfer ownership to the
    // python capsule.
    shape_ptr.release();
    strides_ptr.release();
    return py::capsule(tensor.release(), "dltensor", &dl_capsule_deleter);
}

std::pair<int32_t, int64_t> to_dlpack_device(atlas::array::Array& array) {
    return std::pair<int32_t, int64_t>{kDLCPU, 0};
    // Device type codes are defined in dlpack/dlpack.h
    // CPU = 1
    // CUDA = 2
    // CPU_PINNED = 3
    // OPENCL = 4
    // VULKAN = 7
    // METAL = 8
    // VPI = 9
    // ROCM = 10
    // CUDA_MANAGED = 13
    // ONE_API = 14
}

py::buffer_info to_buffer_info(atlas::array::Array& f) {
    auto strides = f.strides();
    auto sizeof_dtype = f.datatype().size();
    std::transform( strides.begin(), strides.end(), strides.begin(),
                    [&]( auto const& stride ) { return stride * sizeof_dtype; } );
    return py::buffer_info( f.storage(), sizeof_dtype, atlasToPybind( f.datatype() ), f.rank(),
                            f.shape(), strides );
}

} // namespace atlas4py
