#pragma once

#include <pybind11/pybind11.h>

#include "atlas/interpolation.h"

namespace atlas4py {
namespace py = ::pybind11;

void pybind_interpolation(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;

    py::class_<Interpolation>( m, "Interpolation" )
        .def( py::init( [](const std::string& type, const FunctionSpace& source, const FunctionSpace& target, py::kwargs kwargs){
                auto config = to_config(kwargs);
                config.set("type",type);
                return Interpolation(config,source,target);
            } ), "type"_a, "source"_a, "target"_a )
        .def( py::init( [](const std::string& type, const Grid& source, const Grid& target, py::kwargs kwargs){
                auto config = to_config(kwargs);
                config.set("type",type);
                return Interpolation(config,source,target);
            } ), "type"_a, "source"_a, "target"_a )
        .def( "execute", []( Interpolation const& self, const Field& source, Field& target) { return self.execute(source,target);} )
        .def_property_readonly( "source", &Interpolation::source )
        .def_property_readonly( "target", &Interpolation::target );
}
}
