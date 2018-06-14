//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/CellDeepCopy.h>
#include <vtkm/worklet/RemoveUnusedPoints.h>

#include <vector>

namespace vtkm
{
namespace filter
{

inline VTKM_CONT CleanGrid::CleanGrid()
  : CompactPointFields(true)
{
}

template <typename Policy, typename Device>
inline VTKM_CONT vtkm::cont::DataSet CleanGrid::DoExecute(const vtkm::cont::DataSet& inData,
                                                          vtkm::filter::PolicyBase<Policy> policy,
                                                          Device)
{
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  using CellSetType = vtkm::cont::CellSetExplicit<>;
  using VecId = std::vector<CellSetType>::size_type;

  VecId numCellSets = static_cast<VecId>(inData.GetNumberOfCellSets());

  std::vector<CellSetType> outputCellSets(numCellSets);

  // Do a deep copy of the cells to new CellSetExplicit structures
  for (VecId cellSetIndex = 0; cellSetIndex < numCellSets; cellSetIndex++)
  {
    vtkm::cont::DynamicCellSet inCellSet =
      inData.GetCellSet(static_cast<vtkm::IdComponent>(cellSetIndex));

    vtkm::worklet::CellDeepCopy::Run(
      vtkm::filter::ApplyPolicy(inCellSet, policy), outputCellSets[cellSetIndex], Device());
  }

  // Optionally adjust the cell set indices to remove all unused points
  if (this->GetCompactPointFields())
  {
    this->PointCompactor.FindPointsStart(Device());
    for (VecId cellSetIndex = 0; cellSetIndex < numCellSets; cellSetIndex++)
    {
      this->PointCompactor.FindPoints(outputCellSets[cellSetIndex], Device());
    }
    this->PointCompactor.FindPointsEnd(Device());

    for (VecId cellSetIndex = 0; cellSetIndex < numCellSets; cellSetIndex++)
    {
      outputCellSets[cellSetIndex] =
        this->PointCompactor.MapCellSet(outputCellSets[cellSetIndex], Device());
    }
  }

  // Construct resulting data set with new cell sets
  vtkm::cont::DataSet outData;
  for (VecId cellSetIndex = 0; cellSetIndex < numCellSets; cellSetIndex++)
  {
    outData.AddCellSet(outputCellSets[cellSetIndex]);
  }

  // Pass the coordinate systems
  // TODO: This is very awkward. First of all, there is no support for dealing
  // with coordinate systems at all. That is fine if you are computing a new
  // coordinate system, but a pain if you are deriving the coordinate system
  // array. Second, why is it that coordinate systems are automatically mapped
  // but other fields are not? Why shouldn't the Execute of a filter also set
  // up all the fields of the output data set?
  for (vtkm::IdComponent coordSystemIndex = 0;
       coordSystemIndex < inData.GetNumberOfCoordinateSystems();
       coordSystemIndex++)
  {
    vtkm::cont::CoordinateSystem coordSystem = inData.GetCoordinateSystem(coordSystemIndex);

    if (this->GetCompactPointFields())
    {
      auto outArray = this->MapPointField(coordSystem.GetData(), Device());
      outData.AddCoordinateSystem(vtkm::cont::CoordinateSystem(coordSystem.GetName(), outArray));
    }
    else
    {
      outData.AddCoordinateSystem(coordSystem);
    }
  }

  return outData;
}

template <typename ValueType, typename Storage, typename Policy, typename Device>
inline VTKM_CONT bool CleanGrid::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<ValueType, Storage>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<Policy>,
  Device)
{
  if (this->GetCompactPointFields() && fieldMeta.IsPointField())
  {
    vtkm::cont::ArrayHandle<ValueType> compactedArray = this->MapPointField(input, Device());
    result.AddField(fieldMeta.AsField(compactedArray));
  }
  else
  {
    result.AddField(fieldMeta.AsField(input));
  }

  return true;
}

template <typename ValueType, typename Storage, typename Device>
inline VTKM_CONT vtkm::cont::ArrayHandle<ValueType> CleanGrid::MapPointField(
  const vtkm::cont::ArrayHandle<ValueType, Storage>& inArray,
  Device) const
{
  VTKM_ASSERT(this->GetCompactPointFields());

  return this->PointCompactor.MapPointFieldDeep(inArray, Device());
}
}
}
