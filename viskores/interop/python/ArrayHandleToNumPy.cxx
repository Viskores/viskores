//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/interop/python/ArrayHandleToNumPy.h>

namespace viskores
{
namespace interop
{
namespace python
{

nb::object ArrayHandleToNumPy(const viskores::cont::UnknownArrayHandle& array)
{
  if (!array.IsValid())
  {
    throw viskores::cont::ErrorBadValue("UnknownArrayHandle is empty (no underlying array).");
  }

  nb::object result;
  if (TryAnyRuntimeVecArrayToNumPy(array, result))
  {
    return result;
  }

  // The storage type is not directly representable as a RuntimeVec. Deep-copy to
  // a Viskores-allocated basic array so the zero-copy view path can succeed.
  viskores::cont::UnknownArrayHandle copy = array.NewInstanceBasic();
  copy.DeepCopyFrom(array);
  if (TryAnyRuntimeVecArrayToNumPy(copy, result))
  {
    return result;
  }

  throw viskores::cont::ErrorBadValue(
    "UnknownArrayHandle component type is not supported for NumPy conversion.");
}

} // namespace python
} // namespace interop
} // namespace viskores
