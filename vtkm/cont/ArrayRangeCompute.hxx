//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ArrayRangeCompute_hxx
#define vtk_m_cont_ArrayRangeCompute_hxx

#include <vtkm/Deprecated.h>
#include <vtkm/cont/ArrayRangeComputeTemplate.h>

namespace vtkm
{

VTKM_DEPRECATED(1.6, "Use ArrayRangeComputeTemplate.h instead of ArrayRangeCompute.hxx.")
inline void ArrayRangeCompute_hxx_deprecated() {}

inline void ActivateArrayRangeCompute_hxx_deprecated_warning()
{
  ArrayRangeCompute_hxx_deprecated();
}

} // namespace vtkm

#endif //vtk_m_cont_ArrayRangeCompute_hxx
