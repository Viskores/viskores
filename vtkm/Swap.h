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

#ifdef VTKM_CUDA
#include <thrust/swap.h>
#else
#include <algorithm>
#endif

namespace vtkm
{

/// Performs a swap operation. Safe to call from cuda code.
#if defined(VTKM_CUDA)
// CUDA 12 adds a `cub::Swap` function that creates ambiguity with `vtkm::Swap`.
// This happens when a function from the `cub` namespace is called with an object of a class
// defined in the `vtkm` namespace as an argument. If that function has an unqualified call to
// `Swap`, it results in ADL being used, causing the templated functions `cub::Swap` and
// `vtkm::Swap` to conflict.
#if defined(VTKM_CUDA_VERSION_MAJOR) && (VTKM_CUDA_VERSION_MAJOR >= 12) && \
  defined(VTKM_CUDA_DEVICE_PASS)
using cub::Swap;
#else
template <typename T>
VTKM_EXEC_CONT inline void Swap(T& a, T& b)
{
  using thrust::swap;
  swap(a, b);
}
#endif
#elif defined(VTKM_HIP)
template <typename T>
__host__ inline void Swap(T& a, T& b)
{
  using std::swap;
  swap(a, b);
}
template <typename T>
__device__ inline void Swap(T& a, T& b)
{
  T temp = a;
  a = b;
  b = temp;
}
#else
template <typename T>
VTKM_EXEC_CONT inline void Swap(T& a, T& b)
{
  using std::swap;
  swap(a, b);
}
#endif

} // end namespace vtkm

#endif //vtk_m_Swap_h
