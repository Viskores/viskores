//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include "viskores_python_bindings.h"

#include <nanobind/ndarray.h>
#include <nanobind/stl/string.h>

#include <viskores/TypeList.h>
#include <viskores/Types.h>
#include <viskores/VecTraits.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/DefaultTypes.h>
#include <viskores/cont/StorageList.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_set>

namespace viskores::python::bindings
{
namespace
{

// UnknownArrayHandle/NumPy conversion shares storage for dense numeric arrays
// when the NumPy input has a compatible C-contiguous writable layout. Dense
// NumPy views returned from Viskores are read-only and shared only when the
// underlying buffer is one this binding originally created from a NumPy array.
// The conversion code below supports:
//
//   * scalar ArrayHandle<T> from a 1D numeric NumPy array,
//   * runtime-width ArrayHandleRuntimeVec<T> from a 2D numeric NumPy array.
//
// Python to Viskores flow:
//
//   array_from_numpy first lets NumPy normalize dtype and shape, then dispatches
//   by dtype to a concrete Viskores component type. A 1D NumPy array becomes an
//   ArrayHandleBasic<T>. A 2D NumPy array becomes one flat ArrayHandleBasic<T>
//   plus an ArrayHandleRuntimeVec<T> that interprets rows as vector values.
//
// Viskores to Python flow:
//
//   asnumpy and UnknownArrayHandle.asnumpy return a zero-copy NumPy view when the
//   source ArrayHandleBasic<T> or ArrayHandleRuntimeVec<T> wraps a NumPy buffer
//   originally registered by array_from_numpy. Otherwise the data is copied into
//   a fresh NumPy array. Other supported Viskores array layouts are resolved
//   through CastAndCallForTypes and always copied.
//
// Why the zero-copy view path is restricted to Python-owned buffers
//
//   The view path needs the underlying data pointer to stay valid for the lifetime
//   of the NumPy view. For buffers created from NumPy via array_from_numpy this is
//   guaranteed by two properties of the construction:
//     1. The buffer container is a PyBufferOwner; the deleter is
//        ReleasePythonBufferOwner. The Python NumPy array is reference-held by
//        PyBufferOwner, so the data pointer cannot be freed while a Viskores
//        Buffer holding that container is alive.
//     2. The reallocater is the default InvalidRealloc (set by
//        ArrayHandleBasic's container/deleter constructor), which throws
//        ErrorBadAllocation on any size-changing call. Viskores cannot silently
//        move the data pointer to a new allocation.
//   Holding the source ArrayHandle alive in the view's owner capsule is therefore
//   sufficient. We do *not* hold a viskores::cont::Token: a long-lived read token
//   would block any subsequent Viskores write on that buffer until Python garbage
//   collection releases the view, which in user code looks like a deadlock.
//
//   For buffers we did not create (e.g., a filter output allocated by Viskores),
//   neither property holds. The buffer can be reallocated in place by some other
//   holder calling Allocate(), invalidating any direct pointer NumPy held. Those
//   buffers go through the copy path.
//
// Detecting Python-owned buffers
//
//   We register the data pointer of every NumPy array wrapped by
//   array_from_numpy in PythonOwnedDataPointers, and unregister it in
//   ReleasePythonBufferOwner. At view-construction time we compare the source
//   ArrayHandle's read pointer against that set. Detection through Viskores'
//   public API (e.g., comparing the buffer's deleter against
//   ReleasePythonBufferOwner) is not currently possible because BufferInfo does
//   not expose its deleter. The side-table is the smallest workaround that does
//   not modify Viskores. All access is serialized by the Python GIL, which is
//   held during array_from_numpy, asnumpy, and ReleasePythonBufferOwner.

// Central NumPy dtype mapping.
//
// X-macro listing every NumPy scalar dtype this binding accepts on input and
// emits on output. Each entry pairs a Viskores scalar type with its canonical
// NumPy dtype string. The list is expanded by the dispatch table, the
// runtime-vec output try-chain, and NumPyDType<T>().
#define VISKORES_PYTHON_NUMPY_SCALAR_TYPES(V) \
  V(viskores::Int8, "int8")                   \
  V(viskores::Int16, "int16")                 \
  V(viskores::Int32, "int32")                 \
  V(viskores::Int64, "int64")                 \
  V(viskores::UInt8, "uint8")                 \
  V(viskores::UInt16, "uint16")               \
  V(viskores::UInt32, "uint32")               \
  V(viskores::UInt64, "uint64")               \
  V(viskores::Float32, "float32")             \
  V(viskores::Float64, "float64")

// Map a Viskores component type to its canonical NumPy dtype string. Used to
// pass dtype to numpy.empty when allocating output arrays.
template <typename ValueType>
constexpr const char* NumPyDType()
{
  using ComponentType = std::remove_cv_t<ValueType>;
#define VISKORES_PYTHON_NUMPY_DTYPE_CASE(Type, Name)                    \
  if constexpr (std::is_same_v<ComponentType, Type>)                    \
  {                                                                     \
    return Name;                                                        \
  }                                                                     \
  else
  VISKORES_PYTHON_NUMPY_SCALAR_TYPES(VISKORES_PYTHON_NUMPY_DTYPE_CASE)
  {
    static_assert(sizeof(ComponentType) == 0, "Unsupported NumPy component type.");
    return nullptr;
  }
#undef VISKORES_PYTHON_NUMPY_DTYPE_CASE
}

// Side table of data pointers whose Viskores buffer container is a PyBufferOwner.
// Used by the NumPy output path to decide between a zero-copy view (entry present)
// and a defensive copy (entry absent). All access happens under the Python GIL;
// see the file header comment for the lifetime/safety reasoning.
//
// A multiset, not a set: when the same NumPy array is wrapped more than once,
// each wrapper registers an entry, and only the *last* release removes the
// pointer from view consideration. Using a plain set would incorrectly mark the
// pointer non-Python-owned as soon as any one wrapper is destroyed, even when
// other wrappers are still keeping the buffer alive.
std::unordered_multiset<const void*>& PythonOwnedDataPointers()
{
  static std::unordered_multiset<const void*> table;
  return table;
}

// Own the Python-side lifetime state for a NumPy array shared with Viskores.
//
// Viskores stores only a raw data pointer in the ArrayHandleBasic buffer. This
// owner is passed to Viskores as the buffer container so the NumPy array remains
// alive while any Viskores buffer reference can still use that pointer.
// Releasing the final Viskores buffer reference calls ReleasePythonBufferOwner,
// which deletes this object, releases the Python buffer export, and drops the
// Python reference to the NumPy array.
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

