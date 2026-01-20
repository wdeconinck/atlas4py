#include <functional>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <dlpack/dlpack.h>

#include "atlas/functionspace.h"
#include "atlas/grid.h"
#include "atlas/mesh.h"
#include "atlas/mesh/actions/BuildDualMesh.h"
#include "atlas/mesh/actions/BuildEdges.h"
#include "atlas/mesh/actions/BuildNode2CellConnectivity.h"
#include "atlas/mesh/actions/BuildPeriodicBoundaries.h"
#include "atlas/mesh/actions/BuildHalo.h"
#include "atlas/mesh/actions/BuildParallelFields.h"
#include "atlas/meshgenerator.h"
#include "atlas/option/Options.h"
#include "atlas/output/Gmsh.h"
#include "atlas/library.h"
#include "atlas/trans/Trans.h"
#include "atlas/interpolation.h"
#include "atlas/util/function/VortexRollup.h"
#include "atlas/util/function/SphericalHarmonic.h"

#include "eckit/config/Configuration.h"


namespace py = ::pybind11;
using namespace atlas;
using namespace pybind11::literals;

namespace pybind11 {
namespace detail {
template <>
struct type_caster<atlas::array::ArrayStrides>
    : public type_caster<std::vector<atlas::array::ArrayStrides::value_type>> {};
template <>
struct type_caster<atlas::array::ArrayShape> : public type_caster<std::vector<atlas::array::ArrayShape::value_type>> {};

}  // namespace detail
}  // namespace pybind11

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

void initialise_sys_argv() {
    atlas::initialise(PySys::instance().argc,PySys::instance().argv);
};

void config_set( util::Config& config, const std::string& key, py::handle value ) {
        if ( py::isinstance<py::bool_>( value ) ) {
            config.set(key,value.cast<bool>());
        }
        else if ( py::isinstance<py::int_>( value ) ) {
            config.set(key,value.cast<long long>());
        } 
        else if ( py::isinstance<py::float_>( value ) ) {
            config.set(key,value.cast<double>());
        }
        else if ( py::isinstance<py::str>( value ) ) {
            config.set(key, value.cast<std::string>());
        }
        else {
            throw std::out_of_range( "type of value unsupported" );
        }
}

util::Config to_config( py::kwargs kwargs ) {
    util::Config config;
    for( const auto& pair : kwargs ) {
        const auto key = pair.first.cast<std::string>();
        const auto& value = pair.second;
        config_set(config, key, value);
    }
    return config;
}

py::object toPyObject( eckit::Configuration const& v );
py::object toPyObject( eckit::Configuration const& v, std::string& key );

py::object toPyObject(bool v) {
    return py::bool_(v);
}
py::object toPyObject(long v) {
    return py::int_(v);
}
py::object toPyObject(double v) {
    return py::float_(v);
}
py::object toPyObject(std::string const& v) {
    return py::str(v);
}
template <typename T>
py::object toPyObject( std::vector<T> const& v ) {
    py::list ret;
    for ( auto const& val : v ) {
        ret.append( toPyObject( val ) );
    }
    return ret;
}

py::object toPyObject( eckit::Configuration const& v, std::string const& key ) {
    if ( v.isSubConfiguration ( key ) ) {
        return toPyObject( v.getSubConfiguration( key ) );
    }
    else if (v.isBoolean( key )) {
        return toPyObject( v.getBool( key ) );
    }
    else if (v.isIntegral( key )) {
        return toPyObject( v.getLong( key ) );
    }
    else if (v.isFloatingPoint( key )) {
        return toPyObject( v.getDouble( key ) );
    }
    else if (v.isString( key )) {
        return toPyObject( v.getString( key ) );
    }
    else if (v.isSubConfigurationList( key )) {
        std::vector<eckit::LocalConfiguration> subconfigs = v.getSubConfigurations( key );
        return toPyObject( subconfigs );
    }
    else if (v.isIntegralList( key )) {
        std::vector<long> values = v.getLongVector( key );
        return toPyObject( values );
    }
    else if (v.isFloatingPointList( key )) {
        std::vector<double> values = v.getDoubleVector( key );
        return toPyObject( values );
    }
    else if (v.isStringList( key )) {
        std::vector<std::string> values = v.getStringVector( key );
        return toPyObject( values );
    }
    else if (v.isBooleanList( key )) {
        throw std::out_of_range( "boolean lists not supported for key " + key );
    }
    else {
        throw std::out_of_range( "type of value unsupported for key " + key );
    }
}

