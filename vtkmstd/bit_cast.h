//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_std_bit_cast_h
#define vtk_m_std_bit_cast_h

#include <cstring>
#include <type_traits>

namespace vtkmstd
{
// Copy/Paste from cppreference.com
template <class To, class From>
typename std::enable_if<sizeof(To) == sizeof(From) && std::is_trivially_copyable<From>::value &&
                          std::is_trivially_copyable<To>::value,
                        To>::type
// constexpr support needs compiler magic
bit_cast(const From& src) noexcept
{
  static_assert(
    std::is_trivially_constructible<To>::value,
    "This implementation additionally requires destination type to be trivially constructible");

  To dst;
  std::memcpy(&dst, &src, sizeof(To));
  return dst;
}
}
#endif //vtk_m_std_bit_cast_h
