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

class NotRecognizedException : exception
{
};

CommandFile::CommandFile(std::istream &Stream) : Stream(Stream)
{
  Device = new talvos::Device;
}

CommandFile::~CommandFile() { delete Device; }

template <typename T> T CommandFile::get(const char *ParseAction)
{
  CurrentParseAction = ParseAction;
  while (true)
  {
    // Note the current stream position.
    streampos Pos = Stream.tellg();

    // Try and read a token.
    string Token;
    Stream >> Token;
    if (Stream.fail())
      throw Stream.rdstate();

    // Check for comments.
    if (Token[0] == '#')
    {
      // Consume rest of line.
      getline(Stream, Token);
      continue;
    }

    // Rewind and read token of desired type.
    T Result;
    Stream.seekg(Pos);
    Stream >> Result;
    if (Stream.fail())
      throw Stream.rdstate();

    return Result;
  }
}

void CommandFile::parseAllocate()
{
  string Name = get<string>("allocation name");
  string Type = get<string>("allocation type");
  if (Type == "BUFFER")
  {
    size_t NumBytes = get<size_t>("allocation size");

    // TODO: Allocate buffer.
    std::cout << "Allocating buffer of size " << NumBytes << std::endl;

    // Process initializer.
    string Init = get<string>("allocation initializer");
    if (Init == "FILL")
    {
      string InitType = get<string>("fill type");
      string FillValue = get<string>("fill value");

      // TODO: Handle this
      std::cout << "Filling buffer with " << InitType << " value " << FillValue
                << std::endl;
    }
    else if (Init == "SERIES")
    {
      string InitType = get<string>("series type");
      string SeriesStart = get<string>("series start value");
      string SeriesInc = get<string>("series increment value");

      // TODO: Handle this
      std::cout << "Filling buffer with " << InitType << " series "
                << SeriesStart << " +" << SeriesInc << std::endl;
    }
    else
    {
      throw NotRecognizedException();
    }
  }
  else
  {
    throw NotRecognizedException();
  }
}

void CommandFile::parseDescriptorSet()
{
  size_t Set = get<size_t>("descriptor set");
  size_t Binding = get<size_t>("binding");
  string Name = get<string>("resource name");

  // TODO: Handle this
  std::cout << "Binding " << Name << " to " << Set << ":" << Binding
            << std::endl;
}

void CommandFile::parseDispatch()
{
  size_t NumGroupsX = get<size_t>("dispatch size X");
  size_t NumGroupsY = get<size_t>("dispatch size Y");
  size_t NumGroupsZ = get<size_t>("dispatch size Z");

  // TODO: Problem domain
  std::cout << "Dispatch " << NumGroupsX << " x " << NumGroupsY << " x "
            << NumGroupsZ << " groups" << std::endl;

  talvos::DispatchCommand Command(Device, Module.get(), Module->getFunction());
  Command.run();
}

void CommandFile::parseDump()
{
  string Name = get<string>("allocation name");
  // TODO: Dump specific buffer
  Device->getGlobalMemory()->dump();
}

void CommandFile::parseEntry()
{
  string Entry = get<string>("entry name");

  // TODO: Handle this
  std::cout << "Using entry point " << Entry << std::endl;
}

void CommandFile::parseModule()
{
  // Load SPIR-V module.
  string SPVFileName = get<string>("module filename");
  Module = talvos::Module::load(SPVFileName);
  if (!Module)
    throw "failed to load SPIR-V module";
}

bool CommandFile::run()
{
  try
  {
    while (Stream.good())
    {
      // Parse command.
      string Command = get<string>("command");
      if (Command == "ALLOCATE")
        parseAllocate();
      else if (Command == "DESCRIPTOR_SET")
        parseDescriptorSet();
      else if (Command == "DISPATCH")
        parseDispatch();
      else if (Command == "DUMP")
        parseDump();
      else if (Command == "ENTRY")
        parseEntry();
      else if (Command == "MODULE")
        parseModule();
      else
      {
        std::cerr << "Unrecognized command '" << Command << "'" << std::endl;
        return false;
      }
    }
  }
  // TODO: Add line numbers to error messages
  catch (NotRecognizedException e)
  {
    std::cerr << "ERROR: unrecognized " << CurrentParseAction << std::endl;
  }
  catch (const char *err)
  {
    std::cerr << "ERROR: " << err << std::endl;
    return false;
  }
  catch (ifstream::iostate e)
  {
    if (e & istream::eofbit)
    {
      // EOF is only OK if we're parsing the next command.
      if (CurrentParseAction != "command")
        std::cerr << "Unexpected EOF while parsing " << CurrentParseAction
                  << std::endl;
    }
    else
    {
      std::cerr << "Failed to parse " << CurrentParseAction << std::endl;
    }
  }

  return true;
}
