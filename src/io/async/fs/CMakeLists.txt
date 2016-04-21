# boost configuration
if(DEFINED ENV{JENKINS})
    set(Boost_NO_SYSTEM_PATHS ON)
    set(BOOST_ROOT /home/jenkins/atlas)
    set(BOOST_LIBRARYDIR /home/jenkins/atlas/lib)
    set(BOOST_INCLUDEDIR /home/jenkins/atlas/include)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/home/jenkins/atlas/lib -static-libstdc++")
    message("Setting custom boost!")
endif()
set(Boost_USE_MULTITHREADED OFF)
if(${LIBRARY_TYPE} EQUAL "SHARED")
    set(Boost_USE_STATIC_LIBS OFF)
else()
    set(Boost_USE_STATIC_LIBS ON)
endif()

find_package(Boost 1.54 REQUIRED COMPONENTS system filesystem)


#
# AsyncFs configuration
#

set(ASYNC_FS_SRC_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/io/async/fs/)

# list all source files inside the src dir
file(GLOB_RECURSE ASYNC_FS_SOURCES ${ASYNC_FS_SRC_DIR}/*.cpp)

# Common shared sources
set(ASYNC_FS_LIBRARY async_fs)

if(${LIBRARY_TYPE} EQUAL "SHARED")

    add_library(
        ${ASYNC_FS_LIBRARY} SHARED
        ${ASYNC_FS_SOURCES}
        ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/io/locales.cpp
        )
    target_include_directories(${ASYNC_FS_LIBRARY} PRIVATE ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/)
    set_target_properties(${ASYNC_FS_LIBRARY} PROPERTIES OUTPUT_NAME cynpp_async_fs)
    target_link_libraries(${ASYNC_FS_LIBRARY} PRIVATE ${Boost_LIBRARIES})
else()

    add_library(
        ${ASYNC_FS_LIBRARY} STATIC
        ${ASYNC_FS_SOURCES}
        ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/io/locales.cpp
        )

    # include directories for the static lib
    target_include_directories(${ASYNC_FS_LIBRARY} PRIVATE ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/)

    #set the same output name for both static and shared library
    set_target_properties(${ASYNC_FS_LIBRARY} PROPERTIES OUTPUT_NAME cynpp_async_fs)

    # set -fPIC to enable compilation for shared lib
    set_target_properties(${ASYNC_FS_LIBRARY} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # library dependency for the shared and static lib
    target_link_libraries(${ASYNC_FS_LIBRARY} PRIVATE ${Boost_LIBRARIES})
endif()

# create the targets for shared and static libraries

