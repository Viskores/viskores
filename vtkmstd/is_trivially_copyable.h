//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_std_is_trivially_copyable_h
#define vtk_m_std_is_trivially_copyable_h

#include <type_traits>

#if defined(VTKM_USING_GLIBCXX_4)
namespace vtkmstd
{

// GCC 4.8 and 4.9 standard library does not support std::is_trivially_copyable.
// There is no relyable way to get this information (since it has to come special from
// the compiler). For our purposes, we will report as nothing being trivially copyable,
// which causes us to call the constructors with everything. This should be fine unless
// some other part of the compiler is trying to check for trivial copies (perhaps nvcc
// on top of GCC 4.8).
template <typename>
struct is_trivially_copyable : std::false_type
{
};

} // namespace vtkmstd

#else // NOT VTKM_USING_GLIBCXX_4
namespace vtkmstd
{

using std::is_trivially_copyable;

} // namespace vtkmstd
#endif

#endif //vtk_m_std_is_trivially_copyable_h
