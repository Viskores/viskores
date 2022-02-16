//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_CellMeasures_h
#define vtk_m_filter_CellMeasures_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/mesh_info/CellMeasures.h>

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/mesh_info/CellMeasures.h instead of vtkm/filter/CellMeasures.h.")
inline void CellMeasures_deprecated() {}

inline void CellMeasures_deprecated_warning()
{
  CellMeasures_deprecated();
}
#endif //vtk_m_filter_CellMeasures_h
