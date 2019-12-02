//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_m_filter_CleanGrid_hxx
#define vtkm_m_filter_CleanGrid_hxx

#include <vtkm/worklet/CellDeepCopy.h>
#include <vtkm/worklet/RemoveUnusedPoints.h>

#include <vector>

namespace vtkm
{
namespace filter
{

template <typename Policy>
inline VTKM_CONT vtkm::cont::DataSet CleanGrid::DoExecute(const vtkm::cont::DataSet& inData,
                                                          vtkm::filter::PolicyBase<Policy> policy)
{
  using CellSetType = vtkm::cont::CellSetExplicit<>;

  CellSetType outputCellSet;
  // Do a deep copy of the cells to new CellSetExplicit structures
  const vtkm::cont::DynamicCellSet& inCellSet = inData.GetCellSet();
  if (inCellSet.IsType<CellSetType>())
  {
    // Is expected type, do a shallow copy
    outputCellSet = inCellSet.Cast<CellSetType>();
  }
  else
  { // Clean the grid
    auto deducedCellSet = vtkm::filter::ApplyPolicyCellSet(inCellSet, policy);
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;

    this->Invoke(worklet::CellDeepCopy::CountCellPoints{}, deducedCellSet, numIndices);

    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
    vtkm::cont::ArrayHandle<vtkm::Id> offsets;
    vtkm::Id connectivitySize;
    vtkm::cont::ConvertNumIndicesToOffsets(numIndices, offsets, connectivitySize);
    numIndices.ReleaseResourcesExecution();

    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    connectivity.Allocate(connectivitySize);

    auto offsetsTrim =
      vtkm::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);

    this->Invoke(worklet::CellDeepCopy::PassCellStructure{},
                 deducedCellSet,
                 shapes,
                 vtkm::cont::make_ArrayHandleGroupVecVariable(connectivity, offsetsTrim));
    shapes.ReleaseResourcesExecution();
    offsets.ReleaseResourcesExecution();
    connectivity.ReleaseResourcesExecution();

    outputCellSet.Fill(deducedCellSet.GetNumberOfPoints(), shapes, connectivity, offsets);

    //Release the input grid from the execution space
    deducedCellSet.ReleaseResourcesExecution();
  }

  return this->GenerateOutput(inData, outputCellSet);
}
}
}

#endif
