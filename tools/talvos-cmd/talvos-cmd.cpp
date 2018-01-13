// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

static const char *ConfigFileName;

static bool parseArguments(int argc, char *argv[]);
static void printUsage();

int main(int argc, char *argv[])
{
  if (!parseArguments(argc, argv))
    return 1;

  // TODO: Process config file
  cout << ConfigFileName << endl;

  return 0;
}

static bool parseArguments(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++)
  {
    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
    {
      printUsage();
      exit(0);
    }
    else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
    {
      cout << endl;
      // TODO: Add proper version number
      cout << "Talvos XXX" << endl;
      cout << endl;
      cout << "Copyright (c) 2018 the Talvos developers" << endl;
      cout << "https://github.com/jrprice/talvos" << endl;
      cout << endl;
      exit(0);
    }
    else if (argv[i][0] == '-')
    {
      cerr << "Unrecognised option '" << argv[i] << "'" << endl;
      return false;
    }
    else
    {
      if (ConfigFileName == NULL)
      {
        ConfigFileName = argv[i];
      }
      else
      {
        cerr << "Unexpected positional argument '" << argv[i] << "'" << endl;
        return false;
      }
    }
  }

  // Ensure a configuration file has been specified
  if (ConfigFileName == NULL)
  {
    printUsage();
    return false;
  }

  return true;
}

static void printUsage()
{
  cout << "Usage: talvos-cmd [OPTIONS] CONFIG" << endl;
  cout << "       talvos-cmd [--help | --version]" << endl;
  cout << endl;
  cout << "Options:" << endl;
  cout << "  -h --help                    "
          "Display usage information"
       << endl;
  cout << "  -v --version                 "
          "Display version information"
       << endl;
  // TODO: Add a link to some online documentation somewhere
  // cout << endl;
  // cout << "For more information, please visit the Talvos documentation:"
  //      << endl;
  // cout << "-> http://TODO" << endl
  cout << endl;
}
