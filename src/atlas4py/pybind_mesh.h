#pragma once

#include <pybind11/pybind11.h>
#include "atlas/mesh.h"
#include "atlas/mesh/actions/BuildDualMesh.h"
#include "atlas/mesh/actions/BuildEdges.h"
#include "atlas/mesh/actions/BuildNode2CellConnectivity.h"
#include "atlas/mesh/actions/BuildPeriodicBoundaries.h"
#include "atlas/mesh/actions/BuildHalo.h"
#include "atlas/mesh/actions/BuildParallelFields.h"

namespace atlas4py {
void pybind_mesh(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;

    py::class_<Mesh>( m, "Mesh" )
        .def( py::init( []( const Grid& grid ) { return Mesh(grid); } ) )
        .def( py::init( []( const Grid& grid, const grid::Partitioner& partitioner ) { return Mesh(grid,partitioner); } ) )
        .def_property_readonly( "grid", &Mesh::grid )
        .def_property_readonly( "projection", &Mesh::projection )
        .def_property( "nodes", py::overload_cast<>( &Mesh::nodes, py::const_ ), py::overload_cast<>( &Mesh::nodes ) )
        .def_property( "edges", py::overload_cast<>( &Mesh::edges, py::const_ ), py::overload_cast<>( &Mesh::edges ) )
        .def_property( "cells", py::overload_cast<>( &Mesh::cells, py::const_ ), py::overload_cast<>( &Mesh::cells ) )
        .def_property( "metadata", py::overload_cast<>( &Mesh::metadata, py::const_ ), py::overload_cast<>( &Mesh::metadata ) )
        .def_property_readonly( "part", &Mesh::part )
        .def_property_readonly( "nb_parts", &Mesh::nb_parts );
    m.def( "build_edges", []( Mesh& mesh, std::optional<std::reference_wrapper<eckit::Configuration>> const& config ) {
        if(config)
            mesh::actions::build_edges( mesh, config->get() );
        else
            mesh::actions::build_edges( mesh );
    }, "mesh"_a, "config"_a = std::nullopt );
    m.def( "build_node_to_edge_connectivity",
           py::overload_cast<Mesh&>( &mesh::actions::build_node_to_edge_connectivity ) );
    m.def( "build_element_to_edge_connectivity",
           py::overload_cast<Mesh&>( &mesh::actions::build_element_to_edge_connectivity ) );
    m.def( "build_node_to_cell_connectivity",
           py::overload_cast<Mesh&>( &mesh::actions:: build_node_to_cell_connectivity ) );
    m.def( "build_median_dual_mesh", py::overload_cast<Mesh&>( &mesh::actions::build_median_dual_mesh ) );
    m.def( "build_periodic_boundaries", py::overload_cast<Mesh&>( &mesh::actions::build_periodic_boundaries ) );
    m.def( "build_halo", py::overload_cast<Mesh&, int>( &mesh::actions::build_halo ) );
    m.def( "build_parallel_fields", py::overload_cast<Mesh&>( &mesh::actions::build_parallel_fields ) );

    py::class_<mesh::IrregularConnectivity>( m, "IrregularConnectivity" )
        .def( "__getitem__",
              []( mesh::IrregularConnectivity const& c, std::tuple<idx_t, idx_t> const& pos ) {
                  auto const& [row, col] = pos;
                  return c( row, col );
              } )
        .def_property_readonly( "rows", &mesh::IrregularConnectivity::rows )
        .def( "cols", &mesh::IrregularConnectivity::cols, "row_idx"_a )
        .def_property_readonly( "maxcols", &mesh::IrregularConnectivity::maxcols )
        .def_property_readonly( "mincols", &mesh::IrregularConnectivity::mincols );
    py::class_<mesh::MultiBlockConnectivity>( m, "MultiBlockConnectivity" )
        .def( "__getitem__",
              []( mesh::MultiBlockConnectivity const& c, std::tuple<idx_t, idx_t> const& pos ) {
                  auto const& [row, col] = pos;
                  return c( row, col );
              } )
        .def( "__getitem__",
              []( mesh::MultiBlockConnectivity const& c, std::tuple<idx_t, idx_t, idx_t> const& pos ) {
                  auto const& [block, row, col] = pos;
                  return c( block, row, col );
              } )
        .def_property_readonly( "blocks", &mesh::MultiBlockConnectivity::blocks )
        .def( "block", py::overload_cast<idx_t>( &mesh::MultiBlockConnectivity::block, py::const_ ), py::return_value_policy::reference_internal)
        .def_property_readonly( "rows", &mesh::MultiBlockConnectivity::rows )
        .def( "cols", &mesh::MultiBlockConnectivity::cols, "row_idx"_a )
        .def_property_readonly( "maxcols", &mesh::MultiBlockConnectivity::maxcols )
        .def_property_readonly( "mincols", &mesh::MultiBlockConnectivity::mincols );
    py::class_<mesh::BlockConnectivity>( m, "BlockConnectivity" )
        .def( "__getitem__",
              []( mesh::BlockConnectivity const& c, std::tuple<idx_t, idx_t> const& pos ) {
                  auto const& [row, col] = pos;
                  return c( row, col );
              } )
        .def_property_readonly( "rows", &mesh::BlockConnectivity::rows )
        .def_property_readonly( "cols", &mesh::BlockConnectivity::cols );

    py::class_<mesh::Nodes>( m, "Nodes" )
        .def_property_readonly( "size", &mesh::Nodes::size )
        .def_property_readonly( "edge_connectivity",
                                py::overload_cast<>( &mesh::Nodes::edge_connectivity, py::const_ ) )
        .def_property_readonly( "cell_connectivity",
                                py::overload_cast<>( &mesh::Nodes::cell_connectivity, py::const_ ) )
        .def_property_readonly( "lonlat", py::overload_cast<>( &Mesh::Nodes::lonlat, py::const_ ) )
        .def_property_readonly( "xy", py::overload_cast<>( &Mesh::Nodes::xy, py::const_ ) )
        .def("field", []( mesh::Nodes const& n, std::string const& name ) { return n.field( name ); },
             "name"_a, py::return_value_policy::reference_internal )
        .def( "flags", []( mesh::Nodes const& n ) { return n.flags(); },
              py::return_value_policy::reference_internal);

    py::class_<mesh::HybridElements>( m, "HybridElements" )
        .def_property_readonly( "size", &mesh::HybridElements::size )
        .def( "nb_nodes", &mesh::HybridElements::nb_nodes )
        .def( "nb_edges", &mesh::HybridElements::nb_edges )
        .def_property_readonly( "node_connectivity",
                                py::overload_cast<>( &mesh::HybridElements::node_connectivity, py::const_ ) )
        .def_property_readonly( "edge_connectivity",
                                py::overload_cast<>( &mesh::HybridElements::edge_connectivity, py::const_ ) )
        .def_property_readonly( "cell_connectivity",
                                py::overload_cast<>( &mesh::HybridElements::cell_connectivity, py::const_ ) )

        .def("field", []( mesh::HybridElements const& he, std::string const& name ) { return he.field( name ); },
            "name"_a, py::return_value_policy::reference_internal )
        .def( "flags", []( mesh::HybridElements const& he ) { return he.flags(); },
              py::return_value_policy::reference_internal );


    py::class_<mesh::Nodes::Topology> topology( m, "Topology" );
    topology.attr( "NONE" )     = py::cast( int( mesh::Nodes::Topology::NONE ) );
    topology.attr( "GHOST" )    = py::cast( int( mesh::Nodes::Topology::GHOST ) );
    topology.attr( "PERIODIC" ) = py::cast( int( mesh::Nodes::Topology::PERIODIC ) );
    topology.attr( "BC" )       = py::cast( int( mesh::Nodes::Topology::BC ) );
    topology.attr( "WEST" )     = py::cast( int( mesh::Nodes::Topology::WEST ) );
    topology.attr( "EAST" )     = py::cast( int( mesh::Nodes::Topology::EAST ) );
    topology.attr( "NORTH" )    = py::cast( int( mesh::Nodes::Topology::NORTH ) );
    topology.attr( "SOUTH" )    = py::cast( int( mesh::Nodes::Topology::SOUTH ) );
    topology.attr( "PATCH" )    = py::cast( int( mesh::Nodes::Topology::PATCH ) );
    topology.attr( "POLE" )     = py::cast( int( mesh::Nodes::Topology::POLE ) );
    topology.def_static( "reset", &mesh::Nodes::Topology::reset );
    topology.def_static( "set", &mesh::Nodes::Topology::set );
    topology.def_static( "unset", &mesh::Nodes::Topology::unset );
    topology.def_static( "toggle", &mesh::Nodes::Topology::toggle );
    topology.def_static( "check", &mesh::Nodes::Topology::check );
    topology.def_static( "check_all", &mesh::Nodes::Topology::check_all );
    topology.def_static( "check_any", &mesh::Nodes::Topology::check_any );

}
}