//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_internal_DecayHelpers_h
#define vtk_m_worklet_internal_DecayHelpers_h

#include <type_traits>

namespace vtkm
{
namespace worklet
{
namespace internal
{

template <typename T>
using remove_pointer_and_decay = typename std::remove_pointer<typename std::decay<T>::type>::type;

template <typename T>
using remove_cvref = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
}
}
}
#endif
