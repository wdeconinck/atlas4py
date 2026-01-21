
if( NOT ATLAS4PY_CMAKE_MINIMUM_REQUIRED_VERSION) 
 set(ATLAS4PY_CMAKE_MINIMUM_REQUIRED_VERSION 3.14)
endif()

cmake_minimum_required(VERSION ${ATLAS4PY_CMAKE_MINIMUM_REQUIRED_VERSION})

project(atlas4py LANGUAGES CXX)

cmake_policy(SET CMP0074 NEW)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

function(copy_target ct_tgt)
    add_custom_target(copy_${ct_tgt} ALL
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${ct_tgt}> ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        DEPENDS ${ct_tgt}
    )
endfunction()

find_package(atlas QUIET PATHS $ENV{ATLAS_INSTALL_DIR})
if( atlas_FOUND )
  message( "Found atlas: ${atlas_DIR} (found version \"${atlas_VERSION}\")" )
endif()

if(NOT atlas_FOUND)
    find_package(ecbuild)
    if(NOT ecbuild_FOUND)
        FetchContent_Declare(
            ecbuild
            GIT_REPOSITORY https://github.com/ecmwf/ecbuild.git
            GIT_TAG        ${ATLAS4PY_ECBUILD_VERSION}
        )
        FetchContent_MakeAvailable(ecbuild)
    endif()
    find_package(eckit)
    if( eckit_FOUND )
      message( "Found eckit: ${eckit_DIR} (found version \"${eckit_VERSION}\")" )
    endif()

    if(NOT eckit_FOUND)
        FetchContent_Declare(
            eckit
            GIT_REPOSITORY https://github.com/ecmwf/eckit.git
            GIT_TAG        ${ATLAS4PY_ECKIT_VERSION}
        )
        FetchContent_MakeAvailable(eckit)
        set(_atlas4py_built_eckit ON)
    endif()

    FetchContent_Declare(
        atlas
        GIT_REPOSITORY https://github.com/ecmwf/atlas.git
        GIT_TAG        ${ATLAS4PY_ATLAS_VERSION}
    )
    set( ENABLE_GRIDTOOLS_STORAGE OFF CACHE BOOL "" FORCE )
    FetchContent_MakeAvailable(atlas)

    copy_target(atlas)
    copy_target(atlas_io)

    if(_atlas4py_built_eckit)
        # copy atlas' eckit dependencies
        get_target_property(deps atlas LINK_LIBRARIES)
        get_target_property(deps_interface atlas INTERFACE_LINK_LIBRARIES)
        list(APPEND deps ${deps_interface})
        list(REMOVE_DUPLICATES deps)
        foreach(dep IN LISTS deps)
            if(dep MATCHES "eckit.*")
                copy_target(${dep})
            endif()
        endforeach()
    endif()
endif()

# https://github.com/cpm-cmake/CPM.cmake
include (cmake/get_cpm.cmake)
set(PYBIND11_FINDPYTHON NEW)
CPMAddPackage("gh:pybind/pybind11@3.0") # CPMAddPackage("gh:pybind/pybind11@${ATLAS4PY_PYBIND11_VERSION}")
CPMAddPackage("gh:dmlc/dlpack@1.2")

pybind11_add_module(_atlas4py _atlas4py.cpp)
target_include_directories(_atlas4py PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(_atlas4py PRIVATE atlas)
target_link_libraries(_atlas4py PRIVATE dlpack::dlpack)
