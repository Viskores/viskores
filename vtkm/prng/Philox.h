//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_prng_Philox_h
#define vtk_m_prng_Philox_h

#include <vtkm/Types.h>

namespace vtkm
{
namespace prng
{
namespace detail
{
VTKM_EXEC_CONT vtkm::Vec<vtkm::UInt32, 2> mulhilo(vtkm::UInt32 a, vtkm::UInt32 b)
{
  vtkm::UInt64 r = static_cast<vtkm::UInt64>(a) * b;
  auto lo = static_cast<vtkm::UInt32>(r);
  vtkm::UInt32 hi = r >> 32;
  return { lo, hi };
}

#if 0
// FIXME: what to do with CUDA backend?
constexpr VTKM_EXEC_CONT vtkm::Vec<vtkm::UInt64, 2> mulhilo(vtkm::UInt64 a, vtkm::UInt64 b)
{
  __uint128_t r = static_cast<__uint128_t>(a) * static_cast<__uint128_t>(b);
  vtkm::UInt64 lo = static_cast<vtkm::UInt64>(r);
  vtkm::UInt64 hi = r >> 64;
  return { lo, hi };
}
#endif

template <typename UIntType, std::size_t N, UIntType... consts>
class philox_parameters;

template <typename T, T M0, T C0>
struct philox_parameters<T, 2, M0, C0>
{
  static const vtkm::Vec<T, 1> multipliers;
  static const vtkm::Vec<T, 1> round_consts;
};

template <typename T, T M0, T C0>
const vtkm::Vec<T, 1> vtkm::prng::detail::philox_parameters<T, 2, M0, C0>::multipliers =
  vtkm::Vec<T, 1>(M0);
template <typename T, T M0, T C0>
const vtkm::Vec<T, 1> vtkm::prng::detail::philox_parameters<T, 2, M0, C0>::round_consts =
  vtkm::Vec<T, 1>(C0);

// TODO: make it work with C++11
template <typename T, T M0, T C0, T M1, T C1>
struct philox_parameters<T, 4, M0, C0, M1, C1>
{
  static const vtkm::Vec<T, 2> multipliers;
  static const vtkm::Vec<T, 2> round_consts;
};

template <typename T, T M0, T C0, T M1, T C1>
const vtkm::Vec<T, 1> vtkm::prng::detail::philox_parameters<T, 4, M0, C0, M1, C1>::multipliers =
  vtkm::Vec<T, 2>(M0, M1);
template <typename T, T M0, T C0, T M1, T C1>
const vtkm::Vec<T, 1> vtkm::prng::detail::philox_parameters<T, 4, M0, C0, M1, C1>::round_consts =
  vtkm::Vec<T, 2>(C0, C1);

template <typename UIntType, std::size_t N, std::size_t R, UIntType... consts>
class philox_functor;

template <typename UIntType, std::size_t R, UIntType... consts>
class philox_functor<UIntType, 2, R, consts...>
{
public:
  using counters_type = vtkm::Vec<UIntType, 2>;
  using keys_type = vtkm::Vec<UIntType, 1>;

  VTKM_EXEC_CONT counters_type operator()(counters_type counters, keys_type keys) const
  {
    for (std::size_t i = 0; i < R; ++i)
    {
      counters = round(counters, keys);
      keys = bump_keys(keys);
    }
    return counters;
  }

private:
  static VTKM_EXEC_CONT counters_type round(counters_type counters, keys_type round_keys)
  {
    vtkm::Vec<UIntType, 2> r =
      mulhilo(philox_parameters<UIntType, 2, consts...>::multipliers[0], counters[0]);
    return { r[1] ^ round_keys[0] ^ counters[1], r[0] };
  }

  static VTKM_EXEC_CONT keys_type bump_keys(keys_type keys)
  {
    return { keys[0] + philox_parameters<UIntType, 2, consts...>::round_consts[0] };
  }
};

template <typename UIntType, std::size_t R, UIntType... consts>
class philox_functor<UIntType, 4, R, consts...>
{
  using counters_type = vtkm::Vec<UIntType, 4>;
  using keys_type = vtkm::Vec<UIntType, 2>;

  static VTKM_EXEC_CONT counters_type round(counters_type counters, keys_type round_keys)
  {
    vtkm::Vec<UIntType, 2> r0 =
      mulhilo(philox_parameters<UIntType, 4, consts...>::multipliers[0], counters[0]);
    vtkm::Vec<UIntType, 2> r1 =
      mulhilo(philox_parameters<UIntType, 4, consts...>::multipliers[1], counters[2]);
    return {
      r1[1] ^ round_keys[0] ^ counters[1], r1[0], r0[1] ^ round_keys[1] ^ counters[3], r0[0]
    };
  }

  static VTKM_EXEC_CONT keys_type bump_key(keys_type keys)
  {
    keys[0] += philox_parameters<UIntType, 4, consts...>::round_consts[0];
    keys[1] += philox_parameters<UIntType, 4, consts...>::round_consts[1];
    return keys;
  }

public:
  VTKM_EXEC_CONT counters_type operator()(counters_type counters, keys_type keys) const
  {
    for (std::size_t i = 0; i < R; ++i)
    {
      counters = round(counters, keys);
      keys = bump_key(keys);
    }
    return counters;
  }
};

} // namespace detail

using philox_functor2x32x7 = detail::philox_functor<vtkm::UInt32, 2, 7, 0xD256D193, 0x9E3779B9>;
using philox_functor2x32x10 = detail::philox_functor<vtkm::UInt32, 2, 10, 0xD256D193, 0x9E3779B9>;

} // namespace prng
} // namespace vtkm
#endif //vtk_m_prng_Philox_h
