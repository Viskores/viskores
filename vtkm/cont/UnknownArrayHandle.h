//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_UnknownArrayHandle_h
#define vtk_m_cont_UnknownArrayHandle_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleMultiplexer.h>

#include <vtkm/cont/DefaultTypes.h>

#include <memory>
#include <typeindex>

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename T, typename S>
static void UnknownAHDelete(void* mem)
{
  using AH = vtkm::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  delete arrayHandle;
}

template <typename T, typename S>
static void* UnknownADNewInstance()
{
  return new vtkm::cont::ArrayHandle<T, S>;
}

template <typename T, typename S>
static vtkm::Id UnknownAHNumberOfValues(void* mem)
{
  using AH = vtkm::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  return arrayHandle->GetNumberOfValues();
}

template <typename T, typename StaticSize = typename vtkm::VecTraits<T>::IsSizeStatic>
struct UnknownAHNumberOfComponentsImpl;
template <typename T>
struct UnknownAHNumberOfComponentsImpl<T, vtkm::VecTraitsTagSizeStatic>
{
  static constexpr vtkm::IdComponent Value = vtkm::VecTraits<T>::NUM_COMPONENTS;
};
template <typename T>
struct UnknownAHNumberOfComponentsImpl<T, vtkm::VecTraitsTagSizeVariable>
{
  static constexpr vtkm::IdComponent Value = 0;
};

template <typename T>
static vtkm::IdComponent UnknownAHNumberOfComponents()
{
  return UnknownAHNumberOfComponentsImpl<T>::Value;
}

template <typename T, typename S>
static void UnknownAHReleaseResources(void* mem)
{
  using AH = vtkm::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  arrayHandle->ReleaseResources();
}

template <typename T, typename S>
static void UnknownAHReleaseResourcesExecution(void* mem)
{
  using AH = vtkm::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  arrayHandle->ReleaseResourcesExecution();
}

template <typename T, typename S>
static void UnknownAHPrintSummary(void* mem, std::ostream& out, bool full)
{
  using AH = vtkm::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  vtkm::cont::printSummary_ArrayHandle(*arrayHandle, out, full);
}

struct VTKM_CONT_EXPORT UnknownAHContainer;

struct MakeUnknownAHContainerFunctor
{
  template <typename T, typename S>
  std::shared_ptr<UnknownAHContainer> operator()(const vtkm::cont::ArrayHandle<T, S>& array) const;
};

struct VTKM_CONT_EXPORT UnknownAHContainer
{
  void* ArrayHandlePointer;

  std::type_index ValueType;
  std::type_index StorageType;

  using DeleteType = void(void*);
  DeleteType* DeleteFunction;

  using NewInstanceType = void*();
  NewInstanceType& NewInstance;

  using NumberOfValuesType = vtkm::Id(void*);
  NumberOfValuesType* NumberOfValues;

  using NumberOfComponentsType = vtkm::IdComponent();
  NumberOfComponentsType* NumberOfComponents;

  using ReleaseResourcesType = void(void*);
  ReleaseResourcesType* ReleaseResources;
  ReleaseResourcesType* ReleaseResourcesExecution;

  using PrintSummaryType = void(void*, std::ostream&, bool);
  PrintSummaryType* PrintSummary;

  void operator=(const UnknownAHContainer&) = delete;

  ~UnknownAHContainer() { this->DeleteFunction(this->ArrayHandlePointer); }

  std::shared_ptr<UnknownAHContainer> MakeNewInstance() const;

  template <typename T, typename S>
  static std::shared_ptr<UnknownAHContainer> Make(const vtkm::cont::ArrayHandle<T, S>& array)
  {
    return std::shared_ptr<UnknownAHContainer>(new UnknownAHContainer(array));
  }

  template <typename TargetT, typename SourceT, typename SourceS>
  static std::shared_ptr<UnknownAHContainer> Make(
    const vtkm::cont::ArrayHandle<TargetT, vtkm::cont::StorageTagCast<SourceT, SourceS>>& array)
  {
    return Make(array.GetStorage().GetArray());
  }

  template <typename T, typename... Ss>
  static std::shared_ptr<UnknownAHContainer> Make(
    const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagMultiplexer<Ss...>>& array)
  {
    auto&& variant = array.GetStorage().GetArrayHandleVariant();
    if (variant.IsValid())
    {
      return variant.CastAndCall(MakeUnknownAHContainerFunctor{});
    }
    else
    {
      return std::shared_ptr<UnknownAHContainer>{};
    }
  }

private:
  UnknownAHContainer(const UnknownAHContainer&) = default;

