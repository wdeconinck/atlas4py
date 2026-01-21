#pragma once

#include <string>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "atlas/util/DataType.h"

namespace atlas4py {
namespace py = ::pybind11;
std::string atlasToPybind( atlas::DataType const& dt ) {
    switch ( dt.kind() ) {
        case atlas::DataType::KIND_INT32:
            return py::format_descriptor<int32_t>::format();
        case atlas::DataType::KIND_INT64:
            return py::format_descriptor<int64_t>::format();
        case atlas::DataType::KIND_REAL32:
            return py::format_descriptor<float>::format();
        case atlas::DataType::KIND_REAL64:
            return py::format_descriptor<double>::format();
        case atlas::DataType::KIND_UINT64:
            return py::format_descriptor<uint64_t>::format();
        default:
            return "";
    }
}

atlas::DataType pybindToAtlas( py::dtype const& dtype ) {
    if ( dtype.is( py::dtype::of<int32_t>() ) )
        return atlas::DataType::KIND_INT32;
    else if ( dtype.is( py::dtype::of<int64_t>() ) )
        return atlas::DataType::KIND_INT64;
    else if ( dtype.is( py::dtype::of<float>() ) )
        return atlas::DataType::KIND_REAL32;
    else if ( dtype.is( py::dtype::of<double>() ) )
        return atlas::DataType::KIND_REAL64;
    else if ( dtype.is( py::dtype::of<uint64_t>() ) )
        return atlas::DataType::KIND_UINT64;
    else
        return { 0 };
}
}