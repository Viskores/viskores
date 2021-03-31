//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_Algorithms_h
#define vtk_m_Algorithms_h

#include <vtkm/BinarySearch.h>
#include <vtkm/Deprecated.h>
#include <vtkm/LowerBound.h>
#include <vtkm/UpperBound.h>

namespace vtkm
{

VTKM_DEPRECATED(1.6, "Use BinarySearch.h, LowerBound.h, or UpperBound.h instead of Algorithms.h.")
inline void Algorithms_h_deprecated() {}

inline void ActivateAlgorithms_h_deprecated_warning()
{
  Algorithms_h_deprecated();
}

} // end namespace vtkm

#endif // vtk_m_Algorithms_h
