// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <iostream>

#include "CommandInvocation.h"

using namespace std;

bool CommandInvocation::load(const char *FileName)
{
  // Open config file
  ConfigFile.open(FileName);
  if (ConfigFile.fail())
  {
    cerr << "Unable to open config file '" << FileName << "'" << endl;
    return false;
  }

  // Load SPIR-V module
  string SPVFileName;
  getline(ConfigFile, SPVFileName);
  // TODO: Actually load it
  cout << "Loading " << SPVFileName << endl;

  // TODO: Parse remainder of config file

  return true;
}

void CommandInvocation::run()
{
  // TODO: Implement
}
