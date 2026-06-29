//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#ifndef viskores_filter_uncertainty_worklet_gaussian_InterpolateFieldWorklet_h
#define viskores_filter_uncertainty_worklet_gaussian_InterpolateFieldWorklet_h

#include <viskores/Math.h>
#include <viskores/VecTraits.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace detail
{

/// @brief Interpolate a point field to the expected isosurface crossing.
///
/// Shared by the independent and correlated Gaussian contour uncertainty
/// filters; linearly interpolates each component of an input point field
/// between the two edge endpoints at the expected crossing fraction.
struct InterpolateFieldWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn edgeIds,
                                FieldIn crossings,
                                WholeArrayIn inputArray,
                                FieldOut interpolated);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  template <typename InputPortalType, typename InterpolatedType>
  VISKORES_EXEC void operator()(const viskores::Id2& edgeIds,
                                viskores::Float64 crossing,
                                const InputPortalType& inputArray,
                                InterpolatedType& interpolated) const
  {
    using VecTraitsOut = viskores::VecTraits<InterpolatedType>;
    const viskores::IdComponent numComponents = VecTraitsOut::GetNumberOfComponents(interpolated);

    auto input0 = inputArray.Get(edgeIds[0]);
    auto input1 = inputArray.Get(edgeIds[1]);

    using VecTraitsIn = viskores::VecTraits<decltype(input0)>;
    VISKORES_ASSERT(VecTraitsIn::GetNumberOfComponents(input0) == numComponents);
    VISKORES_ASSERT(VecTraitsIn::GetNumberOfComponents(input1) == numComponents);

    for (viskores::IdComponent componentIndex = 0; componentIndex < numComponents; ++componentIndex)
    {
      auto value0 = VecTraitsIn::GetComponent(input0, componentIndex);
      auto value1 = VecTraitsIn::GetComponent(input1, componentIndex);
      VecTraitsOut::SetComponent(
        interpolated, componentIndex, viskores::Lerp(value0, value1, crossing));
    }
  }
};

/// @brief Invoke the interpolation worklet, producing a field at crossing positions.
template <typename InputArrayType>
VISKORES_CONT viskores::cont::UnknownArrayHandle InterpolateField(
  const InputArrayType& inputArray,
  const viskores::cont::UnknownArrayHandle& edgeIdsUnknown,
  const viskores::cont::ArrayHandle<viskores::Float64>& crossings)
{
  using ValueType = typename InputArrayType::ValueType;
  using ComponentType = typename viskores::VecTraits<ValueType>::BaseComponentType;

  viskores::cont::ArrayHandleRuntimeVec<ComponentType> outputArray(
    inputArray.GetNumberOfComponentsFlat());

  viskores::cont::ArrayHandle<viskores::Id2> edgeIds;
  edgeIdsUnknown.AsArrayHandle(edgeIds);
  VISKORES_ASSERT(edgeIds.GetNumberOfValues() == crossings.GetNumberOfValues());

  viskores::cont::Invoker invoke;
  invoke(InterpolateFieldWorklet{}, edgeIds, crossings, inputArray, outputArray);

  return outputArray;
}

} // namespace detail
} // namespace worklet
} // namespace viskores

#endif // viskores_filter_uncertainty_worklet_gaussian_InterpolateFieldWorklet_h
