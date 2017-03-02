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

namespace vtkm {
namespace filter {

//-----------------------------------------------------------------------------
inline VTKM_CONT
ExternalFaces::ExternalFaces():
  vtkm::filter::FilterDataSet<ExternalFaces>()
{

}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
vtkm::filter::ResultDataSet ExternalFaces::DoExecute(const vtkm::cont::DataSet& input,
                                                     const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                                     const DeviceAdapter&)
{
  //1. extract the cell set
  const vtkm::cont::DynamicCellSet& cells =
                  input.GetCellSet(this->GetActiveCellSetIndex());

  //2. using the policy convert the dynamic cell set, and run the
  // external faces worklet
  vtkm::cont::CellSetExplicit<> outCellSet(cells.GetName());
  vtkm::worklet::ExternalFaces exfaces;
  exfaces.Run(vtkm::filter::ApplyPolicyUnstructured(cells, policy),
              outCellSet, DeviceAdapter());

  //3. create the output dataset
  vtkm::cont::DataSet output;
  output.AddCellSet(outCellSet);
  output.AddCoordinateSystem(
          input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()) );

  return vtkm::filter::ResultDataSet(output);
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
inline VTKM_CONT
bool ExternalFaces::DoMapField(vtkm::filter::ResultDataSet&,
                               const vtkm::cont::ArrayHandle<T, StorageType>&,
                               const vtkm::filter::FieldMetadata&,
                               const vtkm::filter::PolicyBase<DerivedPolicy>&,
                               const DeviceAdapter&)
{
  return false;
}

}
}
