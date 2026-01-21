// #include <functional>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "submodules/_atlas4py.library.h"
#include "submodules/_atlas4py.dlpack.h"
#include "submodules/_atlas4py.function.h"
#include "submodules/_atlas4py.functionspace.h"
#include "pybind_config.h"
#include "pybind_datatype.h"
#include "pybind_point.h"
#include "pybind_array.h"
#include "pybind_mesh.h"
#include "pybind_domain.h"
#include "pybind_projection.h"
#include "pybind_grid.h"
#include "pybind_partitioner.h"
#include "pybind_kdtree.h"
#include "pybind_field.h"
#include "pybind_meshgenerator.h"
#include "pybind_metadata.h"
#include "pybind_gmsh.h"
#include "pybind_interpolation.h"
#include "pybind_trans.h"

PYBIND11_MODULE( _atlas4py, m ) {
    m.def("initialize", []() { atlas4py::pybind_library_initialize(); })
     .def("initialise", []() { atlas4py::pybind_library_initialize(); })
     .def("finalize",   []() { atlas4py::pybind_library_finalize(); })
     .def("finalise",   []() { atlas4py::pybind_library_finalize(); });
    atlas4py::pybind_submodule_library(m);
    atlas4py::pybind_submodule_dlpack(m);
    atlas4py::pybind_submodule_function(m);
    atlas4py::pybind_submodule_functionspace(m);
    atlas4py::pybind_config(m);
    atlas4py::pybind_point(m);
    atlas4py::pybind_kdtree(m);
    atlas4py::pybind_grid(m);
    atlas4py::pybind_partitioner(m);
    atlas4py::pybind_projection(m);
    atlas4py::pybind_mesh(m);
    atlas4py::pybind_domain(m);
    atlas4py::pybind_field(m);
    atlas4py::pybind_meshgenerator(m);
    atlas4py::pybind_metadata(m);
    atlas4py::pybind_gmsh(m);
    atlas4py::pybind_interpolation(m);
    atlas4py::pybind_trans(m);
}
