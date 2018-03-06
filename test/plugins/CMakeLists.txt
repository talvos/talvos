# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

foreach(test
  callbacks
  missing-create
)
  # Build plugin library.
  set(TEST_LIB_NAME "${test}-test")
  add_library(${TEST_LIB_NAME} MODULE ${test}.cpp)
  target_link_libraries(${TEST_LIB_NAME} talvos)

  # Add test.
  set(TEST_NAME "plugins/${test}")
  add_test(
    NAME ${TEST_NAME}
    COMMAND
    ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run-test.py
    $<TARGET_FILE:talvos-cmd>
    ${CMAKE_CURRENT_SOURCE_DIR}/${test}.tcf
  )

  # Load plugin for test.
  set_tests_properties(
    ${TEST_NAME} PROPERTIES
    ENVIRONMENT "TALVOS_PLUGINS=$<TARGET_FILE:${TEST_LIB_NAME}>"
  )
endforeach(${test})

# Add test for missing plugin library.
set(TEST_NAME "plugins/missing-library")
add_test(
  NAME ${TEST_NAME}
  COMMAND
  ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run-test.py
  $<TARGET_FILE:talvos-cmd>
  ${CMAKE_CURRENT_SOURCE_DIR}/missing-library.tcf
)
set_tests_properties(
  ${TEST_NAME} PROPERTIES
  ENVIRONMENT "TALVOS_PLUGINS=libmissing-library.so"
)
