#pragma once

#include <pybind11/pybind11.h>
#include "atlas/projection.h"

namespace atlas4py {
namespace py = ::pybind11;
void pybind_projection(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;

    py::class_<Projection>( m, "Projection" ).def( "__repr__", []( Projection const& p ) {
        return "_atlas4py.Projection("_s + py::str( toPyObject( p.spec() ) ) + ")"_s;
    } );
}
}  // namespace atlas4py