# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

foreach(test
  dispatch-without-module
  duplicate-allocation-name
  empty
)
  add_test(
    NAME ${test}
    COMMAND
    ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/test/run-test.py
    ${CMAKE_CURRENT_SOURCE_DIR}/${test}.tcf
    $<TARGET_FILE:talvos-cmd>)
endforeach(${test})