  template <typename T, typename S>
  explicit UnknownAHContainer(const vtkm::cont::ArrayHandle<T, S>& array)
    : ArrayHandlePointer(new vtkm::cont::ArrayHandle<T, S>(array))
    , ValueType(typeid(T))
    , StorageType(typeid(S))
    , DeleteFunction(detail::UnknownAHDelete<T, S>)
    , NewInstance(detail::UnknownADNewInstance<T, S>)
    , NumberOfValues(detail::UnknownAHNumberOfValues<T, S>)
    , NumberOfComponents(detail::UnknownAHNumberOfComponents<T>)
    , ReleaseResources(detail::UnknownAHReleaseResources<T, S>)
    , ReleaseResourcesExecution(detail::UnknownAHReleaseResourcesExecution<T, S>)
    , PrintSummary(detail::UnknownAHPrintSummary<T, S>)
  {
  }
};

template <typename T, typename S>
inline std::shared_ptr<UnknownAHContainer> MakeUnknownAHContainerFunctor::operator()(
  const vtkm::cont::ArrayHandle<T, S>& array) const
{
  return UnknownAHContainer::Make(array);
};

} // namespace detail

// Forward declaration. Include UncertainArrayHandle.h if using this.
template <typename ValueTypeList, typename StorageTypeList>
class UncertainArrayHandle;

/// \brief An ArrayHandle of an unknown value type and storage.
///
/// `UnknownArrayHandle` holds an `ArrayHandle` object using runtime polymorphism
/// to manage different value and storage types rather than compile-time templates.
/// This adds a programming convenience that helps avoid a proliferation of
/// templates. It also provides the management necessary to interface VTK-m with
/// data sources where types will not be known until runtime and is the storage
/// mechanism for classes like `DataSet` and `Field` that can hold numerous
/// types.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in VTK-m, `UnknownArrayHandle` contains a method named `CastAndCallForTypes`
/// that determines the correct type from some known list of value types and
/// storage. This mechanism is used internally by VTK-m's worklet invocation
/// mechanism to determine the type when running algorithms.
///
/// If the `UnknownArrayHandle` is used in a context where the possible array
/// types can be whittled down to a finite list (or you have to), you can
/// specify lists of value types and storage using the `ResetTypesAndStorage`
/// method. This will convert this object to an `UncertainArrayHandle` of the
/// given types. In cases where a finite set of types need to specified but
/// there is no known subset, `VTKM_DEFAULT_TYPE_LIST` and
/// `VTKM_DEFAULT_STORAGE_LIST` can be used.
///
/// `ArrayHandleCast` and `ArrayHandleMultiplexer` are treated special. If
/// the `UnknownArrayHandle` is set to an `ArrayHandle` of one of these
/// types, it will actually store the `ArrayHandle` contained. Likewise,
/// if the `ArrayHandle` is retrieved as one of these types, it will
/// automatically convert it if possible.
///
class VTKM_CONT_EXPORT UnknownArrayHandle
{
  std::shared_ptr<detail::UnknownAHContainer> Container;

  VTKM_CONT bool IsValueTypeImpl(std::type_index type) const;
  VTKM_CONT bool IsStorageTypeImpl(std::type_index type) const;

public:
  VTKM_CONT UnknownArrayHandle() = default;
  UnknownArrayHandle(const UnknownArrayHandle&) = default;

  template <typename T, typename S>
  VTKM_CONT UnknownArrayHandle(const vtkm::cont::ArrayHandle<T, S>& array)
    : Container(detail::UnknownAHContainer::Make(array))
  {
  }

  UnknownArrayHandle& operator=(const vtkm::cont::UnknownArrayHandle&) = default;

  /// \brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new `UnknownArrayHandle` for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  VTKM_CONT UnknownArrayHandle NewInstance() const
  {
    UnknownArrayHandle newArray;
    if (this->Container)
    {
      newArray.Container = this->Container->MakeNewInstance();
    }
    return newArray;
  }

  /// Returns true if this array matches the ValueType template argument.
  ///
  template <typename ValueType>
  VTKM_CONT bool IsValueType() const
  {
    return this->IsValueTypeImpl(typeid(ValueType));
  }

