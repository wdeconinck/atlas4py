#pragma once

#include <string>
#include <pybind11/pybind11.h>

#include "atlas/util/KDTree.h"

namespace atlas4py {
namespace py = ::pybind11;

void pybind_kdtree(py::module_ &m) {
    using namespace atlas;
    using namespace py::literals;

    py::class_<util::IndexKDTree::Value>( m, "IndexKDTreeValue" )
        .def_property_readonly( "point", [](const util::IndexKDTree::Value& v) { return v.point(); } )
        .def_property_readonly( "payload", [](const util::IndexKDTree::Value& v) { return v.payload(); } )
        .def_property_readonly( "index", [](const util::IndexKDTree::Value& v) { return v.payload(); } )
        .def_property_readonly( "distance", [](const util::IndexKDTree::Value& v) { return v.distance(); } )
        ;

    py::class_<util::IndexKDTree::ValueList>( m, "IndexKDTreeValueList" )
        .def_property_readonly( "indices", [](const util::IndexKDTree::ValueList& v) { return v.payloads(); } )
        .def( "__getitem__", [](const util::IndexKDTree::ValueList& v, size_t i) { return v[i]; } )
        .def( "size", [](const util::IndexKDTree::ValueList& v) { return v.size(); } )
        ;

    py::class_<util::IndexKDTree>( m, "IndexKDTree" )
        .def( py::init([](){ return util::IndexKDTree(); }))
        .def( py::init([](const std::string& geometry){ return util::IndexKDTree(util::Config("geometry",geometry)); }), "geometry"_a)
        .def( "reserve", [](util::IndexKDTree& self, idx_t size) { self.reserve(size); })
        .def( "insert", [](util::IndexKDTree& self, const PointLonLat& point, util::IndexKDTree::Payload payload){ self.insert(point, payload); })
        .def( "insert", [](util::IndexKDTree& self, const PointXYZ& point, util::IndexKDTree::Payload payload){ self.insert(point, payload); })
        .def( "build", [](util::IndexKDTree& self) { self.build();} )
        .def( "build", [](util::IndexKDTree& self, py::array_t<double> points) {
            py::buffer_info info = points.request();
            auto size = info.shape[0];
            self.reserve(size);
            if (info.ndim == 2 ) {
                const PointLonLat* lonlat = reinterpret_cast<const PointLonLat*>(info.ptr);
                for( size_t j=0; j<size; ++j) {
                    self.insert(lonlat[j], j);
                }
            }
            else if (info.ndim == 3) {
                const PointXYZ* xyz = reinterpret_cast<const PointXYZ*>(info.ptr);
                for( size_t j=0; j<size; ++j) {
                    self.insert(xyz[j], j);
                }
            }
            self.build();
            } )
        .def( "closest_point", [](util::IndexKDTree& tree, const PointLonLat& point) {
            return tree.closestPoint(point); } )
        .def( "closest_point", [](util::IndexKDTree& tree, const PointXYZ& point) {
            return tree.closestPoint(point); } )
        .def( "closest_point", [](util::IndexKDTree& tree, double lon, double lat) {
            return tree.closestPoint(PointLonLat{lon,lat}); }, "lon"_a, "lat"_a)

        .def( "closest_points", [](util::IndexKDTree& tree, const PointLonLat& point, size_t k) {
            return tree.closestPoints(point, k); } )
        .def( "closest_points", [](util::IndexKDTree& tree, const PointXYZ& point, size_t k) {
            return tree.closestPoints(point, k); } )
        .def( "closest_points", [](util::IndexKDTree& tree, double lon, double lat, size_t k) {
            return tree.closestPoints(PointLonLat{lon,lat}, k); }, "lon"_a, "lat"_a, "k"_a )

        .def( "closest_points_within_radius", [](util::IndexKDTree& tree, const PointLonLat& point, double radius) {
            return tree.closestPointsWithinRadius(point, radius); } )
        .def( "closest_points_within_radius", [](util::IndexKDTree& tree, const PointXYZ& point, double radius) {
            return tree.closestPointsWithinRadius(point, radius); } )
        .def( "closest_points_within_radius", [](util::IndexKDTree& tree, double lon, double lat, double radius) {
            return tree.closestPointsWithinRadius(PointLonLat{lon,lat}, radius); }, "lon"_a, "lat"_a, "radius"_a )

        .def( "closest_index", [](util::IndexKDTree& tree, const PointLonLat& point) {
            return tree.closestPoint(point).payload(); } )
        .def( "closest_index", [](util::IndexKDTree& tree, const PointXYZ& point) {
            return tree.closestPoint(point).payload(); } )
        .def( "closest_index", [](util::IndexKDTree& tree, double lon, double lat) {
            return tree.closestPoint(PointLonLat{lon,lat}).payload(); }, "lon"_a, "lat"_a)

        .def( "closest_indices", [](util::IndexKDTree& tree, const PointLonLat& point, size_t k) {
            return tree.closestPoints(point, k).payloads(); } )
        .def( "closest_indices", [](util::IndexKDTree& tree, const PointXYZ& point, size_t k) {
            return tree.closestPoints(point, k).payloads(); } )
        .def( "closest_indices", [](util::IndexKDTree& tree, double lon, double lat, size_t k) {
            return tree.closestPoints(PointLonLat{lon,lat}, k).payloads(); }, "lon"_a, "lat"_a, "k"_a )

        .def( "closest_indices_within_radius", [](util::IndexKDTree& tree, const PointLonLat& point, double radius) {
            return tree.closestPointsWithinRadius(point, radius).payloads(); } )
        .def( "closest_indices_within_radius", [](util::IndexKDTree& tree, const PointXYZ& point, double radius) {
            return tree.closestPointsWithinRadius(point, radius).payloads(); } )
        .def( "closest_indices_within_radius", [](util::IndexKDTree& tree, double lon, double lat, double radius) {
            return tree.closestPointsWithinRadius(PointLonLat{lon,lat}, radius).payloads(); }, "lon"_a, "lat"_a, "radius"_a )

        ;

}
}
