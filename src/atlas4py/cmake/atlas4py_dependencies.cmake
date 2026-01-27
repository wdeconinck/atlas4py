# This file handles the dependencies of atlas4py. It defines two macros:
# - atlas4py_find_dependencies: which searches for the dependencies of atlas4py (atlas, eckit, and ecbuild)
#   and sets variables build_atlas, build_eckit, and build_ecbuild to indicate which dependencies need to be built from source
# - atlas4py_add_missing_dependencies: which builds the missing dependencies from source given the variables
#   build_atlas, build_eckit, and build_ecbuild set by atlas4py_find_dependencies


macro(atlas4py_find_dependencies)
    if (NOT build_atlas)
        if (DEFINED ENV{ATLAS_INSTALL_DIR})
            set( atlas_ROOT ${ENV{ATLAS_INSTALL_DIR}} )
        endif()
        find_package(atlas CONFIG QUIET)
        if( atlas_FOUND )
            message( STATUS "Found atlas: ${atlas_DIR} (found version \"${atlas_VERSION}\")" )
            message( STATUS "Found eckit: ${eckit_DIR} (found version \"${eckit_VERSION}\")" )
            if (NOT atlas_VERSION VERSION_EQUAL ATLAS4PY_ATLAS_VERSION)
                message( WARNING "Found atlas version \"${atlas_VERSION}\", but configured version is \"${ATLAS4PY_ATLAS_VERSION}\"" )
            endif()
            if (NOT eckit_VERSION VERSION_EQUAL ATLAS4PY_ECKIT_VERSION)
                message( WARNING "Found eckit version \"${eckit_VERSION}\", but configured version is \"${ATLAS4PY_ECKIT_VERSION}\"" )
            endif()
        else()
            set(build_atlas TRUE CACHE INTERNAL "build atlas")
        endif()
    endif()
    if(build_atlas)
        if (NOT build_ecbuild)
            find_package(ecbuild CONFIG QUIET)
            if (ecbuild_FOUND)
                message( STATUS "Found ecbuild: ${ecbuild_DIR} (found version \"${ecbuild_VERSION}\")" )
                if (NOT ecbuild_VERSION VERSION_EQUAL ATLAS4PY_ECBUILD_VERSION)
                    message( WARNING "Found ecbuild version \"${ecbuild_VERSION}\", but configured version is \"${ATLAS4PY_ECBUILD_VERSION}\"" )
                endif()
            else()
                set(build_ecbuild TRUE CACHE INTERNAL "build ecbuild")
            endif()
        endif()
        if (NOT build_eckit)
            find_package(eckit CONFIG QUIET)
            if( eckit_FOUND )
                message( STATUS "Found eckit: ${eckit_DIR} (found version \"${eckit_VERSION}\")" )
                if (NOT eckit_VERSION VERSION_EQUAL ATLAS4PY_ECKIT_VERSION)
                    message( WARNING "Found eckit version \"${eckit_VERSION}\", but configured version is \"${ATLAS4PY_ECKIT_VERSION}\"" )
                endif()
            else()
                set(build_eckit TRUE CACHE INTERNAL "build eckit")
            endif()
        endif()
    endif()
endmacro()

# Build missing dependencies from source given by variables ATLAS4PY_ATLAS_VERSION, ATLAS4PY_ECKIT_VERSION, and ATLAS4PY_ECBUILD_VERSION
macro(atlas4py_add_missing_dependencies)
    if(build_atlas)
        include(FetchContent)
        if (build_ecbuild)
            set ( ecbuild_SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/ecbuild )
            message( STATUS "Downloading ecbuild version \"${ATLAS4PY_ECBUILD_VERSION}\" to ${ecbuild_SOURCE_DIR}" )
            FetchContent_Populate(
                ecbuild
                GIT_REPOSITORY https://github.com/ecmwf/ecbuild.git
                GIT_TAG        ${ATLAS4PY_ECBUILD_VERSION}
                SOURCE_DIR     ${ecbuild_SOURCE_DIR}
                QUIET
            )
            set( ecbuild_ROOT ${ecbuild_SOURCE_DIR} CACHE INTERNAL "Found ecbuild" )
        endif()
        if (build_eckit)
            message( STATUS "Downloading and building eckit version \"${ATLAS4PY_ECKIT_VERSION}\"" )

            # Disable unused features for faster compilation
            set(ECKIT_ENABLE_TESTS     OFF)
            set(ECKIT_ENABLE_DOCS      OFF)
            set(ECKIT_ENABLE_PKGCONFIG OFF)
            set(ECKIT_ENABLE_ECKIT_GEO OFF)
            set(ECKIT_ENABLE_ECKIT_SQL OFF)
            set(ECKIT_ENABLE_ECKIT_CMD OFF)
            FetchContent_Declare(
                eckit
                GIT_REPOSITORY https://github.com/ecmwf/eckit.git
                GIT_TAG        ${ATLAS4PY_ECKIT_VERSION}
            )
            FetchContent_MakeAvailable(eckit)
            set( eckit_ROOT ${eckit_BINARY_DIR} CACHE INTERNAL "Found eckit" )
        endif()

        message( STATUS "Downloading and building atlas version \"${ATLAS4PY_ATLAS_VERSION}\"" )

        # Disable unused features for faster compilation
        set(ATLAS_ENABLE_TESTS     OFF)
        set(ATLAS_ENABLE_DOCS      OFF)
        set(ATLAS_ENABLE_PKGCONFIG OFF)
        set(ECKIT_ENABLE_ECKIT_GEO OFF)
        set(ECKIT_ENABLE_ECKIT_SQL OFF)
        set(ATLAS_ENABLE_ECKIT_CMD OFF)
        FetchContent_Declare(
            atlas
            GIT_REPOSITORY https://github.com/ecmwf/atlas.git
            GIT_TAG        ${ATLAS4PY_ATLAS_VERSION}
        )
        FetchContent_MakeAvailable(atlas)
    endif()
endmacro()
