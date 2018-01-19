// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_DEVICE_H
#define TALVOS_DEVICE_H

namespace talvos
{

class Memory;

class Device
{
public:
  Device();
  ~Device();
  Memory *getGlobalMemory() { return GlobalMemory; }

private:
  Memory *GlobalMemory;
};

} // namespace talvos

#endif
