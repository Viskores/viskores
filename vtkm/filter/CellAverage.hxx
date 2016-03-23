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

#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm {
namespace filter {


//-----------------------------------------------------------------------------
CellAverage::CellAverage():
  vtkm::filter::CellFilter<CellAverage>(),
  Worklet()
{

}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
vtkm::filter::FieldResult CellAverage::DoExecute(const vtkm::cont::DataSet &input,
                                                 const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                                 const vtkm::filter::FieldMetadata&,
                                                 const vtkm::filter::PolicyBase<DerivedPolicy>&,
                                                 const DeviceAdapter&)
{
  vtkm::cont::DynamicCellSet cellSet =
                  input.GetCellSet(this->GetActiveCellSetIndex());

  //todo: we need to ask the policy what storage type we should be using
  //If the input is implicit, we should know what to fall back to
  vtkm::cont::ArrayHandle<T> outArray = field;

  vtkm::worklet::DispatcherMapTopology<vtkm::worklet::CellAverage,
                                       DeviceAdapter > dispatcher(this->Worklet);

  //todo: we need to use the policy to determine the valid conversions
  //that the dispatcher should do, including the result from GetCellSet
  dispatcher.Invoke(field, cellSet, outArray);

  vtkm::cont::Field outField(this->GetOutputFieldName(),
                             vtkm::cont::Field::ASSOC_CELL_SET,
                             cellSet.GetCellSet().GetName(),
                             outArray);

  return vtkm::filter::FieldResult(outField);
}

}
} // namespace vtkm::filter
