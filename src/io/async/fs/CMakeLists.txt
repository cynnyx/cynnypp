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

find_package(Boost 1.54 REQUIRED COMPONENTS system filesystem)



#
# AsyncFs configuration
#

set(ASYNC_FS_SRC_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/io/async/fs/)

# list all source files inside the src dir
file(GLOB_RECURSE ASYNC_FS_SOURCES ${ASYNC_FS_SRC_DIR}/*.cpp)

# Common shared sources
set(ASYNC_FS_LIBRARY_SHARED async_fs_shared)
set(ASYNC_FS_LIBRARY_STATIC async_fs_static)


# note: we cannot compile the code just once, because for the shared version we have to set -fPIC

# todo: we could add another env var that override the global LIBRARY_TYPE specifically for this lib

# add target for the shared lib
if(${LIBRARY_SHARED})
    add_library(
        ${ASYNC_FS_LIBRARY_SHARED} SHARED
        ${ASYNC_FS_SOURCES}
        ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/io/locales.cpp
        )
    target_include_directories(${ASYNC_FS_LIBRARY_SHARED} PRIVATE ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/)
    target_link_libraries(${ASYNC_FS_LIBRARY_SHARED} PRIVATE ${Boost_LIBRARIES})

    # set positions independet code flag for shared lib
    set_target_properties(${ASYNC_FS_LIBRARY_SHARED} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # set the output name
    set_target_properties(${ASYNC_FS_LIBRARY_SHARED} PROPERTIES OUTPUT_NAME cynpp_async_fs)
endif()

# add target for the static lib
if(${LIBRARY_STATIC})
    add_library(
        ${ASYNC_FS_LIBRARY_STATIC} STATIC
        ${ASYNC_FS_SOURCES}
        ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/io/locales.cpp
        )

    target_include_directories(${ASYNC_FS_LIBRARY_STATIC} PRIVATE ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/)
    target_link_libraries(${ASYNC_FS_LIBRARY_STATIC} PRIVATE ${Boost_LIBRARIES})

    # set the output name
    set_target_properties(${ASYNC_FS_LIBRARY_STATIC} PROPERTIES OUTPUT_NAME cynpp_async_fs)
endif()

