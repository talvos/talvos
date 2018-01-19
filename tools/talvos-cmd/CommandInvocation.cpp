// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <iostream>

#include "CommandInvocation.h"
#include "talvos/Device.h"
#include "talvos/DispatchCommand.h"
#include "talvos/Module.h"

using namespace std;

CommandInvocation::CommandInvocation() {}

CommandInvocation::~CommandInvocation() {}

bool CommandInvocation::load(const char *FileName)
{
  // Open config file.
  ConfigFile.open(FileName);
  if (ConfigFile.fail())
  {
    cerr << "Unable to open config file '" << FileName << "'" << endl;
    return false;
  }

  // Load SPIR-V module.
  string SPVFileName;
  getline(ConfigFile, SPVFileName);
  Module = talvos::Module::load(SPVFileName);
  if (!Module)
  {
    cerr << "Failed to load SPIR-V module" << endl;
    return false;
  }

  // TODO: Parse remainder of config file

  return true;
}

void CommandInvocation::run()
{
  talvos::Device *Dev = new talvos::Device;

  // TODO: Problem domain
  talvos::DispatchCommand Command(Dev, Module.get(), Module->getFunction());
  Command.run();

  delete Dev;
}
