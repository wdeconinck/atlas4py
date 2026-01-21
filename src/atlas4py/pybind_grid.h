#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include "atlas/grid.h"

namespace atlas4py {
namespace py = ::pybind11;
void pybind_grid(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;
    py::class_<Grid>( m, "Grid" )
        .def( py::init<const std::string&>(), "name"_a )
        .def( py::init( []( const std::string& name, const Domain& domain ) { return Grid(name,domain); } ), "name"_a, "domain"_a )
        .def_property_readonly( "name", &Grid::name )
        .def_property_readonly( "uid", &Grid::uid )
        .def_property_readonly( "size", &Grid::size )
        .def_property_readonly( "projection", &Grid::projection )
        .def_property_readonly( "domain", &Grid::domain )
        .def( "lonlat", [](Grid const& self) {
                 return self.lonlat().begin();
             })
        .def( "__repr__",
              []( Grid const& g ) { return "_atlas4py.Grid("_s + py::str( toPyObject( g.spec() ) ) + ")"_s; } );

    py::class_<grid::IteratorLonLat>(m, "IteratorLonLat")
        .def("__iter__", [](grid::IteratorLonLat& it) { return it; })
        .def("__next__", [](grid::IteratorLonLat& it) {
            PointLonLat p;
            if( !it.next(p) ) {
                throw py::stop_iteration();
            }
            return p;});

    py::class_<UnstructuredGrid, Grid>( m, "UnstructuredGrid" )
        .def( py::init( [](py::array_t<double> _xy) {
            py::buffer_info xy = _xy.request();
            auto xy_data = _xy.unchecked<2>();
            auto points = new std::vector<PointXY>(xy.size/2);
            auto& p = *points;
            for(size_t n=0; n<p.size(); ++n) {
                p[n][0] = xy_data(n,0);
                p[n][1] = xy_data(n,1);
            }
            return UnstructuredGrid(points);
        } ) )
        .def( py::init( [](py::array_t<double> _x, py::array_t<double> _y) {
            py::buffer_info x = _x.request();
            py::buffer_info y = _y.request();
            auto x_data = _x.unchecked<1>();
            auto y_data = _y.unchecked<1>();
            ATLAS_ASSERT(x.size == y.size);
            auto points = new std::vector<PointXY>(x.size);
            auto& p = *points;
            for(size_t n=0; n<p.size(); ++n) {
                p[n][0] = x_data(n);
                p[n][1] = y_data(n);
            }
            return UnstructuredGrid(points);
        } ) );

    py::class_<grid::Spacing>( m, "Spacing" )
        .def( "__len__", &grid::Spacing::size )
        .def( "__getitem__", &grid::Spacing::operator[])
        .def( "__repr__", []( grid::Spacing const& spacing ) {
            return "_atlas4py.Spacing("_s + py::str( toPyObject( spacing.spec() ) ) + ")"_s;
        } );
    py::class_<grid::LinearSpacing, grid::Spacing>( m, "LinearSpacing" )
        .def( py::init( []( double start, double stop, long N, bool endpoint ) {
                  return grid::LinearSpacing{ start, stop, N, endpoint };
              } ),
              "start"_a, "stop"_a, "N"_a, "endpoint_included"_a = true );
    py::class_<grid::GaussianSpacing, grid::Spacing>( m, "GaussianSpacing" )
        .def( py::init( []( long N ) { return grid::GaussianSpacing{ N }; } ), "N"_a );

    py::class_<StructuredGrid, Grid>( m, "StructuredGrid" )
        .def( py::init( []( std::string const& s) {
                  return StructuredGrid{ s };
              } ),
              "name"_a )
        .def( py::init( []( std::string const& s, Domain const& d ) {
                  return StructuredGrid{ s, d };
              } ),
              "name"_a, "domain"_a )
        .def( py::init( []( grid::LinearSpacing xSpacing, grid::Spacing ySpacing ) {
                  return StructuredGrid{ xSpacing, ySpacing };
              } ),
              "x_spacing"_a, "y_spacing"_a )
        .def(
            py::init( []( std::vector<grid::LinearSpacing> xLinearSpacings, grid::Spacing ySpacing ) {
                std::vector<grid::Spacing> xSpacings;
                std::copy( xLinearSpacings.begin(), xLinearSpacings.end(), std::back_inserter( xSpacings ) );
                return StructuredGrid{ xSpacings, ySpacing, Projection() };
            } ),
            "x_spacings"_a, "y_spacing"_a )
        .def(
            py::init( []( std::vector<grid::LinearSpacing> xLinearSpacings, grid::Spacing ySpacing, Domain const& d ) {
                std::vector<grid::Spacing> xSpacings;
                std::copy( xLinearSpacings.begin(), xLinearSpacings.end(), std::back_inserter( xSpacings ) );
                return StructuredGrid{ xSpacings, ySpacing, Projection(), d };
            } ),
            "x_spacings"_a, "y_spacing"_a, "domain"_a  )

        .def_property_readonly( "valid", &StructuredGrid::valid )
        .def_property_readonly( "ny", &StructuredGrid::ny )
        .def_property_readonly( "nx", py::overload_cast<>( &StructuredGrid::nx, py::const_ ) )
        .def_property_readonly( "nxmax", &StructuredGrid::nxmax )
        .def_property_readonly( "y", py::overload_cast<>( &StructuredGrid::y, py::const_ ) )
        .def_property_readonly( "x", &StructuredGrid::x )
        .def( "xy", py::overload_cast<idx_t, idx_t>( &StructuredGrid::xy, py::const_ ), "i"_a, "j"_a )
        .def( "lonlat", py::overload_cast<idx_t, idx_t>( &StructuredGrid::lonlat, py::const_ ), "i"_a, "j"_a )
        .def_property_readonly( "reduced", &StructuredGrid::reduced )
        .def_property_readonly( "regular", &StructuredGrid::regular )
        .def_property_readonly( "periodic", &StructuredGrid::periodic );

    py::class_<GaussianGrid, StructuredGrid>( m, "GaussianGrid" )
        .def( py::init( []( Grid const& g ) {
                  return GaussianGrid{ g };
              } ),
              "grid"_a )
        .def_property_readonly( "valid", &GaussianGrid::valid )
        .def("__bool__", &GaussianGrid::valid);

}

}