//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ErrorBadType.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/UnknownArrayHandle.h>
#include <viskores/cont/internal/ArrayGetMonotonicity.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/BinaryOperators.h>

namespace viskores
{
namespace cont
{

namespace
{

struct StrictMonotonicity : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(WholeArrayIn, FieldOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename ArrayType>
  VISKORES_EXEC void operator()(const viskores::Id& idx,
                                const ArrayType& input,
                                viskores::UInt8& result) const
  {
    // Bit 0 tracks increasing order; bit 1 tracks decreasing order.
    if (idx == 0)
      result = 0x3;
    else if (input.Get(idx) > input.Get(idx - 1))
      result = 0x1;
    else if (input.Get(idx) < input.Get(idx - 1))
      result = 0x2;
    else
      result = 0x0;
  }
};

} // anonymous namespace

namespace internal
{

VISKORES_CONT_EXPORT
ArrayMonotonicity ArrayGetStrictMonotonicity(const viskores::cont::UnknownArrayHandle& input)
{
  if (input.GetNumberOfComponentsFlat() != 1)
  {
    throw viskores::cont::ErrorBadType(
      "ArrayGetStrictMonotonicity only supported for scalar arrays.");
  }

  if (input.GetNumberOfValues() < 2)
    return ArrayMonotonicity::Increasing;

  viskores::cont::ArrayHandle<viskores::UInt8> result;
  auto resolveType = [&](auto recombineVec)
  {
    VISKORES_ASSERT(recombineVec.GetNumberOfComponents() == 1);
    auto componentArray = recombineVec.GetComponentArray(0);
    viskores::cont::Invoker invoke;
    invoke(StrictMonotonicity{}, componentArray, result);
  };
  input.CastAndCallWithExtractedArray(resolveType);

  viskores::UInt8 monotonicity =
    viskores::cont::Algorithm::Reduce(result, viskores::UInt8(0x3), viskores::BitwiseAnd{});
  if (monotonicity == 0x1)
    return ArrayMonotonicity::Increasing;
  if (monotonicity == 0x2)
    return ArrayMonotonicity::Decreasing;
  return ArrayMonotonicity::NotMonotonic;
}

} // namespace internal
} // namespace cont
} // namespace viskores
