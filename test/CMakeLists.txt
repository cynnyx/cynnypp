#
# Tests configuration
#

set(CYNNYPP_SRC_DIR ${CMAKE_SOURCE_DIR}/${PROJECT_SRC_DIR})

# list all source files inside the src dir
file(GLOB_RECURSE COMMON_TEST_SOURCES ${CYNNYPP_SRC_DIR}/*.cpp)

message("Test sources: ${COMMON_TEST_SOURCES}")

# Common shared sources
set(COMMON_TEST_LIBRARY test_library)
add_library(${COMMON_TEST_LIBRARY} STATIC ${COMMON_TEST_SOURCES})

# include directories for the static lib
target_include_directories(
        ${COMMON_TEST_LIBRARY}
        PRIVATE ${Boost_INCLUDE_DIRS}
        PRIVATE ${CYNNYPP_SRC_DIR}
)

# library dependency for the static lib
target_link_libraries(
        ${COMMON_TEST_LIBRARY}
        PRIVATE ${Boost_LIBRARIES}
        PRIVATE ${CMAKE_THREAD_LIBS_INIT}
)


# list all source files inside the test dir
file(GLOB_RECURSE TARGET_TEST_ALL_SOURCES *.cpp)
message("Tests: ${TARGET_TEST_ALL_SOURCES}") 

#test ALL
set(TARGET_TEST_ALL test_all)
add_executable(${CYNNYPP_TEST_ALL} ${TARGET_TEST_ALL_SOURCES})



# library dependency for the testall target
target_link_libraries(
        ${TARGET_TEST_ALL}
        ${COMMON_TEST_LIBRARY}
)

message("Tests: ${Catch_INCLUDE_DIRS}")
# include directories for the testall target
target_include_directories(
        ${TARGET_TEST_ALL}
        PRIVATE ${CYNNYPP_SRC_DIR}
        PRIVATE ${Catch_INCLUDE_DIRS}
)

# add test_all to the list of test to be executed by CTest
add_test(NAME ${TARGET_TEST_ALL} COMMAND ${TARGET_TEST_ALL})
