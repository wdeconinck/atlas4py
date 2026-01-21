#pragma once

#include <string>
#include <pybind11/pybind11.h>

#include "atlas/util/Point.h"

namespace atlas4py {
namespace py = ::pybind11;

void pybind_point(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;

    py::class_<Point2>( m, "Point2" )
        .def( py::init( []( double x, double y ) {
                  return Point2( { x, y } );
              } ) )
        .def( "__getitem__", &Point2::operator() )
        .def( "__repr__", []( Point2 const& p ) {
            return "_atlas4py.Point2(x=" + std::to_string( p.x() ) + ", y=" + std::to_string( p.y() ) + ")";
        } );

    py::class_<PointLonLat, Point2>( m, "PointLonLat" )
        .def( py::init( []( double lon, double lat ) {
                  return PointLonLat( { lon, lat } );
              } ),
              "lon"_a, "lat"_a )
        .def_property_readonly( "lon", py::overload_cast<>( &PointLonLat::lon, py::const_ ) )
        .def_property_readonly( "lat", py::overload_cast<>( &PointLonLat::lat, py::const_ ) )
        .def( "__repr__", []( PointLonLat const& p ) {
            return "_atlas4py.PointLonLat(lon=" + std::to_string( p.lon() ) + ", lat=" + std::to_string( p.lat() ) +
                   ")";
        } );
    py::class_<PointXY, Point2>( m, "PointXY" )
        .def( "__repr__", []( PointXY const& p ) {
            return "_atlas4py.PointXY(x=" + std::to_string( p.x() ) + ", y=" + std::to_string( p.y() ) + ")";
        } );

    py::class_<Point3>( m, "Point3" )
        .def( py::init( []( double x, double y, double z ) {
                  return Point3(x, y, z);
              } ) )
        .def( "__getitem__", &Point3::operator() )
        .def( "__repr__", []( Point3 const& p ) {
            return "_atlas4py.Point3(" + std::to_string( p(0) ) + ", " + std::to_string( p(1) ) +  ", " + std::to_string( p(2) ) + ")";
        } );

    py::class_<PointXYZ, Point3>( m, "PointXYZ" )
            .def( "__repr__", []( PointXYZ const& p ) {
            return "_atlas4py.PointXYZ(x=" + std::to_string( p.x() ) + ", y=" + std::to_string( p.y() ) + ", z=" + std::to_string( p.z() ) + ")";
        } );
}
}  // namespace atlas4py