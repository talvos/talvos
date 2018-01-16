// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_RESULT_H
#define TALVOS_RESULT_H

#include <cassert>
#include <cstdint>
#include <cstring>

namespace talvos
{

class Result
{
public:
  Result() { Data = nullptr; }

  template <typename T> static Result create(T Value)
  {
    Result R;
    R.Data = new uint8_t[sizeof(T)];
    *((T *)R.Data) = Value;
    return R;
  }

  Result clone() const
  {
    Result R;
    // TODO: Use size of this result
    R.Data = new uint8_t[4];
    memcpy(R.Data, this->Data, 4);
    return R;
  }

  void destroy() { delete[] Data; }

  template <typename T> T get() const
  {
    // TODO: assert size
    assert(Data);
    return *((T *)Data);
  }

  bool isSet() const { return Data ? true : false; };

private:
  // TODO: Type, size, etc
  uint8_t *Data;
};

} // namespace talvos

#endif
