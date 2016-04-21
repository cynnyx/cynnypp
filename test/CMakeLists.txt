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
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.54 REQUIRED COMPONENTS system filesystem)


#
# Tests configuration
#

set(CYNNYPP_SRC_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR})

# list all source files inside the src dir
file(GLOB_RECURSE COMMON_TEST_SOURCES ${CYNNYPP_SRC_DIR}/*.cpp)

# Common shared sources
set(COMMON_TEST_LIBRARY test_library)
add_library(${COMMON_TEST_LIBRARY} STATIC ${COMMON_TEST_SOURCES})

# include directories for the static lib
target_include_directories(
        ${COMMON_TEST_LIBRARY}
        PRIVATE ${CYNNYPP_SRC_DIR}
)

target_link_libraries(
        ${COMMON_TEST_LIBRARY}
        PRIVATE ${Boost_LIBRARIES}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
)


# list all source files inside the test dir
file(GLOB_RECURSE TARGET_TEST_ALL_SOURCES *.cpp)

#test ALL
set(TARGET_TEST_ALL test_all)
add_executable(${CYNNYPP_TEST_ALL} ${TARGET_TEST_ALL_SOURCES})


# library dependency for the testall target
target_link_libraries(
        ${TARGET_TEST_ALL}
        PRIVATE ${Boost_LIBRARIES}
        ${COMMON_TEST_LIBRARY}
)

# include directories for the testall target
target_include_directories(
        ${TARGET_TEST_ALL}
        PRIVATE ${CYNNYPP_SRC_DIR}
        PRIVATE ${Catch_INCLUDE_DIRS}
)

# add test_all to the list of test to be executed by CTest
add_test(NAME ${TARGET_TEST_ALL} COMMAND ${TARGET_TEST_ALL})