  /// Returns true if this array matches the StorageType template argument.
  ///
  template <typename StorageType>
  VTKM_CONT bool IsStorageType() const
  {
    return this->IsStorageTypeImpl(typeid(StorageType));
  }

  /// Returns true if this array matches the ArrayHandleType template argument.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT bool IsType() const
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    return (this->IsValueType<typename ArrayHandleType::ValueType>() &&
            this->IsStorageType<typename ArrayHandleType::StorageTag>());
  }

  /// \brief Assigns potential value and storage types.
  ///
  /// Calling this method will return an `UncertainArrayHandle` with the provided
  /// value and storage type lists. The returned object will hold the same
  /// `ArrayHandle`, but `CastAndCall`s on the returned object will be constrained
  /// to the given types.
  ///
  // Defined in UncertainArrayHandle.h
  template <typename NewValueTypeList, typename NewStorageTypeList>
  VTKM_CONT vtkm::cont::UncertainArrayHandle<NewValueTypeList, NewStorageTypeList> ResetTypes(
    NewValueTypeList = NewValueTypeList{},
    NewStorageTypeList = NewStorageTypeList{}) const;

  /// \brief Returns the number of values in the array.
  ///
  VTKM_CONT vtkm::Id GetNumberOfValues() const
  {
    if (this->Container)
    {
      return this->Container->NumberOfValues(this->Container->ArrayHandlePointer);
    }
    else
    {
      return 0;
    }
  }

  /// \brief Returns the number of components for each value in the array.
  ///
  /// If the array holds `vtkm::Vec` objects, this will return the number of components
  /// in each value. If the array holds a basic C type (such as `float`), this will return 1.
  /// If the array holds `Vec`-like objects that have the number of components that can vary
  /// at runtime, this method will return 0 (because there is no consistent answer).
  ///
  VTKM_CONT vtkm::IdComponent GetNumberOfComponents() const
  {
    if (this->Container)
    {
      return this->Container->NumberOfComponents();
    }
    else
    {
      return 0;
    }
  }

  /// \brief Determine if the contained array can be passed to the given array type.
  ///
  /// This method will return true if calling `AsArrayHandle` of the given type will
  /// succeed. The result is similar to `IsType`, and if `IsType` returns true, then
  /// this will return true. However, this method will also return true for other
  /// types such as an `ArrayHandleMultiplexer` that can contain the array.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT bool CanConvert() const;

  // MSVC will issue deprecation warnings here if this template is instantiated with
  // a deprecated class even if the template is used from a section of code where
  // deprecation warnings are suppressed. This is annoying behavior since this template
  // has no control over what class it is used with. To get around it, we have to
  // suppress all deprecation warnings here.
#ifdef VTKM_MSVC
  VTKM_DEPRECATED_SUPPRESS_BEGIN
#endif
  ///@{
  /// Returns this array cast appropriately and stored in the given `ArrayHandle` type.
  /// Throws an `ErrorBadType` if the stored array cannot be stored in the given array type.
  /// Use the `IsType` method to determine if the array can be returned with the given type.
  ///
  template <typename T, typename S>
  VTKM_CONT void AsArrayHandle(vtkm::cont::ArrayHandle<T, S>& array) const
  {
    using ArrayType = vtkm::cont::ArrayHandle<T, S>;
    if (!this->IsType<ArrayType>())
    {
      VTKM_LOG_CAST_FAIL(*this, decltype(array));
      throwFailedDynamicCast(vtkm::cont::TypeToString(*this), vtkm::cont::TypeToString(array));
    }

    array = *reinterpret_cast<ArrayType*>(this->Container->ArrayHandlePointer);
  }

  template <typename T, typename... Ss>
  VTKM_CONT void AsArrayHandle(
    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagMultiplexer<Ss...>>& array) const;

  template <typename TargetT, typename SourceT, typename SourceS>
  VTKM_CONT void AsArrayHandle(
    vtkm::cont::ArrayHandle<TargetT, vtkm::cont::StorageTagCast<SourceT, SourceS>>& array) const
  {
    using ContainedArrayType = vtkm::cont::ArrayHandle<SourceT, SourceS>;
    array = vtkm::cont::ArrayHandleCast<TargetT, ContainedArrayType>(
      this->AsArrayHandle<ContainedArrayType>());
  }

  template <typename ArrayType>
  VTKM_CONT ArrayType AsArrayHandle() const
  {
    VTKM_IS_ARRAY_HANDLE(ArrayType);
    ArrayType array;
    this->AsArrayHandle(array);
    return array;
  }
  ///@}
