//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/MapFieldPermutation.h>

#include <vtkm/TypeList.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/Logging.h>

#include <vtkm/cont/internal/CastInvalidValue.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/filter/PolicyDefault.h>

namespace
{

template <typename T>
struct MapPermutationWorklet : vtkm::worklet::WorkletMapField
{
  T InvalidValue;

  explicit MapPermutationWorklet(T invalidValue)
    : InvalidValue(invalidValue)
  {
  }

  using ControlSignature = void(FieldIn permutationIndex, WholeArrayIn input, FieldOut output);

  template <typename InputPortalType, typename OutputType>
  VTKM_EXEC void operator()(vtkm::Id permutationIndex,
                            InputPortalType inputPortal,
                            OutputType& output) const
  {
    VTKM_STATIC_ASSERT(vtkm::HasVecTraits<OutputType>::value);
    if ((permutationIndex >= 0) && (permutationIndex < inputPortal.GetNumberOfValues()))
    {
      output = inputPortal.Get(permutationIndex);
    }
    else
    {
      output = this->InvalidValue;
    }
  }
};

struct DoMapFieldPermutation
{
  template <typename InputArrayType>
  void operator()(const InputArrayType& input,
                  const vtkm::cont::ArrayHandle<vtkm::Id>& permutation,
                  vtkm::cont::UnknownArrayHandle& output,
                  vtkm::Float64 invalidValue) const
  {
    using BaseComponentType = typename InputArrayType::ValueType::ComponentType;

    MapPermutationWorklet<BaseComponentType> worklet(
      vtkm::cont::internal::CastInvalidValue<BaseComponentType>(invalidValue));
    vtkm::cont::Invoker{}(
      worklet,
      permutation,
      input,
      output.ExtractArrayFromComponents<BaseComponentType>(vtkm::CopyFlag::Off));
  }
};

} // anonymous namespace

VTKM_FILTER_COMMON_EXPORT VTKM_CONT bool vtkm::filter::MapFieldPermutation(
  const vtkm::cont::Field& inputField,
  const vtkm::cont::ArrayHandle<vtkm::Id>& permutation,
  vtkm::cont::Field& outputField,
  vtkm::Float64 invalidValue)
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  vtkm::cont::UnknownArrayHandle outputArray = inputField.GetData().NewInstanceBasic();
  outputArray.Allocate(permutation.GetNumberOfValues());
  try
  {
    inputField.GetData().CastAndCallWithExtractedArray(
      DoMapFieldPermutation{}, permutation, outputArray, invalidValue);
    outputField = vtkm::cont::Field(inputField.GetName(), inputField.GetAssociation(), outputArray);
    return true;
  }
  catch (...)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Faild to map field " << inputField.GetName());
    return false;
  }
}

VTKM_FILTER_COMMON_EXPORT VTKM_CONT bool vtkm::filter::MapFieldPermutation(
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
