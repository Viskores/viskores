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

#include <vtkm/cont/ArrayExtractComponent.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleMultiplexer.h>
#include <vtkm/cont/ArrayHandleRecombineVec.h>
#include <vtkm/cont/ArrayHandleStride.h>
#include <vtkm/cont/StorageList.h>

#include <vtkm/TypeList.h>

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
static void* UnknownAHNewInstance()
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

template <typename T, typename StaticSize = typename vtkm::VecTraits<T>::IsSizeStatic>
struct UnknownAHNumberOfComponentsFlatImpl;
template <typename T>
struct UnknownAHNumberOfComponentsFlatImpl<T, vtkm::VecTraitsTagSizeStatic>
{
  static constexpr vtkm::IdComponent Value = vtkm::VecFlat<T>::NUM_COMPONENTS;
};
template <typename T>
struct UnknownAHNumberOfComponentsFlatImpl<T, vtkm::VecTraitsTagSizeVariable>
{
  static constexpr vtkm::IdComponent Value = 0;
};

template <typename T>
static vtkm::IdComponent UnknownAHNumberOfComponentsFlat()
{
  return UnknownAHNumberOfComponentsFlatImpl<T>::Value;
}

template <typename T, typename S>
static void UnknownAHAllocate(void* mem,
                              vtkm::Id numValues,
                              vtkm::CopyFlag preserve,
                              vtkm::cont::Token& token)
{
  using AH = vtkm::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  arrayHandle->Allocate(numValues, preserve, token);
}

