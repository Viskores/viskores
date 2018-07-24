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
inline VTKM_CONT WarpScalar::WarpScalar(vtkm::FloatDefault scaleAmount)
  : vtkm::filter::FilterField<WarpScalar>()
  , Worklet()
  , NormalFieldName("normal")
  , NormalFieldAssociation(vtkm::cont::Field::Association::ANY)
  , ScalarFactorFieldName("scalarfactor")
  , ScalarFactorFieldAssociation(vtkm::cont::Field::Association::ANY)
  , ScaleAmount(scaleAmount)
{
  this->SetOutputFieldName("warpscalar");
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet WarpScalar::DoExecute(
  const vtkm::cont::DataSet& inDataSet,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter& device)
{
  using vecType = vtkm::Vec<T, 3>;
  auto normalF = inDataSet.GetField(this->NormalFieldName, this->NormalFieldAssociation);
  auto sfF = inDataSet.GetField(this->ScalarFactorFieldName, this->ScalarFactorFieldAssociation);
  vtkm::cont::ArrayHandle<vecType> result;
  this->Worklet.Run(field,
                    vtkm::filter::ApplyPolicy(normalF, policy),
                    vtkm::filter::ApplyPolicy(sfF, policy),
                    this->ScaleAmount,
                    result,
                    device);

  return internal::CreateResult(inDataSet,
                                result,
                                this->GetOutputFieldName(),
                                fieldMetadata.GetAssociation(),
                                fieldMetadata.GetCellSetName());
}
}
}
