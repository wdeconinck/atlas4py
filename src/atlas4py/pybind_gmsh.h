#pragma once

#include <string>
#include <pybind11/pybind11.h>

#include "atlas/output/Gmsh.h"

namespace atlas4py {
namespace py = ::pybind11;

void pybind_gmsh(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;

    py::class_<output::Gmsh>( m, "Gmsh" )
        .def( py::init( []( std::string const& path ) { return output::Gmsh{ path }; } ), "path"_a )
        .def( py::init( []( std::string const& path, eckit::Configuration const& config, py::kwargs kwargs ) {
            util::Config cfg = util::Config(config);
            cfg.set(to_config(kwargs));
            return output::Gmsh(path,cfg);
          }), "path"_a, "config"_a )
        .def( py::init( []( std::string const& path, py::kwargs kwargs ) {return output::Gmsh{ path, to_config(kwargs) }; } ), "path"_a )
        .def( "__enter__", []( output::Gmsh& gmsh ) { return gmsh; } )
        .def( "__exit__", []( output::Gmsh& gmsh, py::object exc_type, py::object exc_val,
                              py::object exc_tb ) { gmsh.reset( nullptr ); } )
        .def(
            "write", []( output::Gmsh& gmsh, Mesh const& mesh ) { gmsh.write( mesh ); return gmsh; }, "mesh"_a )
        .def(
            "write", []( output::Gmsh& gmsh, Field const& field ) { gmsh.write( field );  return gmsh;}, "field"_a )
        .def(
            "write", []( output::Gmsh& gmsh, Field const& field, FunctionSpace const& fs ) { gmsh.write( field, fs );  return gmsh; },
            "field"_a, "functionspace"_a );
}
}