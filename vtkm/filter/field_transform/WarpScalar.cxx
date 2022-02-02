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
#include <vtkm/filter/field_transform/WarpScalar.h>
#include <vtkm/filter/field_transform/worklet/WarpScalar.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
//-----------------------------------------------------------------------------
VTKM_CONT WarpScalar::WarpScalar(vtkm::FloatDefault scaleAmount)
  : ScaleAmount(scaleAmount)
{
  this->SetOutputFieldName("warpscalar");
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet WarpScalar::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  // TODO: do we still need to deal with this?
  //  WarpScalar often operates on a constant normal value
  //  using AdditionalFieldStorage =
  //    vtkm::List<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_32>::StorageTag,
  //               vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_64>::StorageTag>;
  // TODO:
  //  Ken suggested to provide additional public interface for user to supply a single
  //  value for const normal (and scale factor?).
  vtkm::cont::Field normalF =
    inDataSet.GetField(this->NormalFieldName, this->NormalFieldAssociation);
  vtkm::cont::ArrayHandle<vtkm::Vec3f> normalArray;
  if (normalF.GetData().CanConvert<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_32>>())
  {
    vtkm::Vec3f_32 norm =
      normalF.GetData().AsArrayHandle<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_32>>().GetValue();
    normalArray.AllocateAndFill(normalF.GetData().GetNumberOfValues(), vtkm::Vec3f(norm));
  }
  else if (normalF.GetData().CanConvert<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_64>>())
  {
    vtkm::Vec3f_64 norm =
      normalF.GetData().AsArrayHandle<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_64>>().GetValue();
    normalArray.AllocateAndFill(normalF.GetData().GetNumberOfValues(), vtkm::Vec3f(norm));
  }
  else
  {
    vtkm::cont::ArrayCopyShallowIfPossible(normalF.GetData(), normalArray);
  }

  vtkm::cont::Field sfF =
    inDataSet.GetField(this->ScalarFactorFieldName, this->ScalarFactorFieldAssociation);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> scaleFactorArray;
  vtkm::cont::ArrayCopyShallowIfPossible(sfF.GetData(), scaleFactorArray);

  vtkm::cont::UnknownArrayHandle outArray;

  auto resolveType = [&](const auto& concrete) {
    // We know ValueType is some form of Vec3 due to CastAndCallVecField
    using VecType = typename std::decay_t<decltype(concrete)>::ValueType;

    vtkm::cont::ArrayHandle<VecType> result;
    vtkm::worklet::WarpScalar worklet;
    worklet.Run(concrete, normalArray, scaleFactorArray, this->ScaleAmount, result);
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