py::object toPyObject( eckit::Configuration const& v ) {
    py::dict ret;
    for ( auto const& key : v.keys()) {
        ret[ key.c_str() ] = toPyObject( v, key );
    }
    return ret;
}

std::string atlasToPybind( array::DataType const& dt ) {
    switch ( dt.kind() ) {
        case array::DataType::KIND_INT32:
            return py::format_descriptor<int32_t>::format();
        case array::DataType::KIND_INT64:
            return py::format_descriptor<int64_t>::format();
        case array::DataType::KIND_REAL32:
            return py::format_descriptor<float>::format();
        case array::DataType::KIND_REAL64:
            return py::format_descriptor<double>::format();
        case array::DataType::KIND_UINT64:
            return py::format_descriptor<uint64_t>::format();
        default:
            return "";
    }
}
array::DataType pybindToAtlas( py::dtype const& dtype ) {
    if ( dtype.is( py::dtype::of<int32_t>() ) )
        return array::DataType::KIND_INT32;
    else if ( dtype.is( py::dtype::of<int64_t>() ) )
        return array::DataType::KIND_INT64;
    else if ( dtype.is( py::dtype::of<float>() ) )
        return array::DataType::KIND_REAL32;
    else if ( dtype.is( py::dtype::of<double>() ) )
        return array::DataType::KIND_REAL64;
    else if ( dtype.is( py::dtype::of<uint64_t>() ) )
        return array::DataType::KIND_UINT64;
    else
        return { 0 };
}


}  // namespace

void pybind_dlpack(pybind11::module_ &m);

DLDataType get_dlpack_dtype(atlas::array::DataType const& datatype) {
    DLDataType dl_dtype;
    switch (datatype.kind()) {
    case atlas::array::DataType::KIND_INT32:
        dl_dtype.code = kDLInt;
        dl_dtype.bits = 8 * sizeof(int32_t);
        dl_dtype.lanes = 1;
        break;
    case atlas::array::DataType::KIND_INT64:
        dl_dtype.code = kDLInt;
        dl_dtype.bits = 8 * sizeof(int64_t);
        dl_dtype.lanes = 1;
        break;
    case atlas::array::DataType::KIND_REAL32:
        dl_dtype.code = kDLFloat;
        dl_dtype.bits = 8 * sizeof(float);
        dl_dtype.lanes = 1;
        break;
    case atlas::array::DataType::KIND_REAL64:
        dl_dtype.code = kDLFloat;
        dl_dtype.bits = 8 * sizeof(double);
        dl_dtype.lanes = 1;
        break;
    default:
        throw std::runtime_error("Unsupported data type for DLPack conversion");
    }
    return dl_dtype;
}

void dl_tensor_deleter(DLManagedTensor* self) {
  delete[] self->dl_tensor.shape;
  delete[] self->dl_tensor.strides;
  // Invalidate tensor
  self->dl_tensor.data = nullptr;
  self->dl_tensor.ndim = 0;
  self->dl_tensor.shape = nullptr;
  self->dl_tensor.strides = nullptr;
  self->dl_tensor.byte_offset = 0;
}

void dl_capsule_deleter(PyObject *capsule) {
  void *raw_ptr = nullptr;
  // Can be original name if unused "dltensor"
  if (strcmp("dltensor", PyCapsule_GetName(capsule)) == 0) {
    raw_ptr = PyCapsule_GetPointer(capsule, "dltensor");
  }
  else { // "used_dltensor if capsule is consumed
    raw_ptr = PyCapsule_GetPointer(capsule, "used_dltensor");
  }

  if (raw_ptr) { // Unknown capsule or already freed capsule
    DLManagedTensor* tensor_ptr = static_cast<DLManagedTensor*>(raw_ptr);
    if (tensor_ptr->deleter) // Execute custom deleter, here delete[] shape.
      tensor_ptr->deleter(tensor_ptr);
  }
}

