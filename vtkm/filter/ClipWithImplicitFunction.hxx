//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm {
namespace filter {

//-----------------------------------------------------------------------------
template <typename ImplicitFunctionType, typename DerivedPolicy>
inline
void ClipWithImplicitFunction::SetImplicitFunction(
  const std::shared_ptr<ImplicitFunctionType> &func,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  func->ResetDevice(DerivedPolicy::DeviceAdapterList);
  this->Function = func;
}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy,
         typename DeviceAdapter>
inline vtkm::filter::ResultDataSet ClipWithImplicitFunction::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells =
                  input.GetCellSet(this->GetActiveCellSetIndex());

  const vtkm::cont::CoordinateSystem& inputCoords =
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());


  vtkm::cont::CellSetExplicit<> outputCellSet =
          this->Worklet.Run( vtkm::filter::ApplyPolicy(cells, policy),
                             *this->Function,
                             inputCoords,
                             device
                           );

  // compute output coordinates
  vtkm::cont::CoordinateSystem outputCoords;
  outputCoords.SetData(this->Worklet.ProcessField(inputCoords, device));

  //create the output data
  vtkm::cont::DataSet output;
  output.AddCellSet( outputCellSet );
  output.AddCoordinateSystem( outputCoords );

  vtkm::filter::ResultDataSet result(output);
  return result;
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
inline bool ClipWithImplicitFunction::DoMapField(
  vtkm::filter::ResultDataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  if(fieldMeta.IsPointField() == false)
  {
    //not a point field, we can't map it
    return false;
  }

  vtkm::cont::DynamicArrayHandle output =
                          this->Worklet.ProcessField( input, device);

  //use the same meta data as the input so we get the same field name, etc.
  result.GetDataSet().AddField( fieldMeta.AsField(output) );
  return true;
}

}
}
