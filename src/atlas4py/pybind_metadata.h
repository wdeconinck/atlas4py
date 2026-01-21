#pragma once

#include <string>
#include <pybind11/pybind11.h>

#include "atlas/util/Metadata.h"

namespace atlas4py {
namespace py = ::pybind11;

void pybind_metadata(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;

    py::class_<util::Metadata, eckit::LocalConfiguration>( m, "Metadata" )
        .def_property_readonly( "keys", &util::Metadata::keys )
        .def( "__repr__", []( util::Metadata const& metadata ) {
            return "_atlas4py.Metadata("_s + py::str( toPyObject( metadata ) ) + ")"_s;
        } );
}
}  // namespace atlas4py
