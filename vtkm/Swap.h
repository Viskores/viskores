//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_Swap_h
#define vtk_m_Swap_h

#include <vtkm/internal/ExportMacros.h>

#ifdef __CUDACC__
#include <thrust/swap.h>
#else
#include <algorithm>
#endif

namespace vtkm
{

/// Performs a swap operation. Safe to call from cuda code.
#ifdef __CUDACC__
template <typename T>
VTKM_EXEC_CONT void Swap(T& a, T& b)
{
  using namespace thrust;
  swap(a, b);
}
#else
template <typename T>
VTKM_EXEC_CONT void Swap(T& a, T& b)
{
  using namespace std;
  swap(a, b);
}
#endif

} // end namespace vtkm

#endif //vtk_m_Swap_h
