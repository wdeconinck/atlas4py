#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "atlas/functionspace.h"

#include "pybind_config.h"
#include "pybind_datatype.h"
#include "pybind_array.h"

namespace atlas4py {
namespace py = ::pybind11;

void pybind_submodule_functionspace(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;
    auto m_fs = m.def_submodule("functionspace", "Functionspace submodule");
    py::class_<FunctionSpace>( m_fs, "FunctionSpace" )
        .def_property_readonly( "size", &FunctionSpace::size )
        .def_property_readonly( "type", &FunctionSpace::type )
        .def(
            "create_field",
            []( FunctionSpace const& fs, std::optional<py::object> dtype, py::kwargs kwargs ) {
                util::Config config = to_config(kwargs);
                if ( dtype )
                    config.set( option::datatype( pybindToAtlas( py::dtype::from_args( *dtype ) ) ));
                else
                    config.set( option::datatypeT<double>() );
                return fs.createField( config );
            },
            "dtype"_a = std::nullopt)
        .def(
            "create_field_global",
            []( FunctionSpace const& fs, std::optional<py::object> dtype, py::kwargs kwargs ) {
                util::Config config = to_config(kwargs);
                config.set("global",true);
                if ( dtype )
                    config.set( option::datatype( pybindToAtlas( py::dtype::from_args( *dtype ) ) ));
                else
                    config.set( option::datatypeT<double>() );
                return fs.createField( config );
            },
            "dtype"_a = std::nullopt)
        .def_property_readonly("lonlat", &FunctionSpace::lonlat )
        .def_property_readonly("ghost", &FunctionSpace::ghost )
        .def_property_readonly("remote_index", &FunctionSpace::remote_index )
        .def_property_readonly("partition", &FunctionSpace::partition )
        .def_property_readonly("global_index", &FunctionSpace::global_index )
        .def_property_readonly("part", &FunctionSpace::part )
        .def_property_readonly("nb_parts", &FunctionSpace::nb_parts )
        .def("gather", [](FunctionSpace const& fs, Field const& local, Field& global) {
                return fs.gather(local,global);
            })
        .def("scatter", [](FunctionSpace const& fs, Field const& global, Field& local) {
                return fs.scatter(global,local);
            })
        .def("halo_exchange", [](FunctionSpace const& fs, Field& field) {
                return fs.haloExchange(field);
            })
        .def("halo_exchange", [](FunctionSpace const& fs, py::object array, py::kwargs kwargs) {
                kwargs["copy"] = false;
                Field field("tmp", from_dlpack(array, kwargs).release());
                return fs.haloExchange(field);
            });

    py::class_<functionspace::EdgeColumns, FunctionSpace>( m_fs, "EdgeColumns" )
        .def( py::init( []( Mesh const& m, int halo, int levels ) {
                  return functionspace::EdgeColumns( m, util::Config()( "halo", halo )("levels", levels) );
              } ),
              "mesh"_a, "halo"_a = 0, "levels"_a = 0 )
        .def_property_readonly( "nb_edges", &functionspace::EdgeColumns::nb_edges )
        .def_property_readonly( "mesh", &functionspace::EdgeColumns::mesh )
        .def_property_readonly( "edges", &functionspace::EdgeColumns::edges )
        .def_property_readonly( "valid", &functionspace::EdgeColumns::valid );
    py::class_<functionspace::NodeColumns, FunctionSpace>( m_fs, "NodeColumns" )
        .def( py::init( []( FunctionSpace fs ) { return functionspace::NodeColumns{fs}; } ) )
        .def( py::init( []( Mesh const& m, int halo, int levels ) {
                  return functionspace::NodeColumns( m, util::Config()( "halo", halo )("levels",levels) );
              } ),
              "mesh"_a, "halo"_a = 0, "levels"_a = 0 )
        .def_property_readonly( "nb_nodes", &functionspace::NodeColumns::nb_nodes )
        .def_property_readonly( "mesh", &functionspace::NodeColumns::mesh )
        .def_property_readonly( "nodes", &functionspace::NodeColumns::nodes )
        .def_property_readonly( "valid", &functionspace::NodeColumns::valid )
        .def( "__bool__", [](const functionspace::NodeColumns& self) { return self.valid(); } );

    py::class_<functionspace::CellColumns, FunctionSpace>( m_fs, "CellColumns" )
        .def( py::init( []( Mesh const& m, int halo, int levels ) {
                  return functionspace::CellColumns( m, util::Config()( "halo", halo )("levels",levels) );
              } ),
              "mesh"_a, "halo"_a = 0, "levels"_a = 0 )
        .def_property_readonly( "nb_cells", &functionspace::CellColumns::nb_cells )
        .def_property_readonly( "mesh", &functionspace::CellColumns::mesh )
        .def_property_readonly( "cells", &functionspace::CellColumns::cells )
        .def_property_readonly( "valid", &functionspace::CellColumns::valid );

    py::class_<functionspace::Spectral, FunctionSpace>( m_fs, "Spectral" )
        .def( py::init( []( FunctionSpace fs ) { return functionspace::Spectral{fs}; } ) )
        .def( py::init( []( int truncation ) { return functionspace::Spectral( truncation ); } ), "truncation"_a )
        .def_property_readonly( "nb_spectral_coefficients", &functionspace::Spectral::nb_spectral_coefficients )
        .def_property_readonly( "nb_spectral_coefficients_global", &functionspace::Spectral::nb_spectral_coefficients_global )
        .def_property_readonly( "truncation", &functionspace::Spectral::truncation )
        .def_property_readonly( "valid", &functionspace::Spectral::valid )
        .def( "__bool__", [](const functionspace::Spectral& self) { return self.valid(); } )
        .def("parallel_for", []( const functionspace::Spectral& self, const py::function &f) {
            self.parallel_for<std::function<void(idx_t,idx_t,int,int)>>(f);});


    py::class_<functionspace::StructuredColumns, FunctionSpace>( m_fs, "StructuredColumns" )
        .def( py::init( []( Grid const& g, grid::Partitioner const& p, py::kwargs kwargs ) {
                  return functionspace::StructuredColumns( g, p, to_config(kwargs) ); } ),
              "grid"_a, "partitioner"_a )
        .def( py::init( []( Grid const& g, py::kwargs kwargs ) { return functionspace::StructuredColumns( g, to_config(kwargs) ); } ), "grid"_a )
        .def_property_readonly( "grid", &functionspace::StructuredColumns::grid )
        .def_property_readonly( "valid", &functionspace::StructuredColumns::valid );

    py::class_<functionspace::PointCloud, FunctionSpace>( m_fs, "PointCloud" )
        .def( py::init( []( FunctionSpace fs ) { return functionspace::PointCloud{fs}; } ) )
        .def( py::init( []( Grid const& grid, py::kwargs kwargs) {
                  return functionspace::PointCloud( grid, to_config(kwargs) );
              } ),
              "grid"_a )
        .def( py::init( []( Grid const& grid, atlas::grid::Partitioner const& partitioner, py::kwargs kwargs) {
                  return functionspace::PointCloud( grid, partitioner, to_config(kwargs) );
              } ),
              "grid"_a, "partitioner"_a)
        .def_property_readonly( "valid", &functionspace::PointCloud::valid )
        .def( "__bool__", [](const functionspace::PointCloud& self) { return self.valid(); } );
}
} // namespace atlas4py