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
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/field_transform/CompositeVectors.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
VTKM_CONT vtkm::cont::DataSet CompositeVectors::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::UnknownArrayHandle outArray;
  if (this->NumberOfFields < 2)
  {
    throw vtkm::cont::ErrorFilterExecution("FieldNameList is supposed to be larger than 2.");
  }
  else if (this->NumberOfFields == 2)
  {
    auto fieldAssociation = this->GetFieldFromDataSet(0, inDataSet).GetAssociation();
    if (fieldAssociation != this->GetFieldFromDataSet(1, inDataSet).GetAssociation())
    {
      throw vtkm::cont::ErrorFilterExecution("Field 0 and Field 2 have different associations.");
    }
    auto resolveType2d = [&](const auto& field0) {
      using T = typename std::decay_t<decltype(field0)>::ValueType;
      vtkm::cont::ArrayHandle<T> field1;
      vtkm::cont::ArrayCopyShallowIfPossible(this->GetFieldFromDataSet(1, inDataSet).GetData(),
                                             field1);

      auto compositedArray = vtkm::cont::make_ArrayHandleCompositeVector(field0, field1);

      using VecType = vtkm::Vec<T, 2>;
      using ArrayHandleType = vtkm::cont::ArrayHandle<VecType>;
      ArrayHandleType result;
      vtkm::cont::ArrayCopy(compositedArray, result);
      outArray = result;
    };
    const auto& inField0 = this->GetFieldFromDataSet(0, inDataSet);
    inField0.GetData().CastAndCallForTypes<vtkm::TypeListScalarAll, VTKM_DEFAULT_STORAGE_LIST>(
      resolveType2d);
  }
  else if (this->NumberOfFields == 3)
  {
    auto fieldAssociation0 = this->GetFieldFromDataSet(0, inDataSet).GetAssociation();
    auto fieldAssociation1 = this->GetFieldFromDataSet(1, inDataSet).GetAssociation();
    auto fieldAssociation2 = this->GetFieldFromDataSet(2, inDataSet).GetAssociation();

    if (fieldAssociation0 != fieldAssociation1 || fieldAssociation1 != fieldAssociation2 ||
        fieldAssociation0 != fieldAssociation2)
    {
      throw vtkm::cont::ErrorFilterExecution(
        "Field 0, Field 1 and Field 2 have different associations.");
    }

    auto resolveType3d = [&](const auto& field0) {
      using T = typename std::decay_t<decltype(field0)>::ValueType;
      vtkm::cont::ArrayHandle<T> field1;
      vtkm::cont::ArrayCopyShallowIfPossible(this->GetFieldFromDataSet(1, inDataSet).GetData(),
                                             field1);
      vtkm::cont::ArrayHandle<T> field2;
      vtkm::cont::ArrayCopyShallowIfPossible(this->GetFieldFromDataSet(2, inDataSet).GetData(),
                                             field2);
      auto compositedArray = vtkm::cont::make_ArrayHandleCompositeVector(field0, field1, field2);

      using VecType = vtkm::Vec<T, 3>;
      using ArrayHandleType = vtkm::cont::ArrayHandle<VecType>;
      ArrayHandleType result;
      // ArrayHandleCompositeVector currently does not implement the ability to
      // get to values on the control side, so copy to an array that is accessible.
      vtkm::cont::ArrayCopy(compositedArray, result);
      outArray = result;
    };

    const auto& inField0 = this->GetFieldFromDataSet(0, inDataSet);
    inField0.GetData().CastAndCallForTypes<vtkm::TypeListScalarAll, VTKM_DEFAULT_STORAGE_LIST>(
      resolveType3d);
  }
  else
  {
    throw vtkm::cont::ErrorFilterExecution(
      "Using make_ArrayHandleCompositeVector to composite vectors more than 3.");
  }

  return this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), this->GetActiveFieldAssociation(), outArray);
}
} // namespace field_transform
} // namespace vtkm::filter
} // namespace vtkm
