//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/filter/field_transform/WarpVector.h>
#include <vtkm/filter/field_transform/worklet/WarpVector.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
//-----------------------------------------------------------------------------
VTKM_CONT WarpVector::WarpVector(vtkm::FloatDefault scale)
  : Scale(scale)
{
  this->SetOutputFieldName("warpvector");
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet WarpVector::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::Field vectorF =
    inDataSet.GetField(this->VectorFieldName, this->VectorFieldAssociation);
  vtkm::cont::ArrayHandle<vtkm::Vec3f> vectorArray;
  if (vectorF.GetData().CanConvert<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_32>>())
  {
    vtkm::Vec3f_32 norm =
      vectorF.GetData().AsArrayHandle<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_32>>().GetValue();
    vectorArray.AllocateAndFill(vectorF.GetData().GetNumberOfValues(), vtkm::Vec3f(norm));
  }
  else if (vectorF.GetData().CanConvert<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_64>>())
  {
    vtkm::Vec3f_64 norm =
      vectorF.GetData().AsArrayHandle<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_64>>().GetValue();
    vectorArray.AllocateAndFill(vectorF.GetData().GetNumberOfValues(), vtkm::Vec3f(norm));
  }
  else
  {
    vtkm::cont::ArrayCopyShallowIfPossible(vectorF.GetData(), vectorArray);
  }

  vtkm::cont::UnknownArrayHandle outArray;

  auto resolveType = [&](const auto& concrete) {
    // We know ValueType is some form of Vec3 due to CastAndCallVecField
    using VecType = typename std::decay_t<decltype(concrete)>::ValueType;

    vtkm::cont::ArrayHandle<VecType> result;
    vtkm::worklet::WarpVector worklet;
    worklet.Run(concrete, vectorArray, this->Scale, result);
    outArray = result;
  };
  const auto& field = this->GetFieldFromDataSet(inDataSet);
  this->CastAndCallVecField<3>(field, resolveType);

  return this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), field.GetAssociation(), outArray);
}
} // namespace field_transform
} // namespace filter
} // namespace vtkm