#ifdef VTKM_MSVC
  VTKM_DEPRECATED_SUPPRESS_END
#endif

  /// \brief Call a functor using the underlying array type.
  ///
  /// `CastAndCall` attempts to cast the held array to a specific value type,
  /// and then calls the given functor with the cast array. You must specify
  /// the `TypeList` and `StorageList` as template arguments.
  ///
  template <typename TypeList, typename StorageList, typename Functor, typename... Args>
  VTKM_CONT void CastAndCallForTypes(Functor&& functor, Args&&... args) const;

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  VTKM_CONT void ReleaseResourcesExecution() const
  {
    if (this->Container)
    {
      this->Container->ReleaseResourcesExecution(this->Container->ArrayHandlePointer);
    }
  }

  /// Releases all resources in both the control and execution environments.
  ///
  VTKM_CONT void ReleaseResources() const
  {
    if (this->Container)
    {
      this->Container->ReleaseResources(this->Container->ArrayHandlePointer);
    }
  }

  VTKM_CONT void PrintSummary(std::ostream& out, bool full = false) const
  {
    if (this->Container)
    {
      this->Container->PrintSummary(this->Container->ArrayHandlePointer, out, full);
    }
    else
    {
      out << "null UnknownArrayHandle" << std::endl;
    }
  }
};

//=============================================================================
// Out of class implementations

namespace detail
{

template <typename T, typename S>
struct UnknownArrayHandleCanConvert
{
  VTKM_CONT bool operator()(const vtkm::cont::UnknownArrayHandle& array) const
  {
    return array.IsType<vtkm::cont::ArrayHandle<T, S>>();
  }
};

template <typename TargetT, typename SourceT, typename SourceS>
struct UnknownArrayHandleCanConvert<TargetT, vtkm::cont::StorageTagCast<SourceT, SourceS>>
{
  VTKM_CONT bool operator()(const vtkm::cont::UnknownArrayHandle& array) const
  {
    return UnknownArrayHandleCanConvert<SourceT, SourceS>{}(array);
  }
};

template <typename T>
struct UnknownArrayHandleCanConvertTry
{
  template <typename S>
  VTKM_CONT void operator()(S, const vtkm::cont::UnknownArrayHandle& array, bool& canConvert) const
  {
    canConvert |= UnknownArrayHandleCanConvert<T, S>{}(array);
  }
};

template <typename T, typename... Ss>
struct UnknownArrayHandleCanConvert<T, vtkm::cont::StorageTagMultiplexer<Ss...>>
{
  VTKM_CONT bool operator()(const vtkm::cont::UnknownArrayHandle& array) const
  {
    bool canConvert = false;
    vtkm::ListForEach(UnknownArrayHandleCanConvertTry<T>{}, vtkm::List<Ss...>{}, array, canConvert);
    return canConvert;
  }
};

} // namespace detail

template <typename ArrayHandleType>
VTKM_CONT bool UnknownArrayHandle::CanConvert() const
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

  return detail::UnknownArrayHandleCanConvert<typename ArrayHandleType::ValueType,
                                              typename ArrayHandleType::StorageTag>{}(*this);
}

namespace detail
{

struct UnknownArrayHandleMultplexerCastTry
{
  template <typename T, typename S, typename... Ss>
  VTKM_CONT void operator()(
    S,
    const vtkm::cont::UnknownArrayHandle& unknownArray,
    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagMultiplexer<Ss...>>& outputArray,
    bool& converted) const
  {
    using ArrayType = vtkm::cont::ArrayHandle<T, S>;
    if (unknownArray.CanConvert<ArrayType>())
    {
      if (converted && !unknownArray.IsType<ArrayType>())
      {
        // The array has already been converted and pushed in the multiplexer. It is
        // possible that multiple array types can be put in the ArrayHandleMultiplexer
        // (for example, and ArrayHandle or an ArrayHandle that has been cast). Exact
        // matches will override other matches (hence, the second part of the condition),
        // but at this point we have already found a better array to put inside.
        return;
      }
      outputArray.GetStorage().SetArray(unknownArray.AsArrayHandle<ArrayType>());
      converted = true;
    }
  }
};

} // namespace detail

