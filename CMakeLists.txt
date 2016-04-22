#
# Cynnypp, common C++11 utilities.
#
#

#
# Building in-tree is not allowed (we take care of your craziness).
#
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Prevented in-tree built. Please create a build directory outside of the source code and call cmake from there. Thank you.")
endif()


#
# Project configuration
#

project(cynnypp)
cmake_minimum_required(VERSION 3.2)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT DEFINED ENV{LIBRARY_TYPE})
    set(LIBRARY_TYPE "shared")
    message("-- LIBRARY_TYPE not specified: picking \"${LIBRARY_TYPE}\" by default")
else()
    set(LIBRARY_TYPE $ENV{LIBRARY_TYPE})
endif()

if(LIBRARY_TYPE STREQUAL "shared")
    message("-- LIBRARY_TYPE is set to ${LIBRARY_TYPE} by environment variable")
    set(LIBRARY_SHARED ON)
    set(LIBRARY_STATIC OFF)
elseif(LIBRARY_TYPE STREQUAL "static")
    message("-- LIBRARY_TYPE is set to ${LIBRARY_TYPE} by environment variable")
    set(LIBRARY_SHARED OFF)
    set(LIBRARY_STATIC ON)
elseif(LIBRARY_TYPE STREQUAL "all") # // TODO: does it make sense? is it feasible?
    message("all")
    set(LIBRARY_SHARED ON)
    set(LIBRARY_STATIC ON)
else()
    message("*** Warning: unsupported value for LIBRARY_TYPE: \"$ENV{LIBRARY_TYPE}\"; I will compile shared libs")
    set(LIBRARY_SHARED ON)
    set(LIBRARY_STATIC OFF)
endif()



set(PROJECT_NAME "cynnypp")
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(SETTINGS_ORGANIZATION "Cynny SpA")
set(SETTINGS_APPLICATION ${PROJECT_NAME})
set(PROJECT_AUTHOR "Cynny SpA")
set(PROJECT_YEAR_COPYRIGHT "2016")
set(PROJECT_AUTHOR_EMAIL "info@cynny.com")

set(PROJECT_BUILD_MESSAGE ${PROJECT_NAME} " v" ${PROJECT_VERSION} " (" ${CMAKE_BUILD_TYPE} ")")
set(COPYRIGHT_BUILD_MESSAGE "Copyright (c) " ${PROJECT_YEAR_COPYRIGHT} " " ${PROJECT_AUTHOR}  " <" ${PROJECT_AUTHOR_EMAIL} ">")

set(ARCHITECTS_BUILD_MESSAGE "Designed by")
set(DEVELOPERS_BUILD_MESSAGE "Developed by")
set(CONTRIBUTORS_BUILD_MESSAGE "Contributors")
set(DEVS_CYNNY_CPP "The Cynny C++ team")

message("*")
message("* " ${PROJECT_BUILD_MESSAGE})
message("* " ${COPYRIGHT_BUILD_MESSAGE})
message("*")
message("* " ${ARCHITECTS_BUILD_MESSAGE})
message("*     " ${DEVS_CYNNY_CPP})
message("*")
message("* " ${DEVELOPERS_BUILD_MESSAGE})
message("*     " ${DEVS_CYNNY_CPP})
message("*")
message("* " ${CONTRIBUTORS_BUILD_MESSAGE})
message("*")

if(APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-undefined,dynamic_lookup")
else(APPLE)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
endif(APPLE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14 -pthread -Wno-missing-braces")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Weffc++")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -DRELEASE")

add_definitions(
        -DPROJECT_NAME=${PROJECT_NAME}
        -DPROJECT_VERSION=${PROJECT_VERSION}
)


#
# CMake configuration
#

set(PROJECT_CMAKE_CLEAN cmake/clean)
set(PROJECT_CMAKE_MODULES cmake/modules)
set(PROJECT_INSTALL_DIR install)
set(PROJECT_BUILD_DIR build)
set(PROJECT_DEPS_DIR deps)
set(PROJECT_DOCS_DIR docs)
set(PROJECT_SRC_DIR src)
set(PROJECT_TEST_DIR test)

set(PROJECT_RUNTIME_OUTPUT_DIRECTORY bin)
set(PROJECT_SHARE_OUTPUT_DIRECTORY share)
set(PROJECT_LIBRARY_OUTPUT_DIRECTORY lib)
set(PROJECT_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_LIBRARY_OUTPUT_DIRECTORY})
set(PROJECT_INCLUDE_OUTPUT_DIRECTORY include)


set(
    CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/${PROJECT_CMAKE_MODULES}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_ARCHIVE_OUTPUT_DIRECTORY})

#
# Enable test support using ctest
#

include(CTest)

#
# Referenced packages
#

find_package(Doxygen 1.8)
find_package(Threads REQUIRED)
find_package(Catch)

set(THREADS_PREFER_PTHREAD_FLAG ON)


#
# Referenced directories and targets
#

set(TARGET_CYNNYPP cynnypp)
set(CYNNYPP_TEST_ALL test_all)
set(TARGET_DOCS docs)

add_subdirectory(${PROJECT_SRC_DIR})

if(${Catch_FOUND})
    add_subdirectory(${PROJECT_TEST_DIR})
endif(  ${Catch_FOUND})

#if(${DOXYGEN_FOUND})
#    add_subdirectory(${PROJECT_DOCS_DIR})
#endif(${DOXYGEN_FOUND})

#
# Keep your stuff and go further away, foolish.
#

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VENDOR ${PROJECT_AUTHOR})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cynnypp: Common c++ utilities")
set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION})
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${CPACK_PACKAGE_FILE_NAME}-src)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README)
set(CPACK_GENERATOR TGZ)
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_PACKAGING_INSTALL_DIRECTORY "cynnypp-${CMake_VERSION_MAJOR}.${CMake_VERSION_MINOR}")



include(CPack)

#
# I use QtCreator and I need the lines below, so do not ask. :-)
#

file(
        GLOB_RECURSE PROJECT_FILES FOLLOW_SYMLINKS
        *.txt *.mk *.c *.cpp *.hpp *.h *.java *.in *.cmake *.m *.mm
        *.djinni *.idl *.dox *.sh Makefile *.example *.config *.build
        *.xml *.py *.scala README
)

add_custom_target(
        QTCREATOR_FALLBACK ALL
        COMMENT "Feel free to ignore this target, but please do not remove it."
        SOURCES ${PROJECT_FILES}
)
