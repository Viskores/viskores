//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_m_worklet_Mask_h
#define vtkm_m_worklet_Mask_h

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace worklet
{

// Subselect points using stride for now, creating new cellset of vertices
class Mask
{
public:
  template <typename CellSetType>
  vtkm::cont::CellSetPermutation<CellSetType> Run(const CellSetType& cellSet, const vtkm::Id stride)
  {
    using OutputType = vtkm::cont::CellSetPermutation<CellSetType>;

    vtkm::Id numberOfInputCells = cellSet.GetNumberOfCells();
    vtkm::Id numberOfSampledCells = numberOfInputCells / stride;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> strideArray(0, stride, numberOfSampledCells);

    vtkm::cont::ArrayCopy(strideArray, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet);
  }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& in) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, in);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    vtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Mask_h
