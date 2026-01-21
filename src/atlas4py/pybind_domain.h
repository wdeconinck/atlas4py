#pragma once

#include <pybind11/pybind11.h>

#include "atlas/domain.h"

namespace atlas4py {
namespace py = ::pybind11;
void pybind_domain(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;
    py::class_<Domain>( m, "Domain" )
        .def_property_readonly( "type", &Domain::type )
        .def_property_readonly( "global", &Domain::global )
        .def_property_readonly( "units", &Domain::units )
        .def( "__repr__", []( Domain const& d ) {
            return "_atlas4py.Domain("_s + ( d ? py::str( toPyObject( d.spec() ) ) : "" ) + ")"_s;
        } );
    py::class_<RectangularDomain, Domain>( m, "RectangularDomain" )
        .def( py::init( []( std::tuple<double, double> xInterval, std::tuple<double, double> yInterval ) {
                  auto [xFrom, xTo] = xInterval;
                  auto [yFrom, yTo] = yInterval;
                  return RectangularDomain( { xFrom, xTo }, { yFrom, yTo } );
              } ),
              "x_interval"_a, "y_interval"_a );
}
}