# Copyright (c) 2018 the Talvos developers. All rights reserved.
#
# This file is distributed under a three-clause BSD license. For full license
# terms please see the LICENSE file distributed with this source code.

add_executable(talvos-cmd talvos-cmd.cpp
               CommandFile.cpp CommandFile.h)
target_link_libraries(talvos-cmd talvos)

install(TARGETS talvos-cmd DESTINATION bin)
