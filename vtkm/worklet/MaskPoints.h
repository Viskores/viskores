//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_m_worklet_MaskPoints_h
#define vtkm_m_worklet_MaskPoints_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace worklet
{

// Subselect points using stride for now, creating new cellset of vertices
class MaskPoints
{
public:
  template <typename CellSetType>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet, const vtkm::Id stride)
  {
    vtkm::Id numberOfInputPoints = cellSet.GetNumberOfPoints();
    vtkm::Id numberOfSampledPoints = numberOfInputPoints / stride;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> strideArray(0, stride, numberOfSampledPoints);

    vtkm::cont::ArrayHandle<vtkm::Id> pointIds;
    vtkm::cont::ArrayCopy(strideArray, pointIds);

    // Make CellSetSingleType with VERTEX at each point id
    vtkm::cont::CellSetSingleType<> outCellSet;
    outCellSet.Fill(numberOfInputPoints, vtkm::CellShapeTagVertex::Id, 1, pointIds);

    return outCellSet;
  }
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_MaskPoints_h
