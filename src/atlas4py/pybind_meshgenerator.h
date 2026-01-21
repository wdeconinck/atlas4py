#pragma once

#include <pybind11/pybind11.h>
#include "atlas/meshgenerator.h"

namespace atlas4py {
namespace py = ::pybind11;
void pybind_meshgenerator(py::module_ &m) {
    using namespace atlas;

    py::class_<MeshGenerator>( m, "MeshGenerator" )
        .def( py::init( []( py::kwargs kwargs ) { return MeshGenerator( to_config(kwargs)); } ) )
        .def( py::init( []( util::Config const& config ) { return MeshGenerator( config ); } ) )
        .def( py::init( []( const std::string& type ) { return MeshGenerator(type); } ) ) 
        .def( py::init( [](){ return MeshGenerator(); } ) )
        .def( "generate", [](MeshGenerator const& self, Grid const& grid){ return self.generate(grid); } )
        .def( "generate", [](MeshGenerator const& self, Grid const& grid, grid::Partitioner const& partitioner ){ return self.generate(grid, partitioner); } );

    py::class_<StructuredMeshGenerator>( m, "StructuredMeshGenerator" )
        // TODO in FunctionSpace below we expose config options, not the whole config object
        .def( py::init( []( util::Config const& config ) { return StructuredMeshGenerator( config ); } ) )
        .def( py::init() )
        .def( "generate", py::overload_cast<Grid const&>( &StructuredMeshGenerator::generate, py::const_ ) );
}
}  // namespace atlas4py