pybind11::capsule get_dlpack_tensor(atlas::array::Array& array) {
    DLTensor dl_tensor;
    dl_tensor.data = array.data();
    dl_tensor.device.device_type = DLDeviceType::kDLCPU;
    dl_tensor.device.device_id = 0;
    dl_tensor.dtype = get_dlpack_dtype(array.datatype());
    dl_tensor.ndim = array.rank();
    auto shape_ptr = std::make_unique<int64_t[]>(dl_tensor.ndim);
    dl_tensor.shape = shape_ptr.get();
    std::copy(array.shape().begin(), array.shape().end(), dl_tensor.shape); // Init with correct shape
    auto strides_ptr = std::make_unique<int64_t[]>(dl_tensor.ndim);
    dl_tensor.strides = strides_ptr.get();
    std::copy(array.strides().begin(), array.strides().end(), dl_tensor.strides); // Init with correct shape
    dl_tensor.byte_offset = 0;

    auto tensor = std::make_unique<DLManagedTensor>();
    tensor->dl_tensor = dl_tensor;
    tensor->manager_ctx = &array;
    tensor->deleter = &dl_tensor_deleter;

    // Release unique pointer to capsule, as we transfer ownership to the
    // python capsule.
    shape_ptr.release();
    strides_ptr.release();
    return py::capsule(tensor.release(), "dltensor", &dl_capsule_deleter);
}

namespace {
template <typename Value>
atlas::Field create_field(const std::string& name, void* data, const array::ArraySpec& spec) {
    return atlas::Field("field", static_cast<Value*>(data), spec);
}

atlas::Field create_field_from_dlpack( const py::object& object, const py::kwargs& kwargs ) {
    py::object dlpack_capsule = object.attr("__dlpack__")(**kwargs);
    auto* tensor = reinterpret_cast<DLManagedTensor*>(PyCapsule_GetPointer(dlpack_capsule.ptr(), "dltensor"));
    if (tensor) {
        DLTensor& dl_tensor = tensor->dl_tensor;
        array::ArrayShape shape;
        array::ArrayStrides strides;
        shape.resize(dl_tensor.ndim);
        strides.resize(dl_tensor.ndim);
        for (int i = 0; i < dl_tensor.ndim; ++i) {
            shape[i] = dl_tensor.shape[i];
            strides[i] = dl_tensor.strides[i];
        }
        std::string name = "tmp";
        auto spec = array::ArraySpec(std::move(shape), std::move(strides));
        switch (dl_tensor.dtype.code) {
            case kDLInt:
                if (dl_tensor.dtype.bits == 32)
                    return create_field<int>(name, dl_tensor.data, spec);
                else if (dl_tensor.dtype.bits == 64)
                    return create_field<long>(name, dl_tensor.data, spec);
                else
                    throw std::runtime_error("Unsupported integer bit-width for DLPack conversion");
                break;
            case kDLFloat:
                if (dl_tensor.dtype.bits == 32)
                    return create_field<float>(name, dl_tensor.data, spec);
                else if (dl_tensor.dtype.bits == 64)
                    return create_field<double>(name, dl_tensor.data, spec);
                else
                    throw std::runtime_error("Unsupported float bit-width for DLPack conversion");
                break;
            default:
                throw std::runtime_error("Unsupported data type for DLPack conversion");
        }
    }
    throw std::runtime_error("Invalid DLPack capsule");
}
}

