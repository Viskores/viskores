//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

// Convert a NumPy ndarray to a Viskores UnknownArrayHandle.
// Requires nanobind (nanobind/ndarray.h) and Python development headers.

#ifndef viskores_interop_python_NumPyToArrayHandle_h
#define viskores_interop_python_NumPyToArrayHandle_h

// VISKORES_PYTHON_NUMPY_SCALAR_TYPES, NumPyDType<T>, and the export macro live here.
#include <viskores/interop/python/ArrayHandleToNumPy.h>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>

#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/Error.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <memory>
#include <string>

namespace nb = nanobind;

namespace viskores
{
namespace interop
{
namespace python
{

// Own the Python-side lifetime state for a NumPy array shared with Viskores.
//
// Viskores stores only a raw data pointer in the ArrayHandleBasic buffer. This
// owner is passed to Viskores as the buffer container so the NumPy array remains
// alive while any Viskores buffer reference can still use that pointer.
// Releasing the final Viskores buffer reference calls ReleasePythonBufferOwner,
// which deletes this object, releases the Python buffer export, and drops the
// Python reference to the NumPy array, allowing it to be deleted once all other
// Python references are dropped.
class PyBufferOwner
{
public:
  // Hold a Python reference plus a writable buffer export for the same array.
  // PyObject_GetBuffer/PyBuffer_Release have been part of the CPython Limited
  // API since 3.11, so they remain callable when the binding is built with
  // VISKORES_PYTHON_STABLE_ABI=ON (which itself requires Python 3.12+).
  explicit PyBufferOwner(nb::handle object)
    : Object(nb::borrow<nb::object>(object))
  {
    if (PyObject_GetBuffer(this->Object.ptr(), &this->Buffer, PyBUF_WRITABLE) < 0)
    {
      throw nb::python_error();
    }
    this->HasBuffer = true;
  }

  ~PyBufferOwner()
  {
    if (this->HasBuffer)
    {
      PyBuffer_Release(&this->Buffer);
    }
  }

  PyBufferOwner(const PyBufferOwner&) = delete;
  PyBufferOwner& operator=(const PyBufferOwner&) = delete;

