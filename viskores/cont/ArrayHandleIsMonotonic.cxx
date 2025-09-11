//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/Invoker.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace cont
{

namespace
{
struct MonotonicIncreasing : public viskores::worklet::WorkletMapField
{
  MonotonicIncreasing() = default;

  using ControlSignature = void(WholeArrayIn, FieldOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename ArrayType>
  VISKORES_EXEC void operator()(const viskores::Id& idx, const ArrayType& input, bool& result) const
  {
    if (idx == 0)
      result = true;
    else
      result = input.Get(idx) >= input.Get(idx - 1);
  }
};

struct MonotonicDecreasing : public viskores::worklet::WorkletMapField
{
  MonotonicDecreasing() = default;

  using ControlSignature = void(WholeArrayIn, FieldOut);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename ArrayType>
  VISKORES_EXEC void operator()(const viskores::Id& idx, const ArrayType& input, bool& result) const
  {
    if (idx == 0)
      result = true;
    else
      result = input.Get(idx) <= input.Get(idx - 1);
  }
};
} //anonymous namespace


template <typename T>
VISKORES_CONT bool IsMonotonicIncreasing(
  const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& input)
{
  viskores::Id numValues = input.GetNumberOfValues();
  if (numValues < 2)
    return true;

  viskores::cont::Invoker invoke;

  viskores::cont::ArrayHandle<bool> result;
  invoke(MonotonicIncreasing{}, input, result);
  return viskores::cont::Algorithm::Reduce(result, true, viskores::LogicalAnd());
}

template <typename T>
VISKORES_CONT bool IsMonotonicDecreasing(
  const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagBasic>& input)
{
  viskores::Id numValues = input.GetNumberOfValues();
  if (numValues < 2)
    return true;

  viskores::cont::Invoker invoke;

  viskores::cont::ArrayHandle<bool> result;
  invoke(MonotonicDecreasing{}, input, result);
  return viskores::cont::Algorithm::Reduce(result, true, viskores::LogicalAnd());
}

}
} // namespace viskores::cont
