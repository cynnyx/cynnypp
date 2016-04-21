#
# CYNNYPP executable configuration
#

#set(CYNNYPP_SRC_DIR .)

#set(
#    CYNNYPP_SOURCES
#    main.cpp
#    file_system/fs_manager.cpp
#    utilities/locales.cpp
#    utilities/event.cpp)

#add_library(
#    ${TARGET_CYNNYPP}
#    ${CYNNYPP_SOURCES})

#target_include_directories(
#    ${TARGET_CYNNYPP}
#    PRIVATE ${CYNNYPP_SRC_DIR}
#    PRIVATE ${Boost_INCLUDE_DIRS}
#)
#target_link_libraries(
#    ${TARGET_CYNNYPP}
#    PRIVATE ${Boost_LIBRARIES}
#)

#install(
#    TARGETS ${TARGET_CYNNYPP}
#    LIBRARY DESTINATION ${PROJECT_LIBRARY_OUTPUT_DIRECTORY}
#    ARCHIVE DESTINATION ${PROJECT_ARCHIVE_OUTPUT_DIRECTORY}
#)


add_subdirectory(io/async/fs)
