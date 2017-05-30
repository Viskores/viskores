//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT ClipWithField::ClipWithField()
  : vtkm::filter::FilterDataSetWithField<ClipWithField>()
  , ClipValue(0)
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::filter::ResultDataSet ClipWithField::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  if (fieldMeta.IsPointField() == false)
  {
    //todo: we need to mark this as a failure of input, not a failure
    //of the algorithm
    return vtkm::filter::ResultDataSet();
  }

  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  const vtkm::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::CellSetExplicit<> outputCellSet =
    this->Worklet.Run(vtkm::filter::ApplyPolicy(cells, policy), field, this->ClipValue, device);

  //create the output data
  vtkm::cont::DataSet output;
  output.AddCellSet(outputCellSet);

  // Compute the new boundary points and add them to the output:
  vtkm::cont::DynamicArrayHandle outputCoordsArray =
    this->Worklet.ProcessField(vtkm::filter::ApplyPolicy(inputCoords, policy), device);
  vtkm::cont::CoordinateSystem outputCoords(inputCoords.GetName(), outputCoordsArray);
  output.AddCoordinateSystem(outputCoords);
  vtkm::filter::ResultDataSet result(output);

  return result;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool ClipWithField::DoMapField(
  vtkm::filter::ResultDataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  if (fieldMeta.IsPointField() == false)
  {
    //not a point field, we can't map it
    return false;
  }

  vtkm::cont::DynamicArrayHandle output = this->Worklet.ProcessField(input, device);

  //use the same meta data as the input so we get the same field name, etc.
  result.GetDataSet().AddField(fieldMeta.AsField(output));
  return true;
}
}
}
