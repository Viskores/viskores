//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace filter
{

namespace clipwithimplicitfunction
{

template <typename Device>
struct PointMapHelper
{
  PointMapHelper(const vtkm::worklet::Clip& worklet, vtkm::cont::DynamicArrayHandle& output)
    : Worklet(worklet)
    , Output(output)
  {
  }

  template <typename ArrayType>
  void operator()(const ArrayType& array) const
  {
    this->Output = this->Worklet.ProcessPointField(array, Device());
  }

  const vtkm::worklet::Clip& Worklet;
  vtkm::cont::DynamicArrayHandle& Output;
};

} // end namespace clipwithimplicitfunction

//-----------------------------------------------------------------------------

ClipWithImplicitFunction::ClipWithImplicitFunction()
  : Invert(false)
{
}

template <typename DerivedPolicy, typename DeviceAdapter>
inline vtkm::cont::DataSet ClipWithImplicitFunction::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  using namespace clipwithimplicitfunction;

  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());

  const vtkm::cont::CoordinateSystem& inputCoords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::CellSetExplicit<> outputCellSet = this->Worklet.Run(
    vtkm::filter::ApplyPolicy(cells, policy), this->Function, inputCoords, this->Invert, device);

  // compute output coordinates
  auto outputCoordsArray = this->Worklet.ProcessPointField(inputCoords.GetData(), device);
  vtkm::cont::CoordinateSystem outputCoords(inputCoords.GetName(), outputCoordsArray);

  //create the output data
  vtkm::cont::DataSet output;
  output.AddCellSet(outputCellSet);
  output.AddCoordinateSystem(outputCoords);

  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline bool ClipWithImplicitFunction::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  vtkm::cont::ArrayHandle<T> output;

  if (fieldMeta.IsPointField())
  {
    output = this->Worklet.ProcessPointField(input, device);
  }
  else if (fieldMeta.IsCellField())
  {
    output = this->Worklet.ProcessCellField(input, device);
  }
  else
  {
    return false;
  }

  //use the same meta data as the input so we get the same field name, etc.
  result.AddField(fieldMeta.AsField(output));

  return true;
}
}
} // end namespace vtkm::filter