PYBIND11_MODULE( _atlas4py, m ) {
    pybind_dlpack(m);
    auto m_library = m.def_submodule( "library" );
    m_library.def("initialize", []() { atlas::initialise(PySys::instance().argc, PySys::instance().argv);})
             .def("initialise", []() { atlas::initialise(PySys::instance().argc, PySys::instance().argv);})
             .def("finalize",   []() { atlas::finalize(); })
             .def("finalise",   []() { atlas::finalize(); });
    m_library.attr("version") = atlas::Library::instance().version();

    m.def("initialize", []() { atlas::initialise(PySys::instance().argc, PySys::instance().argv);})
     .def("initialise", []() { atlas::initialise(PySys::instance().argc, PySys::instance().argv);})
     .def("finalize",   []() { atlas::finalize(); })
     .def("finalise",   []() { atlas::finalize(); });

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


    py::class_<Projection>( m, "Projection" ).def( "__repr__", []( Projection const& p ) {
        return "_atlas4py.Projection("_s + py::str( toPyObject( p.spec() ) ) + ")"_s;
    } );
    py::class_<Domain>( m, "Domain" )
        .def_property_readonly( "type", &Domain::type )
        .def_property_readonly( "global", &Domain::global )
        .def_property_readonly( "units", &Domain::units )
        .def( "__repr__", []( Domain const& d ) {
            return "_atlas4py.Domain("_s + ( d ? py::str( toPyObject( d.spec() ) ) : "" ) + ")"_s;
        } );
    py::class_<RectangularDomain, Domain>( m, "RectangularDomain" )
        .def( py::init( []( std::tuple<double, double> xInterval, std::tuple<double, double> yInterval ) {
                  auto [xFrom, xTo] = xInterval;
                  auto [yFrom, yTo] = yInterval;
                  return RectangularDomain( { xFrom, xTo }, { yFrom, yTo } );
              } ),
              "x_interval"_a, "y_interval"_a );

    py::class_<Grid>( m, "Grid" )
        .def( py::init<const std::string&>() )
        .def( py::init( []( const std::string& name, const Domain& domain ) { return Grid(name,domain); } ) )
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
        .def( py::init( []( std::string const& s, Domain const& d ) {
                  return StructuredGrid{ s, d };
              } ),
              "gridname"_a, "domain"_a = Domain() )
        .def( py::init( []( grid::LinearSpacing xSpacing, grid::Spacing ySpacing ) {
                  return StructuredGrid{ xSpacing, ySpacing };
              } ),
              "x_spacing"_a, "y_spacing"_a )
        .def(
            py::init( []( std::vector<grid::LinearSpacing> xLinearSpacings, grid::Spacing ySpacing, Domain const& d ) {
                std::vector<grid::Spacing> xSpacings;
                std::copy( xLinearSpacings.begin(), xLinearSpacings.end(), std::back_inserter( xSpacings ) );
                return StructuredGrid{ xSpacings, ySpacing, Projection(), d };
            } ),
            "x_spacings"_a, "y_spacing"_a, "domain"_a = Domain() )
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

    py::class_<eckit::Configuration>( m, "eckit.Configuration" );
    py::class_<eckit::LocalConfiguration, eckit::Configuration>( m, "eckit.LocalConfiguration" );

    // TODO This is a duplicate of metadata below (because same base class)
    py::class_<util::Config, eckit::LocalConfiguration>( m, "Config" )
        .def( py::init() )
        .def( py::init( []( py::kwargs kwargs) {
            return to_config(kwargs);
        } ) )
        .def( "__setitem__",
              []( util::Config& config, std::string const& key, py::object value ) {
                  config_set(config,key,value);
              } )
        .def( "__getitem__",
              []( util::Config& config, std::string const& key ) -> py::object {
                  if ( !config.has( key ) )
                      throw std::out_of_range( "key <" + key + "> could not be found" );
                  return toPyObject( config, key );
              } )
        .def( "__repr__", []( util::Config const& config ) {
            return "_atlas4py.Config("_s + py::str( toPyObject( config ) ) + ")"_s;
        } );

    py::class_<grid::Partitioner>( m, "Partitioner" )
        .def( py::init( []( py::kwargs kwargs ) { return grid::Partitioner( to_config(kwargs)); } ) )
        .def( py::init( []( util::Config const& config ) { return grid::Partitioner( config ); } ) )
        .def( py::init( []( const std::string& type ) { return grid::Partitioner(type); } ) ) 
        .def( py::init( [](){ return grid::Partitioner(); } ) );

    py::class_<grid::MatchingPartitioner,grid::Partitioner>( m, "MatchingPartitioner" )
        .def( py::init( []( Mesh const& mesh, py::kwargs kwargs ) { return grid::MatchingPartitioner(mesh, to_config(kwargs)); } ) )
        .def( py::init( []( FunctionSpace const& functionspace, py::kwargs kwargs ) { return grid::MatchingPartitioner(functionspace, to_config(kwargs)); } ) );

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

    auto m_fs = m.def_submodule( "functionspace" );
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
                Field field = create_field_from_dlpack(array, kwargs);
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


    py::class_<util::Metadata>( m, "Metadata" )
        .def_property_readonly( "keys", &util::Metadata::keys )
        .def( "__setitem__",
              []( util::Metadata& metadata, std::string const& key, py::object value ) {
                  if ( py::isinstance<py::bool_>( value ) )
                      metadata.set( key, value.cast<bool>() );
                  else if ( py::isinstance<py::int_>( value ) )
                      metadata.set( key, value.cast<long long>() );
                  else if ( py::isinstance<py::float_>( value ) )
                      metadata.set( key, value.cast<double>() );
                  else if ( py::isinstance<py::str>( value ) )
                      metadata.set( key, value.cast<std::string>() );
                  else
                      throw std::out_of_range( "type of value unsupported" );
              } )
        .def( "__getitem__",
              []( util::Metadata& metadata, std::string const& key ) -> py::object {
                  if ( !metadata.has( key ) )
                      throw std::out_of_range( "key <" + key + "> could not be found" );
                  return toPyObject( metadata, key );
              } )
        .def( "__repr__", []( util::Metadata const& metadata ) {
            return "_atlas4py.Metadata("_s + py::str( toPyObject( metadata ) ) + ")"_s;
        } );

    py::class_<Field>( m, "Field", py::buffer_protocol() )
        .def_static( "from_dlpack", []( py::object dlpack_compatible_array, py::kwargs kwargs ) {
            return create_field_from_dlpack( dlpack_compatible_array, kwargs );
        }, "dlpack_tensor"_a)
        .def_property_readonly( "name", &Field::name )
        .def_property_readonly( "strides", &Field::strides )
        .def_property_readonly( "shape", py::overload_cast<>( &Field::shape, py::const_ ) )
        .def_property_readonly( "size", &Field::size )
        .def_property_readonly( "rank", &Field::rank )
        .def_property_readonly( "levels", &Field::levels )
        .def_property_readonly( "datatype", []( Field& f ) { return atlasToPybind( f.datatype() ); } )
        .def_property( "metadata", py::overload_cast<>( &Field::metadata, py::const_ ),
                       py::overload_cast<>( &Field::metadata ) )
        .def_property_readonly( "functionspace", py::overload_cast<>( &Field::functionspace, py::const_ ) )
        .def( "halo_exchange", []( Field& f) { f.haloExchange(); })
        .def_property( "halo_dirty", &Field::dirty, &Field::set_dirty, py::return_value_policy::copy)
        .def_buffer( []( Field& f ) {
            auto strides = f.strides();
            auto sizeof_dtype = f.datatype().size();
            std::transform( strides.begin(), strides.end(), strides.begin(),
                            [&]( auto const& stride ) { return stride * sizeof_dtype; } );
            return py::buffer_info( f.storage(), sizeof_dtype, atlasToPybind( f.datatype() ), f.rank(),
                                    f.shape(), strides );
        })
        .def( "__dlpack__", []( Field& f, const py::object& stream) {
            return get_dlpack_tensor(f);
        }, py::arg("stream") = py::none() )
        .def( "__dlpack_device__", []( Field&f ) { return std::pair<int32_t, int64_t>{kDLCPU, 0}; }
            // Device type codes are defined in dlpack/dlpack.h
            // CPU = 1
            // CUDA = 2
            // CPU_PINNED = 3
            // OPENCL = 4
            // VULKAN = 7
            // METAL = 8
            // VPI = 9
            // ROCM = 10
            // CUDA_MANAGED = 13
            // ONE_API = 14
    );

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


    py::class_<trans::Trans>( m, "Trans" )
        .def( py::init( [](const FunctionSpace& gp, const FunctionSpace& sp, py::kwargs kwargs){ return trans::Trans(gp,sp,to_config(kwargs)); } ), "gp"_a, "sp"_a )
        .def( py::init( [](const Grid& grid, int truncation, py::kwargs kwargs){ return trans::Trans(grid,truncation,to_config(kwargs));} ), "grid"_a, "truncation"_a )
        .def( "dirtrans", []( trans::Trans& trans, const Field& gpfield, Field& spfield) { trans.dirtrans(gpfield,spfield);} )
        .def( "invtrans", []( trans::Trans& trans, const Field& spfield, Field& gpfield) { trans.invtrans(spfield,gpfield);} )
        .def_property_readonly( "truncation", &trans::Trans::truncation )
        .def_property_readonly( "nb_spectral_coefficients", &trans::Trans::spectralCoefficients )
        .def_static("backend", [] ( const std::string& backend ){ trans::Trans::backend(backend); } )
        .def_static("has_backend", [] ( const std::string& backend ){ return trans::Trans::hasBackend(backend); } );

    py::class_<Interpolation>( m, "Interpolation" )
        .def( py::init( [](const std::string& type, const FunctionSpace& source, const FunctionSpace& target, py::kwargs kwargs){
                auto config = to_config(kwargs);
                config.set("type",type);
                return Interpolation(config,source,target);
            } ), "type"_a, "source"_a, "target"_a )
        .def( py::init( [](const std::string& type, const Grid& source, const Grid& target, py::kwargs kwargs){
                auto config = to_config(kwargs);
                config.set("type",type);
                return Interpolation(config,source,target);
            } ), "type"_a, "source"_a, "target"_a )
        .def( "execute", []( Interpolation const& self, const Field& source, Field& target) { return self.execute(source,target);} )
        .def_property_readonly( "source", &Interpolation::source )
        .def_property_readonly( "target", &Interpolation::target );


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


    auto m_function = m.def_submodule( "function" );
    m_function.def("vortex_rollup", [](double lon, double lat, double t) { return util::function::vortex_rollup(lon,lat,t); } );
    m_function.def("spherical_harmonic", [](double lon, double lat,int n, int m ) { return util::function::spherical_harmonic(n,m,lon,lat); }, "lon"_a, "lat"_a, "n"_a, "m"_a );

}


