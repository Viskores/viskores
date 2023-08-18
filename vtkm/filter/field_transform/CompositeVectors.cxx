//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/field_transform/CompositeVectors.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleRuntimeVec.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <vtkm/List.h>
#include <vtkm/TypeList.h>

namespace
{

// Extracts a component from an UnknownArrayHandle and returns the extracted component
// as an UnknownArrayHandle. Perhaps this functionality should be part of UnknownArrayHandle
// proper, but its use is probably rare. Note that this implementation makes some assumptions
// on its use in the CompositeVectors filter.
VTKM_CONT vtkm::cont::UnknownArrayHandle ExtractComponent(
  const vtkm::cont::UnknownArrayHandle& array,
  vtkm::IdComponent componentIndex)
{
  vtkm::cont::UnknownArrayHandle extractedComponentArray;
  auto resolveType = [&](auto componentExample) {
    using ComponentType = decltype(componentExample);
    if (array.IsBaseComponentType<ComponentType>())
    {
      extractedComponentArray =
        array.ExtractComponent<ComponentType>(componentIndex, vtkm::CopyFlag::Off);
    }
  };
  vtkm::ListForEach(resolveType, vtkm::TypeListBaseC{});
  return extractedComponentArray;
}

} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace field_transform
{

VTKM_CONT void CompositeVectors::SetFieldNameList(const std::vector<std::string>& fieldNameList,
                                                  vtkm::cont::Field::Association association)
{
  vtkm::IdComponent index = 0;
  for (auto& fieldName : fieldNameList)
  {
    this->SetActiveField(index, fieldName, association);
    ++index;
  }
}

VTKM_CONT vtkm::IdComponent CompositeVectors::GetNumberOfFields() const
{
  return this->GetNumberOfActiveFields();
}

VTKM_CONT vtkm::cont::DataSet CompositeVectors::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::IdComponent numComponents = this->GetNumberOfFields();
  if (numComponents < 1)
  {
    throw vtkm::cont::ErrorBadValue(
      "No input fields to combine into a vector for CompositeVectors.");
  }

  vtkm::cont::UnknownArrayHandle outArray;

  // Allocate output array to the correct type.
  vtkm::cont::Field firstField = this->GetFieldFromDataSet(0, inDataSet);
  vtkm::Id numValues = firstField.GetNumberOfValues();
  vtkm::cont::Field::Association association = firstField.GetAssociation();
  auto allocateOutput = [&](auto exampleComponent) {
    using ComponentType = decltype(exampleComponent);
    if (firstField.GetData().IsBaseComponentType<ComponentType>())
    {
      outArray = vtkm::cont::ArrayHandleRuntimeVec<ComponentType>{ numComponents };
    }
  };
  vtkm::ListForEach(allocateOutput, vtkm::TypeListBaseC{});
  if (!outArray.IsValid() || (outArray.GetNumberOfComponentsFlat() != numComponents))
  {
    throw vtkm::cont::ErrorBadType("Unable to allocate output array due to unexpected type.");
  }
  outArray.Allocate(numValues);

  // Iterate over all component fields and copy them into the output array.
  for (vtkm::IdComponent componentIndex = 0; componentIndex < numComponents; ++componentIndex)
  {
    vtkm::cont::Field inScalarField = this->GetFieldFromDataSet(componentIndex, inDataSet);
    if (inScalarField.GetData().GetNumberOfComponentsFlat() != 1)
    {
      throw vtkm::cont::ErrorBadValue("All input fields to CompositeVectors must be scalars.");
    }
    if (inScalarField.GetAssociation() != association)
    {
      throw vtkm::cont::ErrorBadValue(
        "All scalar fields must have the same association (point, cell, etc.).");
    }
    if (inScalarField.GetNumberOfValues() != numValues)
    {
      throw vtkm::cont::ErrorBadValue("Inconsistent number of field values.");
    }

    ExtractComponent(outArray, componentIndex).DeepCopyFrom(inScalarField.GetData());
  }

  return this->CreateResultField(inDataSet, this->GetOutputFieldName(), association, outArray);
}

} // namespace field_transform
} // namespace vtkm::filter
} // namespace vtkm
