//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_std_void_t_h
#define vtk_m_std_void_t_h

#include <vtkm/internal/Configure.h>

namespace vtkmstd
{

/// Implementation of std::void_t (C++17):
/// Allows for specialization of class templates based on members of template
/// parameters.
#if defined(VTKM_GCC) && (__GNUC__ < 5)
// Due to a defect in the wording (CWG 1558) unused parameters in alias templates
// were not guaranteed to ensure SFINAE, and therefore would consider everything
// to match the 'true' side. For VTK-m the only known compiler that implemented
// this defect is GCC < 5.
template <class... T>
struct void_pack
{
  using type = void;
};
template <class... T>
using void_t = typename void_pack<T...>::type;
#else
template <typename...>
using void_t = void;
#endif

} // end namespace vtkmstd

#endif //vtk_m_std_void_t_h
