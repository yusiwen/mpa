cmake_minimum_required(VERSION 3.6)
project(mpa)

file (STRINGS "../../VERSION" BUILD_VERSION_ROOT)

include($ENV{PAYMENT_BUILD_PROFILE})
include($ENV{PAYMENT_BUILD_CONFIG})

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
set(INC_DIR "${PROJECT_SOURCE_DIR}/include")
set(TEST_SRC_DIR "${PROJECT_SOURCE_DIR}/tests")
set(TOOL_SRC_DIR "${PROJECT_SOURCE_DIR}/tools")

file(GLOB MPA_SRC_FILES "${SRC_DIR}/*.c")

include_directories("${INC_DIR}")
include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${COMMON_INC_DIR}")

add_library(mpa SHARED ${MPA_SRC_FILES})
set_property(TARGET mpa PROPERTY VERSION "${BUILD_VERSION}")
set_property(TARGET mpa PROPERTY SOVERSION "${VERSION_MAJOR}")
target_link_libraries(mpa rscom)

file(GLOB TEST_SRC_FILES "${TEST_SRC_DIR}/*.c")
foreach(test_source_file ${TEST_SRC_FILES})
  # Used a simple string replace, to cut off .c
  get_filename_component(target_name ${test_source_file} NAME_WE)
  add_executable( ${target_name} ${test_source_file} )
  # Make sure librscom is linked to each app
  target_link_libraries( ${target_name} rscom mpa m)
endforeach(test_source_file ${TEST_SRC_FILES})

file(GLOB TOOL_SRC_FILES "${TOOL_SRC_DIR}/*.c")
foreach(tool_source_file ${TOOL_SRC_FILES})
  # Used a simple string replace, to cut off .c
  get_filename_component(target_name ${tool_source_file} NAME_WE)
  add_executable(${target_name} ${tool_source_file})
  # Make sure librscom is linked to each app
  target_link_libraries(${target_name} rscom mpa m)
  INSTALL(TARGETS ${target_name} DESTINATION ${TOOL_DIR})
endforeach(tool_source_file ${TOOL_SRC_FILES})

INSTALL(TARGETS mpa DESTINATION ${LIB_DIR})
