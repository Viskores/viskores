//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/internal/CastInvalidValue.h>
#include <vtkm/cont/internal/MapArrayPermutation.h>

#include <vtkm/worklet/WorkletMapField.h>


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

namespace vtkm
{
namespace cont
{
namespace internal
{

vtkm::cont::UnknownArrayHandle MapArrayPermutation(
  const vtkm::cont::UnknownArrayHandle& inputArray,
  const vtkm::cont::ArrayHandle<vtkm::Id>& permutation,
  vtkm::Float64 invalidValue)
{
  vtkm::cont::UnknownArrayHandle outputArray = inputArray.NewInstanceBasic();
  outputArray.Allocate(permutation.GetNumberOfValues());
  inputArray.CastAndCallWithExtractedArray(
    DoMapFieldPermutation{}, permutation, outputArray, invalidValue);
  return outputArray;
}

}
}
} // namespace vtkm::cont::internal
