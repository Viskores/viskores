//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Logging.h>

#include <vtkm/cont/internal/MapArrayPermutation.h>

#include <vtkm/filter/MapFieldPermutation.h>

VTKM_FILTER_CORE_EXPORT VTKM_CONT bool vtkm::filter::MapFieldPermutation(
  const vtkm::cont::Field& inputField,
  const vtkm::cont::ArrayHandle<vtkm::Id>& permutation,
  vtkm::cont::Field& outputField,
  vtkm::Float64 invalidValue)
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  try
  {
    vtkm::cont::UnknownArrayHandle outputArray =
      vtkm::cont::internal::MapArrayPermutation(inputField.GetData(), permutation, invalidValue);
    outputField = vtkm::cont::Field(inputField.GetName(), inputField.GetAssociation(), outputArray);
    return true;
  }
  catch (...)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Faild to map field " << inputField.GetName());
    return false;
  }
}

VTKM_FILTER_CORE_EXPORT VTKM_CONT bool vtkm::filter::MapFieldPermutation(
  const vtkm::cont::Field& inputField,
  const vtkm::cont::ArrayHandle<vtkm::Id>& permutation,
  vtkm::cont::DataSet& outputData,
  vtkm::Float64 invalidValue)
{
  vtkm::cont::Field outputField;
  bool success =
    vtkm::filter::MapFieldPermutation(inputField, permutation, outputField, invalidValue);
  if (success)
  {
    outputData.AddField(outputField);
  }
  return success;
}
