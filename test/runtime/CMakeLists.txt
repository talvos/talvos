# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

# Common runtime code and utils.
add_library(test_runtime_common OBJECT common.cpp)

foreach(test
  vecadd
)
  # Build test executable.
  set(TEST_EXE "${test}-test")
  add_executable(${TEST_EXE} ${test}.cpp)
  target_link_libraries(${TEST_EXE}
                        $<TARGET_OBJECTS:test_runtime_common>
                        talvos-vulkan)

  # Add test.
  set(TEST_NAME "runtime/${test}")
  add_test(
    NAME ${TEST_NAME}
    COMMAND
    $<TARGET_FILE:${TEST_EXE}>
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  if ("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows")
    set_tests_properties(
      ${TEST_NAME} PROPERTIES
      ENVIRONMENT "PATH=$<TARGET_FILE_DIR:talvos-vulkan>;$ENV{PATH}"
    )
  endif()

endforeach(${test})
