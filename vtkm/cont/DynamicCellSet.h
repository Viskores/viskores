//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_DynamicCellSet_h
#define vtk_m_cont_DynamicCellSet_h

#include <vtkm/cont/UncertainCellSet.h>

struct VTKM_DEPRECATED(1.8, "Use UnknownCellSet.h or UncertainCellSet.h.")
  DynamicCellSet_h_header_is_deprecated
{
  int x;
};

inline void EmitDynamicCellSetHDeprecationWarning()
{
  static DynamicCellSet_h_header_is_deprecated x;
  ++x.x;
}

#endif //vtk_m_cont_DynamicCellSet_h
