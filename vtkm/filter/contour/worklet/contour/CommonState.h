
//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_contour_CommonState_h
#define vtk_m_worklet_contour_CommonState_h

#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace worklet
{
namespace contour
{

struct CommonState
{
  explicit CommonState(bool mergeDuplicates)
    : MergeDuplicatePoints(mergeDuplicates)
  {
  }

  bool MergeDuplicatePoints = true;
  bool GenerateNormals = false;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> InterpolationWeights;
  vtkm::cont::ArrayHandle<vtkm::Id2> InterpolationEdgeIds;
  vtkm::cont::ArrayHandle<vtkm::Id> CellIdMap;
};
}
}
}

#endif
