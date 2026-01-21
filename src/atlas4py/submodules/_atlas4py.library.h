#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "atlas/library.h"

namespace atlas4py {
namespace py = ::pybind11;

namespace {

struct PySys {
    int argc;
    char** argv;
    static const PySys& instance() {
        static PySys _instance;
        return _instance;
    }
private:
    PySys() {
        py::module sys = py::module::import("sys");
        py::list sys_argv = sys.attr("argv");
        argc = (int)sys_argv.size();
        argv = (char**)malloc(argc * sizeof(char*));
        for (int i = 0; i < argc; ++i) {
            argv[i] = (char*)PyUnicode_AsUTF8(sys_argv[i].ptr());
        }
    }
};

}

void pybind_library_initialize() {
    atlas::initialise(PySys::instance().argc,PySys::instance().argv);
};
void pybind_library_finalize() {
    atlas::finalise();
};

void pybind_submodule_library(py::module_ &m) {
    auto m_library = m.def_submodule("library", "Library submodule");
    m_library.def("initialize", []() { pybind_library_initialize(); })
             .def("initialise", []() { pybind_library_initialize(); })
             .def("finalize",   []() { pybind_library_finalize(); })
             .def("finalise",   []() { pybind_library_finalize(); });
    m_library.attr("version") = atlas::Library::instance().version();
}

}  // namespace atlas4py