template <typename T, typename S>
static std::vector<vtkm::cont::internal::Buffer>
UnknownAHExtractComponent(void* mem, vtkm::IdComponent componentIndex, vtkm::CopyFlag allowCopy)
{
  using AH = vtkm::cont::ArrayHandle<T, S>;
  AH* arrayHandle = reinterpret_cast<AH*>(mem);
  auto componentArray = vtkm::cont::ArrayExtractComponent(*arrayHandle, componentIndex, allowCopy);
  vtkm::cont::internal::Buffer* buffers = componentArray.GetBuffers();
  return std::vector<vtkm::cont::internal::Buffer>(buffers, buffers + 2);
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

struct VTKM_CONT_EXPORT UnknownAHComponentInfo
{
  std::type_index Type;
  bool IsIntegral;
  bool IsFloat;
  bool IsSigned;
  std::size_t Size;

  UnknownAHComponentInfo() = delete;

  bool operator==(const UnknownAHComponentInfo& rhs);

  template <typename T>
  static UnknownAHComponentInfo Make()
  {
    return UnknownAHComponentInfo{ typeid(T),
                                   std::is_integral<T>::value,
                                   std::is_floating_point<T>::value,
                                   std::is_signed<T>::value,
                                   sizeof(T) };
  }

private:
  UnknownAHComponentInfo(std::type_index&& type,
                         bool isIntegral,
                         bool isFloat,
                         bool isSigned,
                         std::size_t size)
    : Type(std::move(type))
    , IsIntegral(isIntegral)
    , IsFloat(isFloat)
    , IsSigned(isSigned)
    , Size(size)
  {
  }
};

struct VTKM_CONT_EXPORT UnknownAHContainer
{
  void* ArrayHandlePointer;

  std::type_index ValueType;
  std::type_index StorageType;
  UnknownAHComponentInfo BaseComponentType;

  using DeleteType = void(void*);
  DeleteType* DeleteFunction;

  using NewInstanceType = void*();
  NewInstanceType* NewInstance;

  using NewInstanceBasicType = std::shared_ptr<UnknownAHContainer>();
  NewInstanceBasicType* NewInstanceBasic;
  NewInstanceBasicType* NewInstanceFloatBasic;

  using NumberOfValuesType = vtkm::Id(void*);
  NumberOfValuesType* NumberOfValues;

  using NumberOfComponentsType = vtkm::IdComponent();
  NumberOfComponentsType* NumberOfComponents;
  NumberOfComponentsType* NumberOfComponentsFlat;

  using AllocateType = void(void*, vtkm::Id, vtkm::CopyFlag, vtkm::cont::Token&);
  AllocateType* Allocate;

  using ExtractComponentType = std::vector<vtkm::cont::internal::Buffer>(void*,
                                                                         vtkm::IdComponent,
                                                                         vtkm::CopyFlag);
  ExtractComponentType* ExtractComponent;

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
    vtkm::cont::ArrayHandleCast<TargetT, vtkm::cont::ArrayHandle<SourceT, SourceS>> castArray =
      array;
    return Make(castArray.GetSourceArray());
  }

  template <typename T, typename... Ss>
  static std::shared_ptr<UnknownAHContainer> Make(
    const vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagMultiplexer<Ss...>>& array)
  {
    auto&& variant = vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandle<T, Ss>...>(array)
                       .GetArrayHandleVariant();
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
  explicit UnknownAHContainer(const vtkm::cont::ArrayHandle<T, S>& array);
};

template <typename T>
static std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceBasic(vtkm::VecTraitsTagSizeStatic)
{
  return UnknownAHContainer::Make(vtkm::cont::ArrayHandleBasic<T>{});
}
template <typename T>
static std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceBasic(vtkm::VecTraitsTagSizeVariable)
{
  throw vtkm::cont::ErrorBadType("Cannot create a basic array container from with ValueType of " +
                                 vtkm::cont::TypeToString<T>());
}
template <typename T>
static std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceBasic()
{
  return UnknownAHNewInstanceBasic<T>(typename vtkm::VecTraits<T>::IsSizeStatic{});
}

template <typename T>
static std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceFloatBasic(
  vtkm::VecTraitsTagSizeStatic)
{
  using FloatT = typename vtkm::VecTraits<T>::template ReplaceBaseComponentType<vtkm::FloatDefault>;
  return UnknownAHContainer::Make(vtkm::cont::ArrayHandleBasic<FloatT>{});
}
template <typename T>
static std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceFloatBasic(
  vtkm::VecTraitsTagSizeVariable)
{
  throw vtkm::cont::ErrorBadType("Cannot create a basic array container from with ValueType of " +
                                 vtkm::cont::TypeToString<T>());
}
template <typename T>
static std::shared_ptr<UnknownAHContainer> UnknownAHNewInstanceFloatBasic()
{
  return UnknownAHNewInstanceFloatBasic<T>(typename vtkm::VecTraits<T>::IsSizeStatic{});
}

template <typename T, typename S>
#ifdef VTKM_GCC
// On rare occasion, we have seen errors like this:
//   `_ZN4vtkm4cont6detailL27UnknownAHNumberOfComponentsIxEEiv' referenced in section
//   `.data.rel.ro.local' of CMakeFiles/UnitTests_vtkm_cont_testing.dir/UnitTestCellSet.cxx.o:
//   defined in discarded section
//   `.text._ZN4vtkm4cont6detailL27UnknownAHNumberOfComponentsIxEEiv[_ZN4vtkm4cont14ArrayGetValuesINS0_15StorageTagBasicExNS0_18StorageTagCountingES2_EEvRKNS0_11ArrayHandleIxT_EERKNS4_IT0_T1_EERNS4_IS9_T2_EE]'
//   of CMakeFiles/UnitTests_vtkm_cont_testing.dir/UnitTestCellSet.cxx.o
// I don't know what circumstances exactly lead up to this, but it appears that the compiler
// is being overly aggressive with removing unused symbols. In this instance, it seems to have
// removed a function actually being used. This might be a bug in the compiler (I happen to have
// seen it in gcc 8.3), or it could be caused by a link-time optimizer. The problem should be
// able to be solved by explictly saying that this templated method is being used. (I can't think
// of any circumstances where this template would be instantiated but not used.) If the compiler
// knows this is being used, it should know all the templated methods internal are also used.
__attribute__((used))
#endif
inline UnknownAHContainer::UnknownAHContainer(const vtkm::cont::ArrayHandle<T, S>& array)
  : ArrayHandlePointer(new vtkm::cont::ArrayHandle<T, S>(array))
  , ValueType(typeid(T))
  , StorageType(typeid(S))
  , BaseComponentType(
      UnknownAHComponentInfo::Make<typename vtkm::VecTraits<T>::BaseComponentType>())
  , DeleteFunction(detail::UnknownAHDelete<T, S>)
  , NewInstance(detail::UnknownAHNewInstance<T, S>)
  , NewInstanceBasic(detail::UnknownAHNewInstanceBasic<T>)
  , NewInstanceFloatBasic(detail::UnknownAHNewInstanceFloatBasic<T>)
  , NumberOfValues(detail::UnknownAHNumberOfValues<T, S>)
  , NumberOfComponents(detail::UnknownAHNumberOfComponents<T>)
  , NumberOfComponentsFlat(detail::UnknownAHNumberOfComponentsFlat<T>)
  , Allocate(detail::UnknownAHAllocate<T, S>)
  , ExtractComponent(detail::UnknownAHExtractComponent<T, S>)
  , ReleaseResources(detail::UnknownAHReleaseResources<T, S>)
  , ReleaseResourcesExecution(detail::UnknownAHReleaseResourcesExecution<T, S>)
  , PrintSummary(detail::UnknownAHPrintSummary<T, S>)
{
}

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
  VTKM_CONT bool IsBaseComponentTypeImpl(const detail::UnknownAHComponentInfo& type) const;

public:
  VTKM_CONT UnknownArrayHandle() = default;
  UnknownArrayHandle(const UnknownArrayHandle&) = default;

  template <typename T, typename S>
  VTKM_CONT UnknownArrayHandle(const vtkm::cont::ArrayHandle<T, S>& array)
    : Container(detail::UnknownAHContainer::Make(array))
  {
  }

  UnknownArrayHandle& operator=(const vtkm::cont::UnknownArrayHandle&) = default;

  /// \brief Returns whether an array is stored in this `UnknownArrayHandle`.
  ///
  /// If the `UnknownArrayHandle` is constructed without an `ArrayHandle`, it
  /// will not have an underlying type, and therefore the operations will be
  /// invalid. It is still possible to set this `UnknownArrayHandle` to an
  /// `ArrayHandle`.
  VTKM_CONT bool IsValid() const;

  /// \brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new `UnknownArrayHandle` for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  VTKM_CONT UnknownArrayHandle NewInstance() const;

  /// \brief Create a new `ArrayHandleBasic` with the same `ValueType` as this array.
  ///
  /// This method creates a new `ArrayHandleBasic` that has the same `ValueType` as the
  /// array held by this one and returns a new `UnknownArrayHandle` for it. This method
  /// is convenient when creating output arrays that should have the same types of values
  /// of the input, but the input might not be a writable array.
  ///
  VTKM_CONT UnknownArrayHandle NewInstanceBasic() const;

  /// \brief Create a new `ArrayHandleBasic` with the base component of `FloatDefault`
  ///
  /// This method creates a new `ArrayHandleBasic` that has a `ValueType` that is similar
  /// to the array held by this one except that the base component type is replaced with
  /// `vtkm::FloatDefault`. For example, if the contained array has `vtkm::Int32` value types,
  /// the returned array will have `vtkm::FloatDefault` value types. If the contained array
  /// has `vtkm::Id3` value types, the returned array will have `vtkm::Vec3f` value types.
  /// If the contained array already has `vtkm::FloatDefault` as the base component (e.g.
  /// `vtkm::FloatDefault`, `vtkm::Vec3f`, `vtkm::Vec<vtkm::Vec2f, 3>`), then the value type
  /// will be preserved.
  ///
  /// The created array is returned in a new `UnknownArrayHandle`.
  ///
  /// This method is used to convert an array of an unknown type to an array of an almost
  /// known type.
  ///
  VTKM_CONT UnknownArrayHandle NewInstanceFloatBasic() const;

  /// \brief Returns the name of the value type stored in the array.
  ///
  /// Returns an empty string if no array is stored.
  VTKM_CONT std::string GetValueTypeName() const;

  /// \brief Returns the name of the base component of the value type stored in the array.
  ///
  /// Returns an empty string if no array is stored.
  VTKM_CONT std::string GetBaseComponentTypeName() const;

  /// \brief Returns the name of the storage tag for the array.
  ///
  /// Returns an empty string if no array is stored.
  VTKM_CONT std::string GetStorageTypeName() const;

  /// \brief Returns a string representation of the underlying data type.
  ///
  /// The returned string will be of the form `vtkm::cont::ArrayHandle<T, S>` rather than the name
  /// of an actual subclass. If no array is stored, an empty string is returned.
  ///
  VTKM_CONT std::string GetArrayTypeName() const;

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

  /// \brief Returns true if this array's `ValueType` has the provided base component type.
  ///
  /// The base component type is the recursive component type of any `Vec`-like object. So
  /// if the array's `ValueType` is `vtkm::Vec<vtkm::Float32, 3>`, then the base component
  /// type will be `vtkm::Float32`. Likewise, if the `ValueType` is
  /// `vtkm::Vec<vtkm::Vec<vtkm::Float32, 3>, 2>`, then the base component type is still
  /// `vtkm::Float32`.
  ///
  /// If the `ValueType` is not `Vec`-like type, then the base component type is the same.
  /// So a `ValueType` of `vtkm::Float32` has a base component type of `vtkm::Float32`.
  ///
  template <typename BaseComponentType>
  VTKM_CONT bool IsBaseComponentType() const
  {
    return this->IsBaseComponentTypeImpl(detail::UnknownAHComponentInfo::Make<BaseComponentType>());
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

  template <typename NewValueTypeList>
  VTKM_DEPRECATED(1.6, "Specify both value types and storage types.")
  VTKM_CONT
    vtkm::cont::UncertainArrayHandle<NewValueTypeList, vtkm::cont::StorageListCommon> ResetTypes(
      NewValueTypeList = NewValueTypeList{}) const
  {
    return this->ResetTypes<NewValueTypeList, vtkm::cont::StorageListCommon>();
  }

  /// \brief Returns the number of values in the array.
  ///
  VTKM_CONT vtkm::Id GetNumberOfValues() const;

  /// \brief Returns the number of components for each value in the array.
  ///
  /// If the array holds `vtkm::Vec` objects, this will return the number of components
  /// in each value. If the array holds a basic C type (such as `float`), this will return 1.
  /// If the array holds `Vec`-like objects that have the number of components that can vary
  /// at runtime, this method will return 0 (because there is no consistent answer).
  ///
  VTKM_CONT vtkm::IdComponent GetNumberOfComponents() const;

  /// \brief Returns the total number of components for each value in the array.
  ///
  /// If the array holds `vtkm::Vec` objects, this will return the total number of components
  /// in each value assuming the object is flattened out to one level of `Vec` objects.
  /// If the array holds a basic C type (such as `float`), this will return 1.
  /// If the array holds a simple `Vec` (such as `vtkm::Vec3f`), this will return the number
  /// of components (in this case 3).
  /// If the array holds a hierarchy of `Vec`s (such as `vtkm::Vec<vtkm::Vec3f, 2>`), this will
  /// return the total number of vecs (in this case 6).
  /// If the array holds `Vec`-like objects that have the number of components that can vary
  /// at runtime, this method will return 0 (because there is no consistent answer).
  ///
  VTKM_CONT vtkm::IdComponent GetNumberOfComponentsFlat() const;

  /// \brief Reallocate the data in the array.
  ///
  /// The allocation works the same as the `Allocate` method of `vtkm::cont::ArrayHandle`.
  ///
  /// @{
  VTKM_CONT void Allocate(vtkm::Id numValues,
                          vtkm::CopyFlag preserve,
                          vtkm::cont::Token& token) const;
  VTKM_CONT void Allocate(vtkm::Id numValues, vtkm::CopyFlag preserve = vtkm::CopyFlag::Off) const;
  /// @}

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
      throwFailedDynamicCast(this->GetArrayTypeName(), vtkm::cont::TypeToString(array));
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

  // For code still expecting a VariantArrayHandle
  template <typename ArrayHandleType>
  VTKM_DEPRECATED(1.6, "Use AsArrayHandle.")
  VTKM_CONT ArrayHandleType Cast() const
  {
    return this->AsArrayHandle<ArrayHandleType>();
  }
  template <typename ArrayHandleType>
  VTKM_DEPRECATED(1.6, "Use AsArrayHandle.")
  VTKM_CONT void CopyTo(ArrayHandleType& array) const
  {
    this->AsArrayHandle(array);
  }
  template <typename MultiplexerType>
  VTKM_DEPRECATED(1.6, "Use AsArrayHandle.")
  VTKM_CONT MultiplexerType AsMultiplexer() const
  {
    return this->AsArrayHandle<MultiplexerType>();
  }
  template <typename MultiplexerType>
  VTKM_DEPRECATED(1.6, "Use AsArrayHandle.")
  VTKM_CONT void AsMultiplexer(MultiplexerType& result) const
  {
    result = this->AsArrayHandle<MultiplexerType>();
  }

  /// \brief Extract a component of the array.
  ///
  /// This method returns an array that holds the data for a given flat component of the data.
  /// The `BaseComponentType` has to be specified and must match the contained array (i.e.
  /// the result of `IsBaseComponentType` must succeed for the given type).
  ///
  /// This method treats each value in the array as a flat `Vec` even if it is a `Vec` of
  /// `Vec`s. For example, if the array actually holds values of type `Vec<Vec<T, 3>, 2>`,
  /// it is treated as if it holds a `Vec<T, 6>`. See `vtkm::VecFlat` for details on how
  /// vectors are flattened.
  ///
  /// The point of using `ExtractComponent` over `AsArrayHandle` is that it drastically reduces
  /// the amount of types you have to try. Most of the type the base component type is one of
  /// the basic C types (i.e. `int`, `long`, `float`, etc.). You do not need to know what shape
  /// the containing `Vec` is in, nor do you need to know the actual storage of the array.
  ///
  /// Note that the type of the array returned is `ArrayHandleStride`. Using this type of
  /// array handle has a slight overhead over basic arrays like `ArrayHandleBasic` and
  /// `ArrayHandleSOA`.
  ///
  /// When extracting a component of an array, a shallow pointer to the data is returned
  /// whenever possible. However, in some circumstances it is impossible to conform the
  /// array. In these cases, the data are by default copied. If copying the data would
  /// cause problems (for example, you are writing into the array), you can select the
  /// optional `allowCopy` flag to `vtkm::CopyFlag::Off`. In this case, an exception
  /// will be thrown if the result cannot be represented by a shallow copy.
  ///
  template <typename BaseComponentType>
  VTKM_CONT vtkm::cont::ArrayHandleStride<BaseComponentType> ExtractComponent(
    vtkm::IdComponent componentIndex,
    vtkm::CopyFlag allowCopy = vtkm::CopyFlag::On) const
  {
    using ComponentArrayType = vtkm::cont::ArrayHandleStride<BaseComponentType>;
    if (!this->IsBaseComponentType<BaseComponentType>())
    {
      VTKM_LOG_CAST_FAIL(*this, ComponentArrayType);
      throwFailedDynamicCast("UnknownArrayHandle with " + this->GetArrayTypeName(),
                             "component array of " + vtkm::cont::TypeToString<BaseComponentType>());
    }

    auto buffers = this->Container->ExtractComponent(
      this->Container->ArrayHandlePointer, componentIndex, allowCopy);
    return ComponentArrayType(buffers);
  }

  /// \brief Extract the array knowing only the component type of the array.
  ///
  /// This method returns an `ArrayHandle` that points to the data in the array. This method
  /// differs from `AsArrayHandle` because you do not need to know the exact `ValueType` and
  /// `StorageTag` of the array. Instead, you only need to know the base component type.
  ///
  /// `ExtractArrayFromComponents` works by calling the `ExtractComponent` method and then
  /// combining them together in a fancy `ArrayHandle`. This allows you to ignore the storage
  /// type of the underlying array as well as any `Vec` structure of the value type. However,
  /// it also places some limitations on how the data can be pulled from the data.
  ///
  /// First, you have to specify the base component type. This must match the data in the
  /// underlying array (as reported by `IsBaseComponentType`).
  ///
  /// Second, the array returned will have the `Vec`s flattened. For example, if the underlying
  /// array has a `ValueType` of `Vec<Vec<T, 3>, 3>`, then this method will tread the data as
  /// if it was `Vec<T, 9>`. There is no way to get an array with `Vec` of `Vec` values.
  ///
  /// Third, because the `Vec` length of the values in the returned `ArrayHandle` must be
  /// determined at runtime, that can break many assumptions of using `Vec` objects. The
  /// type is not going to be a `Vec<T,N>` type but rather an internal class that is intended
  /// to behave like that. The type should behave mostly like a `Vec`, but will have some
  /// differences that can lead to unexpected behavior. For example, this `Vec`-like object
  /// will not have a `NUM_COMPONENTS` constant static expression because it is not known
  /// at compile time. (Use the `GetNumberOfComponents` method instead.) And for the same
  /// reason you will not be able to pass these objects to classes overloaded or templated
  /// on the `Vec` type. Also, these `Vec`-like objects cannot be created as new instances.
  /// Thus, you will likely have to iterate over all components rather than do operations on
  /// the whole `Vec`.
  ///
  /// Fourth, because `ExtractArrayFromComponents` uses `ExtractComponent` to pull data from
  /// the array (which in turn uses `ArrayExtractComponent`), there are some `ArrayHandle` types
  /// that will require copying data to a new array. This could be problematic in cases where
  /// you want to write to the array. To prevent data from being copied, set the optional
  ///  `allowCopy` to `vtkm::CopyFlag::Off`. This will cause an exception to be thrown if
  /// the resulting array cannot reference the memory held in this `UnknownArrayHandle`.
  ///
  /// Fifth, component arrays are extracted using `ArrayHandleStride` as the representation
  /// for each component. This array adds a slight overhead for each lookup as it performs the
  /// arithmetic for finding the index of each component.
  ///
  template <typename BaseComponentType>
  VTKM_CONT vtkm::cont::ArrayHandleRecombineVec<BaseComponentType> ExtractArrayFromComponents(
    vtkm::CopyFlag allowCopy = vtkm::CopyFlag::On) const
  {
    vtkm::cont::ArrayHandleRecombineVec<BaseComponentType> result;
    vtkm::IdComponent numComponents = this->GetNumberOfComponentsFlat();
    for (vtkm::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      result.AppendComponentArray(this->ExtractComponent<BaseComponentType>(cIndex, allowCopy));
    }
    return result;
  }

  /// \brief Call a functor using the underlying array type.
  ///
  /// `CastAndCallForTypes` attempts to cast the held array to a specific value type,
  /// and then calls the given functor with the cast array. You must specify
  /// the `TypeList` and `StorageList` as template arguments.
  ///
  /// After the functor argument you may add any number of arguments that will be
  /// passed to the functor after the converted `ArrayHandle`.
  ///
  template <typename TypeList, typename StorageList, typename Functor, typename... Args>
  VTKM_CONT void CastAndCallForTypes(Functor&& functor, Args&&... args) const;

  /// \brief Call a functor on an array extracted from the components.
  ///
  /// `CastAndCallWithExtractedArray` behaves similarly to `CastAndCallForTypes`.
  /// It converts the contained data to an `ArrayHandle` and calls a functor with
  /// that `ArrayHandle` (and any number of optionally specified arguments).
  ///
  /// The advantage of `CastAndCallWithExtractedArray` is that you do not need to
  /// specify any `TypeList` or `StorageList`. Instead, it internally uses
  /// `ExtractArrayFromComponents` to work with most `ArrayHandle` types with only
  /// about 10 instances of the functor. In contrast, calling `CastAndCallForTypes`
  /// with, for example, `VTKM_DEFAULT_TYPE_LIST` and `VTKM_DEFAULT_STORAGE_LIST`
  /// results in many more instances of the functor but handling many fewer types
  /// of `ArrayHandle`.
  ///
  /// There are, however, costs to using this method. Details of these costs are
  /// documented for the `ExtractArrayFromComponents` method, but briefly they
  /// are that `Vec` types get flattened, the resulting array has a strange `Vec`-like
  /// value type that has many limitations on its use, there is an overhead for
  /// retrieving each value from the array, and there is a potential that data
  /// must be copied.
  ///
  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCallWithExtractedArray(Functor&& functor, Args&&... args) const;

  template <typename FunctorOrStorageList, typename... Args>
  VTKM_CONT VTKM_DEPRECATED(1.6, "Use CastAndCallForTypes.") void CastAndCall(
    FunctorOrStorageList&& functorOrStorageList,
    Args&&... args) const
  {
    this->CastAndCallImpl(vtkm::internal::IsList<FunctorOrStorageList>{},
                          std::forward<FunctorOrStorageList>(functorOrStorageList),
                          std::forward<Args>(args)...);
  }

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  VTKM_CONT void ReleaseResourcesExecution() const;

  /// Releases all resources in both the control and execution environments.
  ///
  VTKM_CONT void ReleaseResources() const;

  VTKM_CONT void PrintSummary(std::ostream& out, bool full = false) const;

private:
  // Remove this when deprecated CastAndCall is removed.
  template <typename... Args>
  VTKM_CONT void CastAndCallImpl(std::false_type, Args&&... args) const
  {
    this->CastAndCallForTypes<vtkm::TypeListCommon, vtkm::cont::StorageListCommon>(
      std::forward<Args>(args)...);
  }
  template <typename StorageList, typename... Args>
  VTKM_CONT void CastAndCallImpl(std::true_type, StorageList, Args&&... args) const
  {
    this->CastAndCallForTypes<vtkm::TypeListCommon, StorageList>(std::forward<Args>(args)...);
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
      outputArray = vtkm::cont::ArrayHandleMultiplexer<vtkm::cont::ArrayHandle<T, Ss>...>(
        unknownArray.AsArrayHandle<ArrayType>());
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
    if (!called && unknownArray.CanConvert<DerivedArrayType>())
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

} // namespace detail

namespace internal
{

namespace detail
{

template <typename T>
struct IsUndefinedArrayType
{
};
template <typename T, typename S>
struct IsUndefinedArrayType<vtkm::List<T, S>> : vtkm::cont::internal::IsInvalidArrayHandle<T, S>
{
};

} // namespace detail

template <typename ValueTypeList, typename StorageTypeList>
using ListAllArrayTypes =
  vtkm::ListRemoveIf<vtkm::ListCross<ValueTypeList, StorageTypeList>, detail::IsUndefinedArrayType>;

VTKM_CONT_EXPORT void ThrowCastAndCallException(const vtkm::cont::UnknownArrayHandle&,
                                                const std::type_info&);

} // namespace internal

template <typename TypeList, typename StorageTagList, typename Functor, typename... Args>
inline void UnknownArrayHandle::CastAndCallForTypes(Functor&& f, Args&&... args) const
{
  using crossProduct = internal::ListAllArrayTypes<TypeList, StorageTagList>;

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
    internal::ThrowCastAndCallException(*this, typeid(TypeList));
  }
}


//=============================================================================
// Free function casting helpers

/// Returns true if \c variant matches the type of ArrayHandleType.
///
template <typename ArrayHandleType>
VTKM_CONT inline bool IsType(const vtkm::cont::UnknownArrayHandle& array)
{
  return array.template IsType<ArrayHandleType>();
}

/// Returns \c variant cast to the given \c ArrayHandle type. Throws \c
/// ErrorBadType if the cast does not work. Use \c IsType
/// to check if the cast can happen.
///
template <typename ArrayHandleType>
VTKM_CONT inline ArrayHandleType Cast(const vtkm::cont::UnknownArrayHandle& array)
{
  return array.template AsArrayHandle<ArrayHandleType>();
}

namespace detail
{

struct UnknownArrayHandleTryExtract
{
  template <typename T, typename Functor, typename... Args>
  void operator()(T,
                  Functor&& f,
                  bool& called,
                  const vtkm::cont::UnknownArrayHandle& unknownArray,
                  Args&&... args) const
  {
    if (!called && unknownArray.IsBaseComponentType<T>())
    {
      called = true;
      auto extractedArray = unknownArray.ExtractArrayFromComponents<T>();
      VTKM_LOG_CAST_SUCC(unknownArray, extractedArray);

      // If you get a compile error here, it means that you have called
      // CastAndCallWithExtractedArray for a vtkm::cont::UnknownArrayHandle and the arguments of
      // the functor do not match those being passed. This is often because it is calling the
      // functor with an ArrayHandle type that was not expected. Add overloads to the functor to
      // accept all possible array types or constrain the types tried for the CastAndCall. Note
      // that the functor will be called with an array of type that is different than the actual
      // type of the `ArrayHandle` stored in the `UnknownArrayHandle`.
      f(extractedArray, std::forward<Args>(args)...);
    }
  }
};

} // namespace detail

template <typename Functor, typename... Args>
inline void UnknownArrayHandle::CastAndCallWithExtractedArray(Functor&& functor,
                                                              Args&&... args) const
{
  bool called = false;
  vtkm::ListForEach(detail::UnknownArrayHandleTryExtract{},
                    vtkm::TypeListScalarAll{},
                    std::forward<Functor>(functor),
                    called,
                    *this,
                    std::forward<Args>(args)...);
  if (!called)
  {
    // Throw an exception.
    // The message will be a little wonky because the types are just the value types, not the
    // full type to cast to.
    VTKM_LOG_CAST_FAIL(*this, vtkm::TypeListScalarAll);
    internal::ThrowCastAndCallException(*this, typeid(vtkm::TypeListScalarAll));
  }
}

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
