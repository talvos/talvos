// Copyright (c) 2018 the Talvos developers. All rights reserved.
//
// This file is distributed under a three-clause BSD license. For full license
// terms please see the LICENSE file distributed with this source code.

#ifndef TALVOS_RESULT_H
#define TALVOS_RESULT_H

#include <cassert>
#include <cstdint>

namespace talvos
{

class Result
{
public:
  Result() { Data = nullptr; }

  template <typename T> T get()
  {
    // TODO: assert size
    assert(Data);
    return *((T *)Data);
  }

  template <typename T> static Result create(T Value)
  {
    Result R;
    R.Data = new uint8_t[sizeof(T)];
    *((T *)R.Data) = Value;
    return R;
  }

  void destroy() { delete[] Data; }

private:
  // TODO: Type, size, etc
  uint8_t *Data;
};

} // namespace talvos

#endif
