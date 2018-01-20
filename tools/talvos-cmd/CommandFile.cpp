// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <iostream>

#include "CommandFile.h"
#include "talvos/Device.h"
#include "talvos/DispatchCommand.h"
#include "talvos/Memory.h"
#include "talvos/Module.h"

using namespace std;

CommandFile::CommandFile(std::istream &Stream) : Stream(Stream) {}

CommandFile::~CommandFile() {}

bool CommandFile::run()
{
  talvos::Device *Dev = new talvos::Device;

  // Load SPIR-V module.
  string SPVFileName;
  getline(Stream, SPVFileName);
  Module = talvos::Module::load(SPVFileName);
  if (!Module)
  {
    cerr << "Failed to load SPIR-V module" << endl;
    return false;
  }

  // TODO: Parse remainder of config file

  // TODO: Problem domain
  talvos::DispatchCommand Command(Dev, Module.get(), Module->getFunction());
  Command.run();

  Dev->getGlobalMemory()->dump();

  delete Dev;

  return true;
}
