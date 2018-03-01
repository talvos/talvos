# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

foreach(test
  step
)
  set(TEST_NAME "interactive/${test}")
  add_test(
    NAME ${TEST_NAME}
    COMMAND
    ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run-test.py
    $<TARGET_FILE:talvos-cmd>
    ${CMAKE_CURRENT_SOURCE_DIR}/${test}.tcf
    ${CMAKE_CURRENT_SOURCE_DIR}/${test}.stdin
  )

  # Enable interactive mode.
  set_tests_properties(${TEST_NAME} PROPERTIES
                       ENVIRONMENT "TALVOS_INTERACTIVE=1")
endforeach(${test})
