#pragma once

#include <pybind11/pybind11.h>
#include "atlas/grid.h"
namespace atlas4py {
namespace py = ::pybind11;
void pybind_partitioner(py::module_ &m) {
    using namespace atlas;

    py::class_<grid::Partitioner>( m, "Partitioner" )
        .def( py::init( []( py::kwargs kwargs ) { return grid::Partitioner( to_config(kwargs)); } ) )
        .def( py::init( []( util::Config const& config ) { return grid::Partitioner( config ); } ) )
        .def( py::init( []( const std::string& type ) { return grid::Partitioner(type); } ) ) 
        .def( py::init( [](){ return grid::Partitioner(); } ) );

    py::class_<grid::MatchingPartitioner,grid::Partitioner>( m, "MatchingPartitioner" )
        .def( py::init( []( Mesh const& mesh, py::kwargs kwargs ) { return grid::MatchingPartitioner(mesh, to_config(kwargs)); } ) )
        .def( py::init( []( FunctionSpace const& functionspace, py::kwargs kwargs ) { return grid::MatchingPartitioner(functionspace, to_config(kwargs)); } ) );

}
}  // namespace atlas4py
