//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

// Convert a Viskores UnknownArrayHandle to a NumPy ndarray.
// Requires nanobind (nanobind/ndarray.h) and Python development headers.

#ifndef viskores_interop_python_ArrayHandleToNumPy_h
#define viskores_interop_python_ArrayHandleToNumPy_h

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>

#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/ArrayHandleRuntimeVec.h>
#include <viskores/cont/Error.h>
#include <viskores/cont/Token.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <memory>
#include <type_traits>

namespace nb = nanobind;

// X-macro listing every scalar dtype supported in both conversion directions.
// Each entry maps a Viskores scalar type to its NumPy dtype string.
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

namespace viskores
{
namespace interop
{
namespace python
{

// Map a Viskores scalar type to its canonical NumPy dtype string.
template <typename ValueType>
constexpr const char* NumPyDType()
{
  using ComponentType = std::remove_cv_t<ValueType>;
#define VISKORES_PYTHON_NUMPY_DTYPE_CASE(Type, Name) \
  if constexpr (std::is_same_v<ComponentType, Type>) \
  {                                                  \
    return Name;                                     \
  }
  VISKORES_PYTHON_NUMPY_SCALAR_TYPES(VISKORES_PYTHON_NUMPY_DTYPE_CASE)
#undef VISKORES_PYTHON_NUMPY_DTYPE_CASE
  static_assert(sizeof(ComponentType) == 0, "Unsupported NumPy component type.");
  return nullptr;
}

// RAII wrapper that pins the host buffer of an ArrayHandleRuntimeVec and keeps
// it alive for the lifetime of a NumPy view. All scalar and vector Viskores
// arrays are routed through this class: scalar ArrayHandleBasic<T> arrays are
// first extracted as a 1-component RuntimeVec, so this single owner class
// handles both shapes without needing a separate BasicArrayViewOwner.
template <typename ComponentType>
class RuntimeVecArrayViewOwner
{
public:
  // Pin the host components buffer (irreversible) before capturing the data
  // pointer. A single Token covers both operations so the buffer stays
  // attached across the pin-then-fetch pair. PinHost provides pointer
  // stability without retaining the Token; retaining the Token would block
  // subsequent Viskores writes until the NumPy view is garbage-collected.
  explicit RuntimeVecArrayViewOwner(
    const viskores::cont::ArrayHandleRuntimeVec<ComponentType>& array)
    : ComponentsArray(array.GetComponentsArray())
  {
    viskores::cont::Token token;
    this->ComponentsArray.GetBuffers()[0].PinHost(token);
    this->Data = this->ComponentsArray.GetReadPointer(token);
  }

  const ComponentType* GetData() const { return this->Data; }

private:
  // Holding ComponentsArray (the ArrayHandleBasic extracted from the RuntimeVec)
  // is sufficient to keep the underlying Buffer alive: ArrayHandleBasic and
  // ArrayHandleRuntimeVec share the same Buffer object via reference counting,
  // so either one prevents the Buffer from being destroyed.
  viskores::cont::ArrayHandleBasic<ComponentType> ComponentsArray;
  const ComponentType* Data = nullptr;
};

// Transfer ownership of an RAII owner object into an nb::capsule.
// Taking owner by value keeps it in a unique_ptr until the capsule is
// successfully constructed; if nb::capsule throws, unique_ptr cleans up.
template <typename OwnerType>
inline nb::capsule MakeArrayViewOwnerCapsule(std::unique_ptr<OwnerType> owner)
{
  nb::capsule capsule(owner.get(),
                      [](void* pointer) noexcept { delete static_cast<OwnerType*>(pointer); });
  // Capsule now owns the object; release without deleting.
  owner.release();
  return capsule;
}

// Return a zero-copy NumPy view of an ArrayHandleRuntimeVec.
// flattenScalar: when true and numberOfComponents == 1, emit a 1D (N,) array
// rather than (N, 1). Pass true only for scalar ArrayHandleBasic storage;
// explicit 1-component RuntimeVec arrays (from (N,1) NumPy input) must stay 2D.
template <typename ComponentType>
nb::object RuntimeVecArrayToNumPy(const viskores::cont::ArrayHandleRuntimeVec<ComponentType>& array,
                                  bool flattenScalar)
{
  using OwnerType = RuntimeVecArrayViewOwner<ComponentType>;
  const size_t numberOfValues = static_cast<size_t>(array.GetNumberOfValues());
  const size_t numberOfComponents = static_cast<size_t>(array.GetNumberOfComponents());

  auto owner = std::make_unique<OwnerType>(array);
  const ComponentType* data = owner->GetData();
  nb::capsule ownerCapsule = MakeArrayViewOwnerCapsule(std::move(owner));

  if (numberOfComponents == 1 && flattenScalar)
  {
    return nb::ndarray<nb::numpy, const ComponentType>(data, { numberOfValues }, ownerCapsule)
      .cast();
  }
  return nb::ndarray<nb::numpy, const ComponentType>(
           data, { numberOfValues, numberOfComponents }, ownerCapsule)
    .cast();
}

// Try to extract an UnknownArrayHandle as ArrayHandleRuntimeVec<ComponentType>
// and return a NumPy view. Returns false if the conversion is not possible.
template <typename ComponentType>
bool TryRuntimeVecArrayToNumPy(const viskores::cont::UnknownArrayHandle& unknown,
                               nb::object& result)
{
  using RuntimeVecArray = viskores::cont::ArrayHandleRuntimeVec<ComponentType>;
  if (!unknown.CanConvert<RuntimeVecArray>())
  {
    return false;
  }
  RuntimeVecArray array;
  unknown.AsArrayHandle(array);
  // Emit 1D only when the underlying storage is a plain scalar ArrayHandleBasic;
  // an explicit 1-component RuntimeVec (e.g. from a (N,1) NumPy input) stays 2D.
  const bool flattenScalar = unknown.IsType<viskores::cont::ArrayHandleBasic<ComponentType>>();
  result = RuntimeVecArrayToNumPy(array, flattenScalar);
  return true;
}

// Try every supported component type for the RuntimeVec NumPy output path.
inline bool TryAnyRuntimeVecArrayToNumPy(const viskores::cont::UnknownArrayHandle& unknown,
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

// Convert an UnknownArrayHandle to a NumPy ndarray. Zero-copy for
// ArrayHandleBasic and ArrayHandleRuntimeVec; other storage types are
// deep-copied to a basic array first.
inline nb::object ArrayHandleToNumPy(const viskores::cont::UnknownArrayHandle& array)
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

#endif // viskores_interop_python_ArrayHandleToNumPy_h
