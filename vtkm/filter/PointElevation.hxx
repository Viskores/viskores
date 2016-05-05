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

#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm {
namespace filter {


//-----------------------------------------------------------------------------
PointElevation::PointElevation():
  Worklet()
{
  this->SetOutputFieldName("elevation");
}

//-----------------------------------------------------------------------------
void PointElevation::SetLowPoint(vtkm::Float64 x, vtkm::Float64 y, vtkm::Float64 z)
{
  this->Worklet.SetLowPoint( vtkm::make_Vec(x,y,z) );
}

//-----------------------------------------------------------------------------
void PointElevation::SetHighPoint(vtkm::Float64 x, vtkm::Float64 y, vtkm::Float64 z)
{
  this->Worklet.SetHighPoint( vtkm::make_Vec(x,y,z) );
}

//-----------------------------------------------------------------------------
void PointElevation::SetRange(vtkm::Float64 low, vtkm::Float64 high)
{
  this->Worklet.SetRange(low,high);
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
vtkm::filter::ResultField
PointElevation::DoExecute(const vtkm::cont::DataSet &inDataSet,
                          const vtkm::cont::ArrayHandle<T,StorageType> &field,
                          const vtkm::filter::FieldMetadata &fieldMetadata,
                          const vtkm::filter::PolicyBase<DerivedPolicy>&,
                          const DeviceAdapter&)
{
  vtkm::cont::ArrayHandle<vtkm::Float64> outArray;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::PointElevation,
                                    DeviceAdapter > dispatcher(this->Worklet);

  //todo, we need to use the policy to determine the valid conversions
  //that the dispatcher should do
  dispatcher.Invoke(field, outArray);

  return vtkm::filter::ResultField(inDataSet,
                                   outArray,
                                   this->GetOutputFieldName(),
                                   fieldMetadata.GetAssociation(),
                                   fieldMetadata.GetCellSetName());
}

}
} // namespace vtkm::filter
