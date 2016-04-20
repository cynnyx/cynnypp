# FindCatch
# ---------
#
# Locate Catch library (headers only)
#
# This module defines:
#
# ::
#
#   Catch_INCLUDE_DIRS, where to find the headers
#   Catch_FOUND, if false, do not try to link against
#

set(BUILD_DEPS_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_DEPS_DIR})
set(CATCH_DEPS_DIR Catch)

find_path(
    Catch_INCLUDE_DIR NAMES catch.hpp
    PATHS ${BUILD_DEPS_DIR}/${CATCH_DEPS_DIR}/include
    NO_DEFAULT_PATH
)

set(Catch_INCLUDE_DIRS ${Catch_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
    Catch
    FOUND_VAR Catch_FOUND
    REQUIRED_VARS
        Catch_INCLUDE_DIRS
)

mark_as_advanced(Catch_INCLUDE_DIR)
