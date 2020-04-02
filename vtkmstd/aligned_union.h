//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_std_aligned_union_h
#define vtk_m_std_aligned_union_h

#include <vtkm/internal/Configure.h>

#include <type_traits>

#if defined(VTKM_USING_GLIBCXX_4)

#include <algorithm>

namespace vtkmstd
{

template <std::size_t... Xs>
struct max_size;
template <std::size_t X>
struct max_size<X>
{
  static constexpr std::size_t value = X;
};
template <std::size_t X0, std::size_t... Xs>
struct max_size<X0, Xs...>
{
  static constexpr std::size_t other_value = max_size<Xs...>::value;
  static constexpr std::size_t value = (other_value > X0) ? other_value : X0;
};

// This is to get around an apparent bug in GCC 4.8 where alianas(x) does not
// seem to work when x is a constexpr. See
// https://stackoverflow.com/questions/29879609/g-complains-constexpr-function-is-not-a-constant-expression
template <std::size_t Alignment, std::size_t Size>
struct aligned_data_block
{
  alignas(Alignment) char _s[Size];
};

template <std::size_t Len, class... Types>
struct aligned_union
{
  static constexpr std::size_t alignment_value = vtkmstd::max_size<alignof(Types)...>::value;

  using type =
    vtkmstd::aligned_data_block<alignment_value, vtkmstd::max_size<Len, sizeof(Types)...>::value>;
};

} // namespace vtkmstd

#else // NOT VTKM_USING_GLIBCXX_4

namespace vtkmstd
{

using std::aligned_union;

} // namespace vtkmstd

#endif // NOT VTKM_USING_GLIBCXX_4

#endif //vtk_m_std_aligned_union_h
