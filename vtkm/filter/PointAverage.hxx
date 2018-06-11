//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/internal/CreateResult.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT PointAverage::PointAverage()
  : vtkm::filter::FilterCell<PointAverage>()
  , Worklet()
{
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet PointAverage::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& inField,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter&)
{
  if (!fieldMetadata.IsCellField())
  {
    throw vtkm::cont::ErrorFilterExecution("Cell field expected.");
  }

  vtkm::cont::DynamicCellSet cellSet = input.GetCellSet(this->GetActiveCellSetIndex());

  //todo: we need to ask the policy what storage type we should be using
  //If the input is implicit, we should know what to fall back to
  vtkm::cont::ArrayHandle<T> outArray;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::PointAverage, DeviceAdapter> dispatcher(
    this->Worklet);

  dispatcher.Invoke(vtkm::filter::ApplyPolicy(cellSet, policy), inField, outArray);

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = fieldMetadata.GetName();
  }

  return internal::CreateResult(
    input, outArray, outputName, vtkm::cont::Field::Association::POINTS, cellSet.GetName());
}
}
} // namespace vtkm::filter
