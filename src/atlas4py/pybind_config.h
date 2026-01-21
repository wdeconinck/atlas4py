#pragma once

#include <pybind11/pybind11.h>

#include "eckit/config/Configuration.h"

namespace atlas4py {
namespace py = ::pybind11;
void config_set( eckit::LocalConfiguration& config, const std::string& key, py::handle value ) {
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

atlas::util::Config to_config( py::kwargs kwargs ) {
    atlas::util::Config config;
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

void pybind_config(py::module_ &m) {
    using namespace py::literals;
    py::class_<eckit::Configuration>( m, "eckit.Configuration" )
        .def( "__getitem__",
              []( eckit::Configuration& config, std::string const& key ) -> py::object {
                  if ( !config.has( key ) )
                      throw std::out_of_range( "key <" + key + "> could not be found" );
                  return toPyObject( config, key );
              } )
        .def( "__repr__", []( eckit::Configuration const& config ) {
            return "_atlas4py.eckit.Configuration("_s + py::str( toPyObject( config ) ) + ")"_s;
        } );

    py::class_<eckit::LocalConfiguration, eckit::Configuration>( m, "eckit.LocalConfiguration" )
        .def( py::init() )
        .def( "__setitem__",
              []( eckit::LocalConfiguration& config, std::string const& key, py::object value ) {
                  config_set(config,key,value);
              } )
        .def( "__repr__", []( eckit::LocalConfiguration const& config ) {
            return "_atlas4py.eckit.LocalConfiguration("_s + py::str( toPyObject( config ) ) + ")"_s;
        } );

    py::class_<atlas::util::Config, eckit::LocalConfiguration>( m, "Config" )
        .def( py::init() )
        .def( py::init( []( py::kwargs kwargs) {
            return to_config(kwargs);
        } ) )
        .def( "__repr__", []( atlas::util::Config const& config ) {
            return "_atlas4py.Config("_s + py::str( toPyObject( config ) ) + ")"_s;
        } );
}
}