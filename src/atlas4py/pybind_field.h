#pragma once

#include <pybind11/pybind11.h>

#include "atlas/field.h"

#include "pybind_array.h"

namespace atlas4py {
namespace py = ::pybind11;
void pybind_field(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;

    py::class_<Field>( m, "Field", py::buffer_protocol() )
        .def_static( "from_dlpack", []( py::object dlpack_compatible_array, py::kwargs kwargs ) {
            return Field("field", from_dlpack(dlpack_compatible_array, kwargs).release() );
        }, "dlpack_tensor"_a)
        .def_property_readonly( "name", &Field::name )
        .def_property_readonly( "strides", &Field::strides )
        .def_property_readonly( "shape", py::overload_cast<>( &Field::shape, py::const_ ) )
        .def_property_readonly( "size", &Field::size )
        .def_property_readonly( "rank", &Field::rank )
        .def_property_readonly( "levels", &Field::levels )
        .def_property_readonly( "datatype", []( Field& f ) { return atlasToPybind( f.datatype() ); } )
        .def_property( "metadata", py::overload_cast<>( &Field::metadata, py::const_ ),
                       py::overload_cast<>( &Field::metadata ) )
        .def_property_readonly( "functionspace", py::overload_cast<>( &Field::functionspace, py::const_ ) )
        .def( "halo_exchange", []( Field& f) { f.haloExchange(); })
        .def_property( "halo_dirty", &Field::dirty, &Field::set_dirty, py::return_value_policy::copy)
        .def_buffer( []( Field& f ) {
            return to_buffer_info(f);
        })
        .def( "__dlpack__", []( Field& f, const py::object& stream) {
            return to_dlpack(f);
        }, py::arg("stream") = py::none() )
        .def( "__dlpack_device__", []( Field&f ) {
            return to_dlpack_device(f);
        }
    );
}
}