void pybind_dlpack(py::module_ &m) {
    py::module_ dlpack = m.def_submodule("dlpack", "DLpack python binding.");
    py::enum_<DLDeviceType>(dlpack, "DLDeviceType")
        .value("kDLCPU", DLDeviceType::kDLCPU)
        .value("kDLCUDA", DLDeviceType::kDLCUDA)
        .value("kDLCUDAHost", DLDeviceType::kDLCUDAHost)
        .value("kDLOpenCL", DLDeviceType::kDLOpenCL)
        .value("kDLVulkan", DLDeviceType::kDLVulkan)
        .value("kDLMetal", DLDeviceType::kDLMetal)
        .value("kDLVPI", DLDeviceType::kDLVPI)
        .value("kDLROCM", DLDeviceType::kDLROCM)
        .value("kDLROCMHost", DLDeviceType::kDLROCMHost)
        .value("kDLExtDev", DLDeviceType::kDLExtDev)
        .value("kDLCUDAManaged", DLDeviceType::kDLCUDAManaged)
        .value("kDLOneAPI", DLDeviceType::kDLOneAPI)
        .value("kDLWebGPU", DLDeviceType::kDLWebGPU)
        // .value("kDLHexagon", kDLHexagon::kDLHexagon)
        .value("kDLMAIA", kDLMAIA)
        .export_values(); // DLPack is C, so we don't have strongly typed enums
    dlpack.attr("DLPACK_MAJOR_VERSION") = DLPACK_MAJOR_VERSION;
    dlpack.attr("DLPACK_MINOR_VERSION") = DLPACK_MINOR_VERSION;
}