  template <typename ValueType>
  ValueType* Data() const
  {
    return static_cast<ValueType*>(this->Buffer.buf);
  }

private:
  nb::object Object;
  Py_buffer Buffer = {};
  bool HasBuffer = false;
};

inline void ReleasePythonBufferOwner(void* container)
{
  // Viskores can release buffers outside a direct Python call path. Acquire
  // Python's Global Interpreter Lock before releasing the Python buffer export
  // and dropping the object reference.
  nb::gil_scoped_acquire gil;
  PyBufferOwner* owner = static_cast<PyBufferOwner*>(container);
  delete owner;
}

// Result of checking whether a NumPy array's layout can be shared with Viskores.
// `Shareable` means it can; the other values name the specific constraint that
// prevents sharing.
//
// Layout flags are read from `array.flags.<name>` rather than from nb::ndarray
// because nb::ndarray does not expose a runtime accessor for writability or
// alignment. (Contiguity could be inferred from strides, but going through
// flags keeps all three checks consistent.)
enum class ShareableCheck
{
  Shareable,
  NotCContiguous,
  NotAligned,
  NotWriteable,
};

inline ShareableCheck CheckShareableNumPyArray(nb::handle array)
{
  nb::object flags = array.attr("flags");
  if (!nb::cast<bool>(flags.attr("c_contiguous")))
  {
    return ShareableCheck::NotCContiguous;
  }
  if (!nb::cast<bool>(flags.attr("aligned")))
  {
    return ShareableCheck::NotAligned;
  }
  if (!nb::cast<bool>(flags.attr("writeable")))
  {
    return ShareableCheck::NotWriteable;
  }
  return ShareableCheck::Shareable;
}

// Throw explaining why a NumPy array's layout cannot be shared.
// Pre: reason != ShareableCheck::Shareable.
[[noreturn]] inline void ThrowUnshareableNumPyArrayReason(ShareableCheck reason)
{
  switch (reason)
  {
    case ShareableCheck::NotCContiguous:
      throw viskores::cont::ErrorBadValue(
        "NumPy array input must be C-contiguous to share with Viskores. "
        "Pass `allow_copy=True` to make a contiguous copy automatically.");
    case ShareableCheck::NotAligned:
      throw viskores::cont::ErrorBadValue(
        "NumPy array input must be aligned to share with Viskores. "
        "Pass `allow_copy=True` to make an aligned copy automatically.");
    case ShareableCheck::NotWriteable:
      throw viskores::cont::ErrorBadValue(
        "NumPy array input must be writable to share with Viskores. "
        "Pass `allow_copy=True` to make a writable copy automatically.");
    case ShareableCheck::Shareable:
      break;
  }
  throw viskores::cont::ErrorBadValue("Unknown NumPy shareability failure.");
}

// Build a basic ArrayHandle that shares storage with a NumPy array.
template <typename ValueType>
viskores::cont::ArrayHandleBasic<ValueType> SharedBasicArrayFromNumPy(nb::handle array,
                                                                      viskores::Id numberOfValues)
{
  auto owner = std::make_unique<PyBufferOwner>(array);
  ValueType* data = owner->Data<ValueType>();
  // The constructor's default InvalidRealloc reallocater causes any Viskores
  // resize attempt on the Python-owned buffer to throw rather than reallocate.
  // Viskores calls ReleasePythonBufferOwner when the last shared buffer
  // reference is destroyed, not necessarily when one Python UnknownArrayHandle
  // wrapper goes away.
  viskores::cont::ArrayHandleBasic<ValueType> result(
    data, owner.get(), numberOfValues, &ReleasePythonBufferOwner);
  owner.release();
  return result;
}

template <typename ValueType>
viskores::cont::UnknownArrayHandle ArrayFromNumPyScalarArray(nb::handle array,
                                                             viskores::Id numberOfValues)
{
  return SharedBasicArrayFromNumPy<ValueType>(array, numberOfValues);
}

template <typename ComponentType>
viskores::cont::UnknownArrayHandle ArrayFromNumPyVectorArray(
  nb::handle array,
  viskores::Id numberOfValues,
  viskores::IdComponent numberOfComponents)
{
  if (numberOfComponents < 1)
  {
    throw viskores::cont::ErrorBadValue("2D NumPy array input must have at least one component.");
  }
  auto componentArray =
    SharedBasicArrayFromNumPy<ComponentType>(array, numberOfValues * numberOfComponents);
  return viskores::cont::ArrayHandleRuntimeVec<ComponentType>(numberOfComponents, componentArray);
}

// Dispatch entry for a single supported NumPy scalar dtype. Matching uses the
// dlpack::dtype struct so we avoid string round-trips through Python; the
// human-readable dtype name in the error path is taken from the input array's
// dtype.name attribute.
struct NumPyDTypeEntry
{
  nb::dlpack::dtype DType;
  viskores::cont::UnknownArrayHandle (*Scalar)(nb::handle, viskores::Id);
  viskores::cont::UnknownArrayHandle (*Vector)(nb::handle, viskores::Id, viskores::IdComponent);
};

#define VISKORES_PYTHON_NUMPY_DTYPE_ENTRY(ValueType, DTypeName) \
  NumPyDTypeEntry{ nb::dtype<ValueType>(),                      \
                   &ArrayFromNumPyScalarArray<ValueType>,       \
                   &ArrayFromNumPyVectorArray<ValueType> },

constexpr NumPyDTypeEntry NumPyDTypeTable[] = { VISKORES_PYTHON_NUMPY_SCALAR_TYPES(
  VISKORES_PYTHON_NUMPY_DTYPE_ENTRY) };
#undef VISKORES_PYTHON_NUMPY_DTYPE_ENTRY

// Find the dispatch entry for a dlpack dtype, or throw if the dtype is not
// one of the supported scalar types.
inline const NumPyDTypeEntry& FindNumPyDTypeEntry(nb::dlpack::dtype dtype,
                                                  const char* dtypeNameForError)
{
  for (const NumPyDTypeEntry& entry : NumPyDTypeTable)
  {
    if (entry.DType == dtype)
    {
      return entry;
    }
  }
  throw viskores::cont::ErrorBadValue(std::string("Unsupported NumPy dtype: ") + dtypeNameForError);
}

// Wrap a Python array-like object as a Viskores UnknownArrayHandle.
// The input must be C-contiguous, aligned, and writable to share storage; pass
// allowCopy=true to make a copy when those constraints are not met.
inline viskores::cont::UnknownArrayHandle NumPyToArrayHandle(nb::handle object, bool allowCopy)
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

#endif // viskores_interop_python_NumPyToArrayHandle_h
