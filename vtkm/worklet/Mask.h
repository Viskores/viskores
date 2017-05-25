//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_m_worklet_Mask_h
#define vtkm_m_worklet_Mask_h

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm
{
namespace worklet
{

// Subselect points using stride for now, creating new cellset of vertices
class Mask
{
public:
  struct BoolType : vtkm::ListTagBase<bool>
  {
  };

  template <typename CellSetType, typename DeviceAdapter>
  vtkm::cont::CellSetPermutation<CellSetType> Run(const CellSetType& cellSet, const vtkm::Id stride,
                                                  DeviceAdapter)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;
    typedef vtkm::cont::CellSetPermutation<CellSetType> OutputType;

    vtkm::Id numberOfInputCells = cellSet.GetNumberOfCells();
    vtkm::Id numberOfSampledCells = numberOfInputCells / stride;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> strideArray(0, stride, numberOfSampledCells);

    DeviceAlgorithm::Copy(strideArray, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet, cellSet.GetName());
  }

  // Permute cell data to match permuted cells
  class PermuteCellData
  {
  public:
    PermuteCellData(const vtkm::cont::ArrayHandle<vtkm::Id> validCellIds,
                    vtkm::cont::DynamicArrayHandle& data)
      : ValidCellIds(validCellIds)
      , Data(&data)
    {
    }

    template <typename ArrayHandleType>
    void operator()(const ArrayHandleType& input) const
    {
      *(this->Data) = vtkm::cont::DynamicArrayHandle(
        vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, input));
    }

  private:
    vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
    vtkm::cont::DynamicArrayHandle* Data;
  };

  vtkm::cont::Field ProcessCellField(const vtkm::cont::Field field) const
  {
    if (field.GetAssociation() != vtkm::cont::Field::ASSOC_CELL_SET)
    {
      throw vtkm::cont::ErrorBadValue("Expecting cell field.");
    }

    vtkm::cont::DynamicArrayHandle data;
    CastAndCall(field, PermuteCellData(this->ValidCellIds, data));

    return vtkm::cont::Field(field.GetName(), field.GetAssociation(), field.GetAssocCellSet(),
                             data);
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Mask_h