  void* RawData() const { return this->Buffer.buf; }

private:
  nb::object Object;
  Py_buffer Buffer = {};
  bool HasBuffer = false;
};

void ReleasePythonBufferOwner(void* container)
{
  // Viskores can release buffers outside a direct Python call path. Acquire
  // Python's Global Interpreter Lock before releasing the Python buffer export
  // and dropping the object reference.
  nb::gil_scoped_acquire gil;
  PyBufferOwner* owner = static_cast<PyBufferOwner*>(container);
  // Drop one entry for this owner's data pointer. Use erase(iterator) (not
  // erase(value)) so that other wrappers of the same NumPy array — each
  // contributing its own entry — keep the pointer registered. The pointer is
  // only considered non-Python-owned again after every wrapper is gone.
  auto& table = PythonOwnedDataPointers();
  auto it = table.find(owner->RawData());
  if (it != table.end())
  {
    table.erase(it);
  }
  delete owner;
}

// Reason a NumPy array's layout cannot be shared directly with Viskores, or
// `Shareable` if it can.
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

ShareableCheck CheckShareableNumPyArray(nb::handle array)
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

// Throw a runtime_error explaining why a NumPy array's layout cannot be shared.
// Pre: reason != ShareableCheck::Shareable.
[[noreturn]] void ThrowUnshareableNumPyArrayReason(ShareableCheck reason)
{
  switch (reason)
  {
    case ShareableCheck::NotCContiguous:
      throw std::runtime_error(
        "NumPy array input must be C-contiguous to share with Viskores. "
        "Pass `allow_copy=True` to make a contiguous copy automatically.");
    case ShareableCheck::NotAligned:
      throw std::runtime_error(
        "NumPy array input must be aligned to share with Viskores. "
        "Pass `allow_copy=True` to make an aligned copy automatically.");
    case ShareableCheck::NotWriteable:
      throw std::runtime_error(
        "NumPy array input must be writable to share with Viskores. "
        "Pass `allow_copy=True` to make a writable copy automatically.");
    case ShareableCheck::Shareable:
      break; // unreachable per precondition
  }
  throw std::runtime_error("Unknown NumPy shareability failure.");
}

// Build a basic ArrayHandle that shares storage with a NumPy array.
template <typename ValueType>
viskores::cont::ArrayHandleBasic<ValueType> SharedBasicArrayFromNumPy(nb::handle array,
                                                                      viskores::Id numberOfValues)
{
  auto owner = std::make_unique<PyBufferOwner>(array);
  ValueType* data = owner->Data<ValueType>();
  // ArrayHandleBasic takes ownership of the PyBufferOwner as its container.
  // The default reallocater is InvalidRealloc, which throws on any size-changing
  // call. Combined with Python ownership of the underlying memory, this means
  // the data pointer is stable for the lifetime of the buffer.
  // Viskores calls ReleasePythonBufferOwner when the last shared buffer
  // reference is destroyed, not necessarily when one Python UnknownArrayHandle
  // wrapper goes away.
  viskores::cont::ArrayHandleBasic<ValueType> result(
    data, owner.get(), numberOfValues, &ReleasePythonBufferOwner);
  owner.release();
  // Register the data pointer only after the ArrayHandle has taken ownership of
  // the PyBufferOwner. If insert throws (rare bad_alloc), `result` going out of
  // scope triggers ReleasePythonBufferOwner, which will erase the (absent) entry
  // as a no-op — no leak in the side table.
  PythonOwnedDataPointers().insert(data);
  return result;
}

// Build an UnknownArrayHandle for a 1D NumPy array with scalar values.
// `numberOfValues` is the row count from the NumPy shape, captured by the
// caller before any copy fallback so the dispatch table only sees the
// post-copy array.
template <typename ValueType>
viskores::cont::UnknownArrayHandle ArrayFromNumPyScalarArray(nb::handle array,
                                                             viskores::Id numberOfValues)
{
  return SharedBasicArrayFromNumPy<ValueType>(array, numberOfValues);
}

// Build an UnknownArrayHandle for a 2D NumPy array with runtime-width vector rows.
template <typename ComponentType>
viskores::cont::UnknownArrayHandle ArrayFromNumPyVectorArray(nb::handle array,
                                                             viskores::Id numberOfValues,
                                                             viskores::IdComponent numberOfComponents)
{
  if (numberOfComponents < 1)
  {
    throw std::runtime_error("2D NumPy array input must have at least one component.");
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

#define VISKORES_PYTHON_NUMPY_DTYPE_ENTRY(ValueType, DTypeName)             \
  NumPyDTypeEntry{ nb::dtype<ValueType>(),                                  \
                   &ArrayFromNumPyScalarArray<ValueType>,                   \
                   &ArrayFromNumPyVectorArray<ValueType> },
constexpr NumPyDTypeEntry NumPyDTypeTable[] = { VISKORES_PYTHON_NUMPY_SCALAR_TYPES(
  VISKORES_PYTHON_NUMPY_DTYPE_ENTRY) };
#undef VISKORES_PYTHON_NUMPY_DTYPE_ENTRY

// Find the dispatch entry for a dlpack dtype, or throw if the dtype is not
// one of the supported scalar types.
const NumPyDTypeEntry& FindNumPyDTypeEntry(nb::dlpack::dtype dtype, const char* dtypeNameForError)
{
  for (const NumPyDTypeEntry& entry : NumPyDTypeTable)
  {
    if (entry.DType == dtype)
    {
      return entry;
    }
  }

  throw std::runtime_error(std::string("Unsupported NumPy dtype: ") + dtypeNameForError);
}

// Convert a Python array-like object to an UnknownArrayHandle.
//
// `allowCopy` controls behavior when the input's layout cannot be shared
// directly (non-contiguous, misaligned, or read-only):
//   * allowCopy=false (default): raise an error. The caller keeps full control
//     over allocations and avoids hidden copies.
//   * allowCopy=true: produce a contiguous, writable copy and share storage
//     with that copy. The original NumPy array is not connected to the
//     resulting ArrayHandle, so subsequent mutations of the original do not
//     propagate. A copy is only made when the input is not directly shareable;
//     a shareable input still takes the zero-copy path.
viskores::cont::UnknownArrayHandle ArrayFromNumPy(nb::handle object, bool allowCopy)
{
  nb::object numpy = nb::module_::import_("numpy");
  // Normalize the input through numpy.asarray so we accept Python lists, scalars,
  // and other array-like objects in addition to NumPy arrays.
  nb::object array = numpy.attr("asarray")(object);

  nb::object dtypeObject = array.attr("dtype");
  // Reject dtype kinds that don't have a DLPack representation: the nb::ndarray
  // cast below would otherwise surface them as an opaque std::bad_cast. Numeric
  // kinds (b=bool, i=signed int, u=unsigned int, f=float, c=complex) pass
  // through; bool and complex are then rejected by the dispatch table with a
  // clean dtype name because the binding doesn't support them as scalar
  // component types.
  const std::string dtypeKind = nb::cast<std::string>(dtypeObject.attr("kind"));
  if (dtypeKind != "b" && dtypeKind != "i" && dtypeKind != "u" &&
      dtypeKind != "f" && dtypeKind != "c")
  {
    const std::string dtypeName = nb::cast<std::string>(dtypeObject.attr("name"));
    throw std::runtime_error(
      "NumPy arrays with " + dtypeName + " dtype are not supported.");
  }
  // Non-native byte-order dtypes (e.g., '>f4' on a little-endian host) cannot
  // be cast to nb::ndarray<nb::numpy, ...> and would surface as an opaque
  // std::bad_cast. Reject up front with a clean message.
  if (!nb::cast<bool>(dtypeObject.attr("isnative")))
  {
    throw std::runtime_error(
      "NumPy arrays with non-native byte-order dtype are not supported. "
      "Convert with array.astype(array.dtype.newbyteorder('=')) before passing.");
  }

  // Use nb::ndarray (read-only access; writability is checked separately so we
  // can produce a precise error message before considering the copy fallback)
  // for ndim, shape, and dtype inspection. This replaces direct PyObject_*
  // calls and gives type-safe access to the dlpack dtype struct used by the
  // dispatch table.
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

  throw std::runtime_error("Only one-dimensional scalar arrays and two-dimensional vector arrays "
                           "are supported.");
}

// Allocate a fresh, uninitialized NumPy array of the given shape and dtype.
// 1D when numberOfComponents == 1; otherwise 2D with shape (values, components).
template <typename ComponentType>
nb::object AllocateNumPyArray(size_t numberOfValues, size_t numberOfComponents)
{
  nb::object numpy = nb::module_::import_("numpy");
  const char* dtype = NumPyDType<ComponentType>();
  nb::object shape = (numberOfComponents == 1)
    ? nb::cast(numberOfValues)
    : nb::object(nb::make_tuple(numberOfValues, numberOfComponents));
  return numpy.attr("empty")(shape, nb::arg("dtype") = dtype);
}

// RAII wrapper for a writable Py_buffer view. Used to write directly into a
// NumPy array's storage without going through the boxed-Python-object path of
// numpy.array(list, dtype=...).
class WritableBufferView
{
public:
  explicit WritableBufferView(nb::handle pyArray)
  {
    if (PyObject_GetBuffer(pyArray.ptr(), &this->Buffer, PyBUF_WRITABLE | PyBUF_C_CONTIGUOUS) < 0)
    {
      throw nb::python_error();
    }
  }
  ~WritableBufferView() { PyBuffer_Release(&this->Buffer); }
  WritableBufferView(const WritableBufferView&) = delete;
  WritableBufferView& operator=(const WritableBufferView&) = delete;

  template <typename T>
  T* Data() const { return static_cast<T*>(this->Buffer.buf); }

private:
  Py_buffer Buffer = {};
};

template <typename ValueType>
nb::object BasicArrayToNumPy(const viskores::cont::ArrayHandleBasic<ValueType>& array);

// Convert a concrete scalar or fixed-width vector ArrayHandle to a dense NumPy
// array. For basic storage we delegate to the zero-copy view path; for other
// storages we allocate a fresh NumPy array and write components directly
// through its buffer pointer (avoiding the per-component Python object boxing
// that a list-of-numerics intermediate would incur).
template <typename ArrayHandleType>
nb::object ArrayHandleToNumPy(const ArrayHandleType& array)
{
  if constexpr (std::is_same<typename ArrayHandleType::StorageTag,
                             viskores::cont::StorageTagBasic>::value)
  {
    return BasicArrayToNumPy(
      viskores::cont::ArrayHandleBasic<typename ArrayHandleType::ValueType>(array));
  }

  using ValueType = typename ArrayHandleType::ValueType;
  using Traits = viskores::VecTraits<ValueType>;
  using ComponentType = typename Traits::BaseComponentType;

  const viskores::Id numberOfValues = array.GetNumberOfValues();
  const viskores::IdComponent numberOfComponents = array.GetNumberOfComponentsFlat();

  nb::object result = AllocateNumPyArray<ComponentType>(static_cast<size_t>(numberOfValues),
                                                        static_cast<size_t>(numberOfComponents));
  WritableBufferView destination(result);
  ComponentType* destinationData = destination.Data<ComponentType>();

  auto portal = array.ReadPortal();
  for (viskores::Id index = 0; index < numberOfValues; ++index)
  {
    const ValueType value = portal.Get(index);
    for (viskores::IdComponent component = 0; component < numberOfComponents; ++component)
    {
      destinationData[index * numberOfComponents + component] =
        static_cast<ComponentType>(Traits::GetComponent(value, component));
    }
  }
  return result;
}

// NumPy views returned from Viskores need an owner object too. The capsule that
// owns this object keeps the Viskores ArrayHandle alive for the lifetime of the
// Python view. The data pointer remains valid because the buffer is Python-owned
// (see file header for the safety argument); we deliberately do not hold a
// viskores::cont::Token, which would block subsequent Viskores writes on the
// buffer until the Python view is garbage-collected.
template <typename ArrayHandleType>
class BasicArrayViewOwner
{
public:
  using ValueType = typename ArrayHandleType::ValueType;
  using ComponentType = typename viskores::VecTraits<ValueType>::BaseComponentType;

  // Hold the ArrayHandle for the lifetime of the NumPy view. The data pointer
  // is captured once at construction; pointer stability is provided by Python
  // ownership of the buffer plus InvalidRealloc on the buffer's reallocater.
  explicit BasicArrayViewOwner(const ArrayHandleType& array)
    : Array(array)
    , Data(this->Array.GetReadPointer())
  {
  }

  const ComponentType* GetData() const
  {
    // BaseComponentType is the layout-equivalent scalar type for both T and
    // Vec<T,N>. Viskores' Vec<T,N> is a trivially-copyable POD with no padding,
    // so the cast yields a contiguous component pointer that NumPy can view as
    // an (N, components) array.
    return reinterpret_cast<const ComponentType*>(this->Data);
  }

private:
  ArrayHandleType Array;
  const ValueType* Data = nullptr;
};

template <typename ComponentType>
class RuntimeVecArrayViewOwner
{
public:
  // Hold the runtime-vector ArrayHandle and its components ArrayHandle for the
  // lifetime of a 2D NumPy view. Same lifetime model as BasicArrayViewOwner: no
  // long-lived read Token; pointer stability comes from Python ownership.
  explicit RuntimeVecArrayViewOwner(
    const viskores::cont::ArrayHandleRuntimeVec<ComponentType>& array)
    : Array(array)
    , ComponentsArray(this->Array.GetComponentsArray())
    , Data(this->ComponentsArray.GetReadPointer())
  {
  }

  const ComponentType* GetData() const { return this->Data; }

private:
  viskores::cont::ArrayHandleRuntimeVec<ComponentType> Array;
  viskores::cont::ArrayHandleBasic<ComponentType> ComponentsArray;
  const ComponentType* Data = nullptr;
};

template <typename OwnerType>
nb::capsule MakeArrayViewOwnerCapsule(OwnerType* owner)
{
  return nb::capsule(owner,
                     [](void* pointer) noexcept { delete static_cast<OwnerType*>(pointer); });
}

// Return whether a data pointer was registered by SharedBasicArrayFromNumPy.
// A `true` return means the buffer is Python-owned and pointer-stable, so a
// zero-copy NumPy view of it is safe; see the file header for details.
bool IsPythonOwnedData(const void* data)
{
  if (data == nullptr)
  {
    return false;
  }
  return PythonOwnedDataPointers().count(data) > 0;
}

// Allocate a fresh writable NumPy array of the given shape and component dtype,
// and memcpy `numberOfValues * numberOfComponents` components from `source`
// into it. Used by the copy fallback when the source buffer is not Python-owned.
template <typename ComponentType>
nb::object CopyComponentsToNumPy(const ComponentType* source,
                                 size_t numberOfValues,
                                 size_t numberOfComponents)
{
  nb::object result = AllocateNumPyArray<ComponentType>(numberOfValues, numberOfComponents);
  WritableBufferView destination(result);
  std::memcpy(destination.Data<void>(),
              source,
              numberOfValues * numberOfComponents * sizeof(ComponentType));
  return result;
}

// Return a NumPy array for a basic ArrayHandle, choosing between a zero-copy
// view (when the buffer was originally created from Python via array_from_numpy)
// and a defensive copy (otherwise). See the file header for the safety argument.
template <typename ValueType>
nb::object BasicArrayToNumPy(const viskores::cont::ArrayHandleBasic<ValueType>& array)
{
  using BasicArray = viskores::cont::ArrayHandleBasic<ValueType>;
  using OwnerType = BasicArrayViewOwner<BasicArray>;
  using ComponentType = typename OwnerType::ComponentType;

  const size_t numberOfValues = static_cast<size_t>(array.GetNumberOfValues());
  const size_t numberOfComponents = static_cast<size_t>(array.GetNumberOfComponentsFlat());

  // Capture the data pointer once. If the source buffer was registered by
  // array_from_numpy, it is Python-owned and InvalidRealloc-protected, so a
  // zero-copy NumPy view is safe. Otherwise the buffer could be reallocated
  // by another holder and we must copy to avoid use-after-free.
  const ValueType* dataPointer = array.GetReadPointer();
  if (IsPythonOwnedData(dataPointer))
  {
    auto owner = std::make_unique<OwnerType>(array);
    OwnerType* ownerPointer = owner.get();
    nb::capsule ownerCapsule = MakeArrayViewOwnerCapsule(ownerPointer);
    owner.release();

    if (numberOfComponents == 1)
    {
      nb::ndarray<nb::numpy, const ComponentType> result(
        ownerPointer->GetData(), { numberOfValues }, ownerCapsule);
      return result.cast();
    }
    nb::ndarray<nb::numpy, const ComponentType> result(
      ownerPointer->GetData(), { numberOfValues, numberOfComponents }, ownerCapsule);
    return result.cast();
  }

  // Copy fallback: memcpy into a fresh NumPy array. The result has no
  // dependency on the source ArrayHandle's lifetime.
  return CopyComponentsToNumPy(reinterpret_cast<const ComponentType*>(dataPointer),
                               numberOfValues,
                               numberOfComponents);
}

// Return a NumPy array for an ArrayHandleRuntimeVec, choosing between a 2D
// zero-copy view (Python-owned components buffer) and a copy fallback.
template <typename ComponentType>
nb::object RuntimeVecArrayToNumPy(const viskores::cont::ArrayHandleRuntimeVec<ComponentType>& array)
{
  using OwnerType = RuntimeVecArrayViewOwner<ComponentType>;
  auto componentsArray = array.GetComponentsArray();
  const size_t numberOfValues = static_cast<size_t>(array.GetNumberOfValues());
  const size_t numberOfComponents = static_cast<size_t>(array.GetNumberOfComponents());
  const ComponentType* dataPointer = componentsArray.GetReadPointer();

  if (IsPythonOwnedData(dataPointer))
  {
    auto owner = std::make_unique<OwnerType>(array);
    OwnerType* ownerPointer = owner.get();
    nb::capsule ownerCapsule = MakeArrayViewOwnerCapsule(ownerPointer);
    owner.release();
    nb::ndarray<nb::numpy, const ComponentType> result(
      ownerPointer->GetData(), { numberOfValues, numberOfComponents }, ownerCapsule);
    return result.cast();
  }

  return CopyComponentsToNumPy(dataPointer, numberOfValues, numberOfComponents);
}

// Try to convert an UnknownArrayHandle holding an ArrayHandleRuntimeVec to NumPy.
template <typename ComponentType>
bool TryRuntimeVecArrayToNumPy(const viskores::cont::UnknownArrayHandle& unknown,
                               nb::object& result)
{
  using RuntimeVecArray = viskores::cont::ArrayHandleRuntimeVec<ComponentType>;
  if (!unknown.IsType<RuntimeVecArray>())
  {
    return false;
  }

  RuntimeVecArray array;
  unknown.AsArrayHandle(array);
  result = RuntimeVecArrayToNumPy(array);
  return true;
}

// Try all supported ArrayHandleRuntimeVec component types for NumPy output.
bool TryAnyRuntimeVecArrayToNumPy(const viskores::cont::UnknownArrayHandle& unknown,
                                  nb::object& result)
{
#define VISKORES_PYTHON_TRY_RUNTIME_VEC_TO_NUMPY(ValueType, DTypeName) \
  if (TryRuntimeVecArrayToNumPy<ValueType>(unknown, result))           \
  {                                                                    \
    return true;                                                       \
  }
  VISKORES_PYTHON_NUMPY_SCALAR_TYPES(VISKORES_PYTHON_TRY_RUNTIME_VEC_TO_NUMPY)
#undef VISKORES_PYTHON_TRY_RUNTIME_VEC_TO_NUMPY
  return false;
}

// Convert an UnknownArrayHandle to NumPy.
nb::object UnknownArrayToNumPy(const viskores::cont::UnknownArrayHandle& unknown)
{
  // CastAndCallForTypes (and IsType / AsArrayHandle) require a valid
  // underlying array; calling them on a default-constructed UnknownArrayHandle
  // is undefined. Surface a Python error so users see a clean message.
  if (!unknown.IsValid())
  {
    throw std::runtime_error("UnknownArrayHandle is empty (no underlying array).");
  }

  nb::object result;
  // ArrayHandleRuntimeVec (the layout produced by array_from_numpy for 2D
  // inputs) is not part of VISKORES_DEFAULT_STORAGE_LIST, so CastAndCallForTypes
  // would not match it. Handle it with an explicit fast-path here.
  if (TryAnyRuntimeVecArrayToNumPy(unknown, result))
  {
    return result;
  }

  // For everything else, let UnknownArrayHandle resolve the concrete
  // ArrayHandle type via CastAndCall. The default storage list includes
  // StorageTagBasic, so ArrayHandleBasic<T> and ArrayHandleBasic<Vec<T,N>>
  // are both reached through ArrayHandleToNumPy's StorageTagBasic branch and
  // hit the zero-copy view path when the buffer is Python-owned.
  unknown.CastAndCallForTypes<viskores::TypeListAll, VISKORES_DEFAULT_STORAGE_LIST>(
    [&result](const auto& array) { result = ArrayHandleToNumPy(array); });
  return result;
}

// Build a concise Python repr for UnknownArrayHandle.
std::string UnknownArrayHandleRepr(const viskores::cont::UnknownArrayHandle& self)
{
  return "viskores.cont.UnknownArrayHandle(values=" +
    std::to_string(self.GetNumberOfValues()) +
    ", components=" + std::to_string(self.GetNumberOfComponentsFlat()) + ")";
}

} // namespace

// Add the viskores.cont bindings supplied by this source file.
void BindCont(nb::module_& m)
{
  // UnknownArrayHandle is exposed as the C++ type-erased array holder. NumPy
  // input is a module-level helper because the helper first inspects dtype and
  // shape, then creates the matching concrete ArrayHandle before returning it as
  // an UnknownArrayHandle.
  nb::class_<viskores::cont::UnknownArrayHandle>(m, "UnknownArrayHandle")
    .def(nb::init<>())
    // Match Python len(array) to the C++ number of values.
    .def("__len__",
         [](const viskores::cont::UnknownArrayHandle& self) { return self.GetNumberOfValues(); })
    .def("__repr__", &UnknownArrayHandleRepr)
    .def("GetNumberOfValues", &viskores::cont::UnknownArrayHandle::GetNumberOfValues)
    .def("GetNumberOfComponentsFlat",
         &viskores::cont::UnknownArrayHandle::GetNumberOfComponentsFlat)
    .def("asnumpy", &UnknownArrayToNumPy);

  m.def("array_from_numpy",
        &ArrayFromNumPy,
        nb::arg("array"),
        nb::arg("allow_copy") = false,
        "Wrap a NumPy array as a Viskores UnknownArrayHandle. By default the\n"
        "binding shares storage with the NumPy buffer; this requires the input\n"
        "to be C-contiguous, aligned, and writable. Pass allow_copy=True to\n"
        "let the binding make a contiguous, writable copy when the input\n"
        "layout is not directly shareable. allow_copy is permission, not a\n"
        "command: a directly-shareable input still takes the zero-copy path.");
  m.def("asnumpy", &UnknownArrayToNumPy, nb::arg("array"));
}

} // namespace viskores::python::bindings
