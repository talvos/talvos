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
  CurrentLine = 1;
}

CommandFile::~CommandFile() { delete Device; }

template <typename T> T CommandFile::get(const char *ParseAction)
{
  CurrentParseAction = ParseAction;
  while (true)
  {
    // Note the current stream position.
    streampos Pos = Stream.tellg();

    // Skip leading whitespace and check for newlines.
    while (true)
    {
      int c = Stream.peek();
      if (!isspace(c))
        break;

      if (c == '\n')
        ++CurrentLine;
      Stream.get();
    }

    // Try to read a token.
    string Token;
    Stream >> Token;
    if (Stream.fail())
      throw Stream.rdstate();

    // Check for comments.
    if (Token[0] == '#')
    {
      // Consume rest of line.
      getline(Stream, Token);
      ++CurrentLine;
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

void CommandFile::parseBuffer()
{
  string Name = get<string>("buffer name");
  if (Buffers.count(Name))
    throw "duplicate buffer name";

  uint64_t NumBytes = get<uint64_t>("buffer size");

  // Allocate buffer.
  uint64_t Address = Device->getGlobalMemory().allocate(NumBytes);
  Buffers[Name] = {Address, NumBytes};

  // Process initializer.
  string Init = get<string>("buffer initializer");
  if (Init == "DATA")
  {
    string InitType = get<string>("data type");
    if (InitType == "INT8")
      data<int8_t>(Address, NumBytes);
    else if (InitType == "UINT8")
      data<uint8_t>(Address, NumBytes);
    else if (InitType == "INT16")
      data<int16_t>(Address, NumBytes);
    else if (InitType == "UINT16")
      data<uint16_t>(Address, NumBytes);
    else if (InitType == "INT32")
      data<int32_t>(Address, NumBytes);
    else if (InitType == "UINT32")
      data<uint32_t>(Address, NumBytes);
    else if (InitType == "INT64")
      data<int64_t>(Address, NumBytes);
    else if (InitType == "UINT64")
      data<uint64_t>(Address, NumBytes);
    else if (InitType == "FLOAT")
      data<float>(Address, NumBytes);
    else if (InitType == "DOUBLE")
      data<double>(Address, NumBytes);
    else
      throw NotRecognizedException();
  }
  else if (Init == "FILL")
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
  else if (Init == "SERIES")
  {
    string InitType = get<string>("series type");
    if (InitType == "INT8")
      series<int8_t>(Address, NumBytes);
    else if (InitType == "UINT8")
      series<uint8_t>(Address, NumBytes);
    else if (InitType == "INT16")
      series<int16_t>(Address, NumBytes);
    else if (InitType == "UINT16")
      series<uint16_t>(Address, NumBytes);
    else if (InitType == "INT32")
      series<int32_t>(Address, NumBytes);
    else if (InitType == "UINT32")
      series<uint32_t>(Address, NumBytes);
    else if (InitType == "INT64")
      series<int64_t>(Address, NumBytes);
    else if (InitType == "UINT64")
      series<uint64_t>(Address, NumBytes);
    else if (InitType == "FLOAT")
      series<float>(Address, NumBytes);
    else if (InitType == "DOUBLE")
      series<double>(Address, NumBytes);
    else
      throw NotRecognizedException();
  }
  else if (Init == "DATFILE")
  {
    // Open data file.
    string Filename = get<string>("data filename");
    std::ifstream DatFile(Filename, std::ios::binary);
    if (!DatFile)
      throw "unable to open file";

    // Load data from file.
    std::vector<char> Data(NumBytes);
    if (!DatFile.read(Data.data(), NumBytes))
      throw "failed to read binary data";

    // Copy data to buffer.
    Device->getGlobalMemory().store(Address, NumBytes, (uint8_t *)Data.data());
  }
  else
  {
    throw NotRecognizedException();
  }
}

void CommandFile::parseDescriptorSet()
{
  uint32_t Set = get<uint32_t>("descriptor set");
  uint32_t Binding = get<uint32_t>("binding");
  string Name = get<string>("resource name");
  if (!Buffers.count(Name))
    throw "invalid resource identifier";

  DescriptorSet[{Set, Binding}] = Buffers[Name].first;
}

void CommandFile::parseDispatch()
{
  if (!Module)
    throw "DISPATCH reached with no prior MODULE command";
  if (!Function)
    throw "DISPATCH reached with no prior ENTRY command";

  talvos::Dim3 GroupCount;
  GroupCount.X = get<uint32_t>("group count X");
  GroupCount.Y = get<uint32_t>("group count Y");
  GroupCount.Z = get<uint32_t>("group count Z");
  talvos::DispatchCommand Command(Device, Module.get(), Function, GroupCount,
                                  DescriptorSet);
  Command.run();
}

void CommandFile::parseDump()
{
  string DumpType = get<string>("dump type");

  unsigned VecWidth = 1;
  size_t VPos = DumpType.find('v');
  if (VPos != string::npos)
  {
    char *End;
    VecWidth = strtoul(DumpType.c_str() + VPos + 1, &End, 10);
    if (VecWidth == 0 || strlen(End))
      throw "invalid vector suffix";
    DumpType = DumpType.substr(0, VPos);
  }

  if (DumpType == "INT8")
    dump<int8_t>(VecWidth);
  else if (DumpType == "UINT8")
    dump<uint8_t>(VecWidth);
  else if (DumpType == "INT16")
    dump<int16_t>(VecWidth);
  else if (DumpType == "UINT16")
    dump<uint16_t>(VecWidth);
  else if (DumpType == "INT32")
    dump<int32_t>(VecWidth);
  else if (DumpType == "UINT32")
    dump<uint32_t>(VecWidth);
  else if (DumpType == "INT64")
    dump<int64_t>(VecWidth);
  else if (DumpType == "UINT64")
    dump<uint64_t>(VecWidth);
  else if (DumpType == "FLOAT")
    dump<float>(VecWidth);
  else if (DumpType == "DOUBLE")
    dump<double>(VecWidth);
  else if (DumpType == "ALL")
    Device->getGlobalMemory().dump();
  else if (DumpType == "RAW")
  {
    string Name = get<string>("allocation name");
    if (!Buffers.count(Name))
      throw "invalid resource identifier";
    Device->getGlobalMemory().dump(Buffers.at(Name).first);
  }
  else
    throw NotRecognizedException();
}

void CommandFile::parseEndLoop()
{
  if (Loops.empty())
    throw "ENDLOOP without matching LOOP";

  if (--Loops.back().first)
    Stream.seekg(Loops.back().second);
  else
    Loops.pop_back();
}

void CommandFile::parseEntry()
{
  string Name = get<string>("entry name");
  Function = Module->getEntryPoint(Name);
  if (!Function)
    throw "invalid entry point";
}

void CommandFile::parseLoop()
{
  size_t Count = get<size_t>("loop count");
  if (Count == 0)
    throw "loop count must be > 0";
  Loops.push_back({Count, Stream.tellg()});
}

void CommandFile::parseModule()
{
  // Load SPIR-V module.
  string SPVFileName = get<string>("module filename");
  Module = talvos::Module::load(SPVFileName);
  if (!Module)
    throw "failed to load SPIR-V module";
}

template <typename T> void CommandFile::dump(unsigned VecWidth)
{
  string Name = get<string>("allocation name");
  if (!Buffers.count(Name))
    throw "invalid resource identifier";

  uint64_t Address = Buffers.at(Name).first;
  uint64_t NumBytes = Buffers.at(Name).second;

  // 3-element vectors are padded to 4-elements in buffers.
  if (VecWidth == 3)
    VecWidth = 4;

  std::cout << std::endl
            << "Buffer '" << Name << "' (" << NumBytes
            << " bytes):" << std::endl;
  for (uint64_t i = 0; i < NumBytes / sizeof(T); i += VecWidth)
  {
    std::cout << "  " << Name << "[" << (i / VecWidth) << "] = ";

    if (VecWidth > 1)
      std::cout << "(";
    for (unsigned v = 0; v < VecWidth; v++)
    {
      if (v > 0)
        std::cout << ", ";

      if (i + v >= NumBytes / sizeof(T))
        break;

      T Value;
      Device->getGlobalMemory().load((uint8_t *)&Value,
                                     Address + (i + v) * sizeof(T), sizeof(T));
      std::cout << Value;
    }
    if (VecWidth > 1)
      std::cout << ")";

    std::cout << std::endl;
  }
}

template <typename T>
void CommandFile::data(uint64_t Address, uint64_t NumBytes)
{
  for (uint64_t i = 0; i < NumBytes; i += sizeof(T))
  {
    T Value = get<T>("data value");
    Device->getGlobalMemory().store(Address + i, sizeof(Value),
                                    (uint8_t *)&Value);
  }
}

template <typename T>
void CommandFile::fill(uint64_t Address, uint64_t NumBytes)
{
  T FillValue = get<T>("fill value");
  for (uint64_t i = 0; i < NumBytes; i += sizeof(FillValue))
    Device->getGlobalMemory().store(Address + i, sizeof(FillValue),
                                    (uint8_t *)&FillValue);
}

template <typename T>
void CommandFile::series(uint64_t Address, uint64_t NumBytes)
{
  T Value = get<T>("series start");
  T RangeInc = get<T>("series inc");
  for (uint64_t i = 0; i < NumBytes; i += sizeof(Value), Value += RangeInc)
    Device->getGlobalMemory().store(Address + i, sizeof(Value),
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
      if (Command == "BUFFER")
        parseBuffer();
      else if (Command == "DESCRIPTOR_SET")
        parseDescriptorSet();
      else if (Command == "DISPATCH")
        parseDispatch();
      else if (Command == "DUMP")
        parseDump();
      else if (Command == "ENDLOOP")
        parseEndLoop();
      else if (Command == "ENTRY")
        parseEntry();
      else if (Command == "LOOP")
        parseLoop();
      else if (Command == "MODULE")
        parseModule();
      else
      {
        std::cerr << "line " << CurrentLine << ": ";
        std::cerr << "Unrecognized command '" << Command << "'" << std::endl;
        return false;
      }
    }
  }
  // TODO: Add line numbers to error messages
  catch (NotRecognizedException e)
  {
    std::cerr << "line " << CurrentLine << ": ";
    std::cerr << "ERROR: unrecognized " << CurrentParseAction << std::endl;
    return false;
  }
  catch (const char *err)
  {
    std::cerr << "line " << CurrentLine << ": ";
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
        std::cerr << "line " << CurrentLine << ": ";
        std::cerr << "Unexpected EOF while parsing " << CurrentParseAction
                  << std::endl;
        return false;
      }
      if (!Loops.empty())
      {
        std::cerr << "line " << CurrentLine << ": ";
        std::cerr << "ERROR: Unterminated LOOP" << std::endl;
        return false;
      }
    }
    else
    {
      std::cerr << "line " << CurrentLine << ": ";
      std::cerr << "Failed to parse " << CurrentParseAction << std::endl;
      return false;
    }
  }

  return true;
}
