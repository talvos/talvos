# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(Doxyfile.in Doxyfile)
  add_custom_target(doxygen COMMAND ${DOXYGEN_EXECUTABLE} VERBATIM)
endif()

find_program(SPHINX_EXECUTABLE NAMES sphinx-build
             DOC "Sphinx documentation generator")
if (SPHINX_EXECUTABLE)
  add_custom_target(sphinx VERBATIM
                    COMMAND ${SPHINX_EXECUTABLE}
                    ${CMAKE_CURRENT_SOURCE_DIR}/sphinx
                    ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
endif()
