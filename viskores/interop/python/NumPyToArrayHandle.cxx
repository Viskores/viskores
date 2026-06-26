//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/interop/python/NumPyToArrayHandle.h>

namespace viskores
{
namespace interop
{
namespace python
{

viskores::cont::UnknownArrayHandle NumPyToArrayHandle(nb::handle object, bool allowCopy)
{
  nb::object numpy = nb::module_::import_("numpy");
  // Normalize the input through numpy.asarray so we accept Python lists,
  // scalars, and other array-like objects in addition to NumPy arrays.
  nb::object array = numpy.attr("asarray")(object);

  nb::object dtypeObject = array.attr("dtype");
  // Reject dtype kinds that don't have a DLPack representation: the nb::ndarray
  // cast below would otherwise surface them as an opaque std::bad_cast. Numeric
  // kinds (b=bool, i=signed int, u=unsigned int, f=float, c=complex) pass
  // through; bool and complex are then rejected by the dispatch table with a
  // clean dtype name because the binding doesn't support them as scalar
  // component types.
  const std::string dtypeKind = nb::cast<std::string>(dtypeObject.attr("kind"));
  if (dtypeKind != "b" && dtypeKind != "i" && dtypeKind != "u" && dtypeKind != "f" &&
      dtypeKind != "c")
  {
    const std::string dtypeName = nb::cast<std::string>(dtypeObject.attr("name"));
    throw viskores::cont::ErrorBadValue("NumPy arrays with " + dtypeName +
                                        " dtype are not supported.");
  }
  // Non-native byte-order dtypes (e.g., '>f4' on a little-endian host) cannot
  // be cast to nb::ndarray<nb::numpy, ...> and would surface as an opaque
  // std::bad_cast. Reject up front with a clean message.
  if (!nb::cast<bool>(dtypeObject.attr("isnative")))
  {
    throw viskores::cont::ErrorBadValue(
      "NumPy arrays with non-native byte-order dtype are not supported. "
      "Convert with array.astype(array.dtype.newbyteorder('=')) before passing.");
  }

  // Cast with nb::ro so shape and dtype are accessible regardless of
  // writability; the shareability check reports write-access errors separately.
  auto inspector = nb::cast<nb::ndarray<nb::numpy, nb::ro>>(array);
  const size_t ndim = inspector.ndim();
  // Use dtype.name (e.g., "bool", "float16", "complex128") for the error message
  // instead of dtype.str (e.g., "|b1", "<f2", "<c16"); the human-readable form
  // is what NumPy users expect.
  const std::string dtypeName = nb::cast<std::string>(dtypeObject.attr("name"));
  const NumPyDTypeEntry& entry = FindNumPyDTypeEntry(inspector.dtype(), dtypeName.c_str());

  const ShareableCheck shareability = CheckShareableNumPyArray(array);
  if (shareability != ShareableCheck::Shareable)
  {
    if (!allowCopy)
    {
      ThrowUnshareableNumPyArrayReason(shareability);
    }
    // ndarray.copy() always returns a writable, C-contiguous array. (Unlike
    // numpy.ascontiguousarray, which returns the input unchanged when it is
    // already contiguous and therefore preserves a read-only flag.) Shape and
    // dtype are preserved, so the values captured from `inspector` remain
    // valid for dispatch.
    array = array.attr("copy")();
  }

  if (ndim == 1)
  {
    return entry.Scalar(array, static_cast<viskores::Id>(inspector.shape(0)));
  }

  if (ndim == 2)
  {
    return entry.Vector(array,
                        static_cast<viskores::Id>(inspector.shape(0)),
                        static_cast<viskores::IdComponent>(inspector.shape(1)));
  }

  throw viskores::cont::ErrorBadValue(
    "Only one-dimensional scalar arrays and two-dimensional vector arrays are supported.");
}

} // namespace python
} // namespace interop
} // namespace viskores
