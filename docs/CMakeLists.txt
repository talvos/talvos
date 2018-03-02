# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(Doxyfile.in Doxyfile)
  add_custom_target(doxygen COMMAND ${DOXYGEN_EXECUTABLE} VERBATIM)
endif()
