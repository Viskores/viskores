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

#include <vtkm/filter/internal/CreateResult.h>
#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT CylindricalCoordinateTransform::CylindricalCoordinateTransform()
  : Worklet()
{
  this->SetOutputFieldName("cylindricalCoordinateSystemTransform");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet CylindricalCoordinateTransform::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  vtkm::cont::ArrayHandle<T> outArray;
  Worklet.Run(field, outArray, device);
  return internal::CreateResult(inDataSet,
                                outArray,
                                this->GetOutputFieldName(),
                                fieldMetadata.GetAssociation(),
                                fieldMetadata.GetCellSetName());
}

//-----------------------------------------------------------------------------
inline VTKM_CONT SphericalCoordinateTransform::SphericalCoordinateTransform()
  : Worklet()
{
  this->SetOutputFieldName("sphericalCoordinateSystemTransform");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet SphericalCoordinateTransform::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device) const
{
  vtkm::cont::ArrayHandle<T> outArray;
  Worklet.Run(field, outArray, device);
  return internal::CreateResult(inDataSet,
                                outArray,
                                this->GetOutputFieldName(),
                                fieldMetadata.GetAssociation(),
                                fieldMetadata.GetCellSetName());
}
}
} // namespace vtkm::filter
