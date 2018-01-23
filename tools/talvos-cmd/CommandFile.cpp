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
  if (Buffers.count(Name))
    throw "duplicate allocation name";

  string Type = get<string>("allocation type");
  if (Type == "BUFFER")
  {
    size_t NumBytes = get<size_t>("allocation size");

    // Allocate buffer.
    size_t Address = Device->getGlobalMemory()->allocate(NumBytes);
    Buffers[Name] = {Address, NumBytes};

    // Process initializer.
    string Init = get<string>("allocation initializer");
    if (Init == "FILL")
    {
      string InitType = get<string>("fill type");
      if (InitType == "INT8")
        fill<int8_t>(Address, NumBytes);
      else if (InitType == "UINT8")
        fill<uint8_t>(Address, NumBytes);
      else if (InitType == "INT16")
        fill<int16_t>(Address, NumBytes);
      else if (InitType == "UINT16")
        fill<uint16_t>(Address, NumBytes);
      else if (InitType == "INT32")
        fill<int32_t>(Address, NumBytes);
      else if (InitType == "UINT32")
        fill<uint32_t>(Address, NumBytes);
      else if (InitType == "INT64")
        fill<int64_t>(Address, NumBytes);
      else if (InitType == "UINT64")
        fill<uint64_t>(Address, NumBytes);
      else if (InitType == "FLOAT")
        fill<float>(Address, NumBytes);
      else if (InitType == "DOUBLE")
        fill<double>(Address, NumBytes);
      else
        throw NotRecognizedException();
    }
    else if (Init == "RANGE")
    {
      string InitType = get<string>("range type");
      if (InitType == "INT8")
        range<int8_t>(Address, NumBytes);
      else if (InitType == "UINT8")
        range<uint8_t>(Address, NumBytes);
      else if (InitType == "INT16")
        range<int16_t>(Address, NumBytes);
      else if (InitType == "UINT16")
        range<uint16_t>(Address, NumBytes);
      else if (InitType == "INT32")
        range<int32_t>(Address, NumBytes);
      else if (InitType == "UINT32")
        range<uint32_t>(Address, NumBytes);
      else if (InitType == "INT64")
        range<int64_t>(Address, NumBytes);
      else if (InitType == "UINT64")
        range<uint64_t>(Address, NumBytes);
      else if (InitType == "FLOAT")
        range<float>(Address, NumBytes);
      else if (InitType == "DOUBLE")
        range<double>(Address, NumBytes);
      else
        throw NotRecognizedException();
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
  if (!Buffers.count(Name))
    throw "invalid resource identifier";

  DescriptorSet[{Set, Binding}] = Buffers[Name].first;
}

void CommandFile::parseDispatch()
{
  if (!Module)
    throw "DISPATCH reached with no prior MODULE command";

  uint32_t GroupsCountX = get<uint32_t>("group count X");
  uint32_t GroupsCountY = get<uint32_t>("group count Y");
  uint32_t GroupsCountZ = get<uint32_t>("group count Z");
  talvos::DispatchCommand Command(Device, Module.get(), Module->getFunction(),
                                  GroupsCountX, GroupsCountY, GroupsCountZ,
                                  DescriptorSet);
  Command.run();
}

void CommandFile::parseDump()
{
  string DumpType = get<string>("dump type");
  if (DumpType == "INT8")
    dump<int8_t>();
  else if (DumpType == "UINT8")
    dump<uint8_t>();
  else if (DumpType == "INT16")
    dump<int16_t>();
  else if (DumpType == "UINT16")
    dump<uint16_t>();
  else if (DumpType == "INT32")
    dump<int32_t>();
  else if (DumpType == "UINT32")
    dump<uint32_t>();
  else if (DumpType == "INT64")
    dump<int64_t>();
  else if (DumpType == "UINT64")
    dump<uint64_t>();
  else if (DumpType == "FLOAT")
    dump<float>();
  else if (DumpType == "DOUBLE")
    dump<double>();
  else if (DumpType == "ALL")
    Device->getGlobalMemory()->dump();
  else if (DumpType == "RAW")
  {
    string Name = get<string>("allocation name");
    if (!Buffers.count(Name))
      throw "invalid resource identifier";
    Device->getGlobalMemory()->dump(Buffers.at(Name).first);
  }
  else
    throw NotRecognizedException();
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

template <typename T> void CommandFile::dump()
{
  string Name = get<string>("allocation name");
  if (!Buffers.count(Name))
    throw "invalid resource identifier";

  size_t Address = Buffers.at(Name).first;
  size_t NumBytes = Buffers.at(Name).second;

  std::cout << std::endl
            << "Buffer '" << Name << "' (" << NumBytes
            << " bytes):" << std::endl;
  for (int i = 0; i < NumBytes / sizeof(T); i++)
  {
    T Value;
    Device->getGlobalMemory()->load((uint8_t *)&Value, Address + i * sizeof(T),
                                    sizeof(T));
    std::cout << "  " << Name << "[" << i << "] = " << Value << std::endl;
  }
}

template <typename T> void CommandFile::fill(size_t Address, size_t NumBytes)
{
  T FillValue = get<T>("fill value");
  for (int i = 0; i < NumBytes; i += sizeof(FillValue))
    Device->getGlobalMemory()->store(Address + i, sizeof(FillValue),
                                     (uint8_t *)&FillValue);
}

template <typename T> void CommandFile::range(size_t Address, size_t NumBytes)
{
  T Value = get<T>("range start");
  T RangeInc = get<T>("range inc");
  for (int i = 0; i < NumBytes; i += sizeof(Value), Value += RangeInc)
    Device->getGlobalMemory()->store(Address + i, sizeof(Value),
                                     (uint8_t *)&Value);
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
    return false;
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
      {
        std::cerr << "Unexpected EOF while parsing " << CurrentParseAction
                  << std::endl;
        return false;
      }
    }
    else
    {
      std::cerr << "Failed to parse " << CurrentParseAction << std::endl;
      return false;
    }
  }

  return true;
}
