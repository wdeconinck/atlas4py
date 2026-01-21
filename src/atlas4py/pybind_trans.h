#pragma once

#include <pybind11/pybind11.h>
#include "atlas/trans/Trans.h"

namespace atlas4py {

void pybind_trans(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;

    py::class_<trans::Trans>( m, "Trans" )
        .def( py::init( [](const FunctionSpace& gp, const FunctionSpace& sp, py::kwargs kwargs){ return trans::Trans(gp,sp,to_config(kwargs)); } ), "gp"_a, "sp"_a )
        .def( py::init( [](const Grid& grid, int truncation, py::kwargs kwargs){ return trans::Trans(grid,truncation,to_config(kwargs));} ), "grid"_a, "truncation"_a )
        .def( "dirtrans", []( trans::Trans& trans, const Field& gpfield, Field& spfield) { trans.dirtrans(gpfield,spfield);} )
        .def( "invtrans", []( trans::Trans& trans, const Field& spfield, Field& gpfield) { trans.invtrans(spfield,gpfield);} )
        .def_property_readonly( "truncation", &trans::Trans::truncation )
        .def_property_readonly( "nb_spectral_coefficients", &trans::Trans::spectralCoefficients )
        .def_static("backend", [] ( const std::string& backend ){ trans::Trans::backend(backend); } )
        .def_static("has_backend", [] ( const std::string& backend ){ return trans::Trans::hasBackend(backend); } );
}

}  // namespace atlas4py
