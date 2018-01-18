// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_INTERPRETER_H
#define TALVOS_INTERPRETER_H

namespace talvos
{

struct Function;
class Module;

void interpret(const Module *M, const Function *F);

} // namespace talvos

#endif
