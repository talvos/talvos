// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_DEVICE_H
#define TALVOS_DEVICE_H

namespace talvos
{

class Memory;

/// A Device instance encapsulates properties and state for the virtual device.
class Device
{
public:
  Device();
  ~Device();

  // Do not allow Device objects to be copied.
  ///\{
  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;
  ///\}

  /// Get the global memory instance associated with this device.
  Memory &getGlobalMemory() { return *GlobalMemory; }

private:
  Memory *GlobalMemory; ///< The global memory of this device.
};

} // namespace talvos

#endif