template <typename T, typename... Ss>
void UnknownArrayHandle::AsArrayHandle(
  vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagMultiplexer<Ss...>>& array) const
{
  bool converted = false;
  vtkm::ListForEach(
    detail::UnknownArrayHandleMultplexerCastTry{}, vtkm::List<Ss...>{}, *this, array, converted);

  if (!converted)
  {
    VTKM_LOG_CAST_FAIL(*this, decltype(array));
    throwFailedDynamicCast(vtkm::cont::TypeToString(*this), vtkm::cont::TypeToString(array));
  }
}

namespace detail
{

struct UnknownArrayHandleTry
{
  template <typename T, typename S, typename Functor, typename... Args>
  void operator()(vtkm::List<T, S>,
                  Functor&& f,
                  bool& called,
                  const vtkm::cont::UnknownArrayHandle& unknownArray,
                  Args&&... args) const
  {
    using DerivedArrayType = vtkm::cont::ArrayHandle<T, S>;
    if (!called && unknownArray.IsType<DerivedArrayType>())
    {
      called = true;
      DerivedArrayType derivedArray;
      unknownArray.AsArrayHandle(derivedArray);
      VTKM_LOG_CAST_SUCC(unknownArray, derivedArray);

      // If you get a compile error here, it means that you have called CastAndCall for a
      // vtkm::cont::UnknownArrayHandle and the arguments of the functor do not match those
      // being passed. This is often because it is calling the functor with an ArrayHandle
      // type that was not expected. Either add overloads to the functor to accept all
      // possible array types or constrain the types tried for the CastAndCall. Note that
      // the functor will be called with an array of type vtkm::cont::ArrayHandle<T, S>.
      // Directly using a subclass of ArrayHandle (e.g. vtkm::cont::ArrayHandleConstant<T>)
      // might not work.
      f(derivedArray, std::forward<Args>(args)...);
    }
  }
};

template <typename T>
struct IsUndefinedArrayType
{
};
template <typename T, typename S>
struct IsUndefinedArrayType<vtkm::List<T, S>> : vtkm::cont::internal::IsInvalidArrayHandle<T, S>
{
};

template <typename ValueTypeList, typename StorageTypeList>
using ListAllArrayTypes =
  vtkm::ListRemoveIf<vtkm::ListCross<ValueTypeList, StorageTypeList>, IsUndefinedArrayType>;


VTKM_CONT_EXPORT void ThrowCastAndCallException(const vtkm::cont::UnknownArrayHandle&,
                                                const std::type_info&);

} // namespace detail



template <typename TypeList, typename StorageTagList, typename Functor, typename... Args>
VTKM_CONT void UnknownArrayHandle::CastAndCallForTypes(Functor&& f, Args&&... args) const
{
  using crossProduct = detail::ListAllArrayTypes<TypeList, StorageTagList>;

  bool called = false;
  vtkm::ListForEach(detail::UnknownArrayHandleTry{},
                    crossProduct{},
                    std::forward<Functor>(f),
                    called,
                    *this,
                    std::forward<Args>(args)...);
  if (!called)
  {
    // throw an exception
    VTKM_LOG_CAST_FAIL(*this, TypeList);
    detail::ThrowCastAndCallException(*this, typeid(TypeList));
  }
}

template <typename Functor, typename... Args>
void CastAndCall(const UnknownArrayHandle& handle, Functor&& f, Args&&... args)
{
  handle.CastAndCallForTypes<VTKM_DEFAULT_TYPE_LIST, VTKM_DEFAULT_STORAGE_LIST>(
    std::forward<Functor>(f), std::forward<Args>(args)...);
}

namespace internal
{

template <>
struct DynamicTransformTraits<vtkm::cont::UnknownArrayHandle>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal

}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION

namespace vtkm
{
namespace cont
{

template <>
struct VTKM_CONT_EXPORT SerializableTypeString<vtkm::cont::UnknownArrayHandle>
{
  static VTKM_CONT std::string Get();
};
}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

template <>
struct VTKM_CONT_EXPORT Serialization<vtkm::cont::UnknownArrayHandle>
{
public:
  static VTKM_CONT void save(BinaryBuffer& bb, const vtkm::cont::UnknownArrayHandle& obj);
  static VTKM_CONT void load(BinaryBuffer& bb, vtkm::cont::UnknownArrayHandle& obj);
};

} // namespace mangled_diy_namespace

/// @endcond SERIALIZATION

#endif //vtk_m_cont_UnknownArrayHandle_h
