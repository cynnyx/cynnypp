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
# SwappingBuffer configuration
#

set(SWAPPING_BUF_SRC_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/io/async/swap/)

# list all source files inside the src dir
file(GLOB_RECURSE SWAP_BUF_SOURCES ${SWAPPING_BUF_SRC_DIR}/*.cpp)

# Common shared sources
set(SWAP_BUF_LIBRARY_NAME cynpp_swap_buf)
set(SWAP_BUF_LIBRARY_SHARED swap_buf_shared)
set(SWAP_BUF_LIBRARY_STATIC swap_buf_static)


# note: we cannot compile the code just once, because for the shared version we have to set -fPIC

# todo: we could add another env var that override the global LIBRARY_TYPE specifically for this lib

# add target for the shared lib
if(${LIBRARY_SHARED})
    add_library(
        ${SWAP_BUF_LIBRARY_SHARED} SHARED
        ${SWAP_BUF_SOURCES}
        )
    target_include_directories(${SWAP_BUF_LIBRARY_SHARED} PRIVATE ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/)
    target_link_libraries(${SWAP_BUF_LIBRARY_SHARED} PRIVATE ${Boost_LIBRARIES} PRIVATE ${ASYNC_FS_LIBRARY_SHARED})

    # set positions independet code flag for shared lib
    set_target_properties(${SWAP_BUF_LIBRARY_SHARED} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # set output name and path
    set_target_properties(
        ${SWAP_BUF_LIBRARY_SHARED}
        PROPERTIES
        OUTPUT_NAME ${SWAP_BUF_LIBRARY_NAME}
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        )
endif()

# add target for the static lib
if(${LIBRARY_STATIC})
    add_library(
        ${SWAP_BUF_LIBRARY_STATIC} STATIC
        ${SWAP_BUF_SOURCES}
        )

    target_include_directories(${SWAP_BUF_LIBRARY_STATIC} PRIVATE ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR}/)
    target_link_libraries(${SWAP_BUF_LIBRARY_STATIC} PRIVATE ${Boost_LIBRARIES} PRIVATE ${ASYNC_FS_LIBRARY_STATIC})

    # set output name and path
    set_target_properties(
        ${SWAP_BUF_LIBRARY_STATIC}
        PROPERTIES
        OUTPUT_NAME ${SWAP_BUF_LIBRARY_NAME}
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
        )
endif()

