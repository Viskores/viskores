//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/vector_analysis/DotProduct.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace // anonymous namespace making worklet::DotProduct internal to this .cxx
{

struct DotProductWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T1, typename T2, typename T3>
  VTKM_EXEC void operator()(const T1& v1, const T2& v2, T3& outValue) const
  {
    VTKM_ASSERT(v1.GetNumberOfComponents() == v2.GetNumberOfComponents());
    outValue = v1[0] * v2[0];
    for (vtkm::IdComponent i = 1; i < v1.GetNumberOfComponents(); ++i)
    {
      outValue += v1[i] * v2[i];
    }
  }
};

template <typename PrimaryArrayType>
vtkm::cont::UnknownArrayHandle DoDotProduct(const PrimaryArrayType& primaryArray,
                                            const vtkm::cont::Field& secondaryField)
{
  using T = typename PrimaryArrayType::ValueType::ComponentType;

  vtkm::cont::Invoker invoke;
  vtkm::cont::ArrayHandle<T> outputArray;

  if (secondaryField.GetData().IsBaseComponentType<T>())
  {
    invoke(DotProductWorklet{},
           primaryArray,
           secondaryField.GetData().ExtractArrayFromComponents<T>(),
           outputArray);
  }
  else
  {
    // Data types of primary and secondary array do not match. Rather than try to replicate every
    // possibility, get the secondary array as a FloatDefault.
    vtkm::cont::UnknownArrayHandle castSecondaryArray = secondaryField.GetDataAsDefaultFloat();
    invoke(DotProductWorklet{},
           primaryArray,
           castSecondaryArray.ExtractArrayFromComponents<vtkm::FloatDefault>(),
           outputArray);
  }

  return outputArray;
}

} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace vector_analysis
{

VTKM_CONT DotProduct::DotProduct()
{
  this->SetOutputFieldName("dotproduct");
}

VTKM_CONT vtkm::cont::DataSet DotProduct::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::Field primaryField = this->GetFieldFromDataSet(0, inDataSet);
  vtkm::cont::Field secondaryField = this->GetFieldFromDataSet(1, inDataSet);

  if (primaryField.GetData().GetNumberOfComponentsFlat() !=
      secondaryField.GetData().GetNumberOfComponentsFlat())
  {
    throw vtkm::cont::ErrorFilterExecution(
      "Primary and secondary arrays of DotProduct filter have different number of components.");
  }

  vtkm::cont::UnknownArrayHandle outArray;

  auto resolveArray = [&](const auto& concretePrimaryField) {
    outArray = DoDotProduct(concretePrimaryField, secondaryField);
  };
  this->CastAndCallVariableVecField(primaryField, resolveArray);

  return this->CreateResultField(inDataSet,
                                 this->GetOutputFieldName(),
                                 this->GetFieldFromDataSet(inDataSet).GetAssociation(),
                                 outArray);
}

} // namespace vector_analysis
} // namespace filter
} // namespace vtkm
