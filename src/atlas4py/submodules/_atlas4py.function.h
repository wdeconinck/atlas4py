#pragma once

#include <pybind11/pybind11.h>

#include "atlas/util/function/VortexRollup.h"
#include "atlas/util/function/SphericalHarmonic.h"

namespace atlas4py {
namespace py = ::pybind11;
void pybind_submodule_function(py::module_ &m) {
    using namespace pybind11::literals;
    auto m_function = m.def_submodule("function", "Function submodule");
    m_function.def("vortex_rollup", [](double lon, double lat, double t) { return atlas::util::function::vortex_rollup(lon,lat,t); } );
    m_function.def("spherical_harmonic", [](double lon, double lat,int n, int m ) { return atlas::util::function::spherical_harmonic(n,m,lon,lat); }, "lon"_a, "lat"_a, "n"_a, "m"_a );
}
}
