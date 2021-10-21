//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_VariantArrayHandle_h
#define vtk_m_cont_VariantArrayHandle_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/TypeList.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandleMultiplexer.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/CastAndCall.h>
#include <vtkm/cont/DefaultTypes.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/StorageList.h>
#include <vtkm/cont/UncertainArrayHandle.h>
#include <vtkm/cont/UnknownArrayHandle.h>

#include <sstream>

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
#include <vtkm/cont/ArrayHandleVirtual.h>
#endif //VTKM_NO_DEPRECATED_VIRTUAL

// This is a deprecated class. Don't warn about deprecation while implementing
// deprecated functionality.
VTKM_DEPRECATED_SUPPRESS_BEGIN

namespace vtkm
{
namespace cont
{

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
namespace internal
{
namespace variant
{

struct ForceCastToVirtual
{
  template <typename SrcValueType, typename Storage, typename DstValueType>
  VTKM_CONT typename std::enable_if<std::is_same<SrcValueType, DstValueType>::value>::type
  operator()(const vtkm::cont::ArrayHandle<SrcValueType, Storage>& input,
             vtkm::cont::ArrayHandleVirtual<DstValueType>& output) const
  { // ValueTypes match
    output = vtkm::cont::make_ArrayHandleVirtual<DstValueType>(input);
  }

  template <typename SrcValueType, typename Storage, typename DstValueType>
  VTKM_CONT typename std::enable_if<!std::is_same<SrcValueType, DstValueType>::value>::type
  operator()(const vtkm::cont::ArrayHandle<SrcValueType, Storage>& input,
             vtkm::cont::ArrayHandleVirtual<DstValueType>& output) const
  { // ValueTypes do not match
    this->ValidateWidthAndCast<SrcValueType, DstValueType>(input, output);
  }

private:
  template <typename S,
            typename D,
            typename InputType,
            vtkm::IdComponent SSize = vtkm::VecTraits<S>::NUM_COMPONENTS,
            vtkm::IdComponent DSize = vtkm::VecTraits<D>::NUM_COMPONENTS>
  VTKM_CONT typename std::enable_if<SSize == DSize>::type ValidateWidthAndCast(
    const InputType& input,
    vtkm::cont::ArrayHandleVirtual<D>& output) const
  { // number of components match
    auto casted = vtkm::cont::make_ArrayHandleCast<D>(input);
    output = vtkm::cont::make_ArrayHandleVirtual<D>(casted);
  }

  template <typename S,
            typename D,
            vtkm::IdComponent SSize = vtkm::VecTraits<S>::NUM_COMPONENTS,
            vtkm::IdComponent DSize = vtkm::VecTraits<D>::NUM_COMPONENTS>
  VTKM_CONT typename std::enable_if<SSize != DSize>::type ValidateWidthAndCast(
    const ArrayHandleBase&,
    ArrayHandleBase&) const
  { // number of components do not match
    std::ostringstream str;
    str << "VariantArrayHandle::AsVirtual: Cannot cast from " << vtkm::cont::TypeToString<S>()
        << " to " << vtkm::cont::TypeToString<D>()
        << "; "
           "number of components must match exactly.";
    throw vtkm::cont::ErrorBadType(str.str());
  }
};

template <typename S>
struct NoCastStorageTransformImpl
{
  using type = S;
};
template <typename T, typename S>
struct NoCastStorageTransformImpl<vtkm::cont::StorageTagCast<T, S>>
{
  using type = S;
};
template <typename S>
using NoCastStorageTransform = typename NoCastStorageTransformImpl<S>::type;

}
} // namespace internal::variant
#endif //VTKM_NO_DEPRECATED_VIRTUAL

/// \brief VariantArrayHandle superclass holding common operations.
///
/// `VariantArrayHandleCommon` is a superclass to all `VariantArrayHandleBase`
/// and `VariantArrayHandle` classes. It contains all operations that are
/// independent of the type lists defined for these templated class or has
/// versions of methods that allow you to specify type lists.
///
/// See the documentation of `VariantArrayHandleBase` for more information.
///
class VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(
  1.7,
  "VariantArrayHandle classes replaced with UnknownArrayHandle and UncertainArrayHandle.")
  VariantArrayHandleCommon : public vtkm::cont::UnknownArrayHandle
{
  using Superclass = vtkm::cont::UnknownArrayHandle;

public:
  using Superclass::Superclass;

  VTKM_CONT VariantArrayHandleCommon() = default;

  VTKM_CONT VariantArrayHandleCommon(const vtkm::cont::UnknownArrayHandle& array)
    : Superclass(array)
  {
  }

  /// Returns this array cast to the given \c ArrayHandle type. Throws \c
  /// ErrorBadType if the cast does not work. Use \c IsType
  /// to check if the cast can happen.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT ArrayHandleType Cast() const
  {
    return this->AsArrayHandle<ArrayHandleType>();
  }

  /// \brief Call a functor using the underlying array type.
  ///
  /// `CastAndCall` attempts to cast the held array to a specific value type,
  /// and then calls the given functor with the cast array. You must specify
  /// the `TypeList` and `StorageList` as template arguments. (Note that this
  /// calling differs from that of the `CastAndCall` methods of subclasses.)
  ///
  template <typename TypeList, typename StorageList, typename Functor, typename... Args>
  VTKM_CONT void CastAndCall(Functor&& functor, Args&&... args) const
  {
    this->CastAndCallForTypes<TypeList, StorageList>(std::forward<Functor>(functor),
                                                     std::forward<Args>(args)...);
  }

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
  /// Returns this array cast to a `ArrayHandleVirtual` of the given type.
  /// This will perform type conversions as necessary, and will log warnings
  /// if the conversion is lossy.
  ///
  /// This method internally uses `CastAndCall`. A custom storage tag list may
  /// be specified in the second template parameter, which will be passed to
  /// the CastAndCall. You can also specify a list of types to try as the optional
  /// third template argument.
  ///
  template <typename T,
            typename StorageList = VTKM_DEFAULT_STORAGE_LIST,
            typename TypeList = vtkm::List<T>>
  VTKM_CONT VTKM_DEPRECATED(1.6, "ArrayHandleVirtual is no longer supported.")
    vtkm::cont::ArrayHandleVirtual<T> AsVirtual() const
  {
    VTKM_IS_LIST(StorageList);
    VTKM_IS_LIST(TypeList);
    // Remove cast storage from storage list because we take care of casting elsewhere
    using CleanStorageList =
      vtkm::ListTransform<StorageList, vtkm::cont::internal::variant::NoCastStorageTransform>;
    vtkm::cont::internal::variant::ForceCastToVirtual caster;
    vtkm::cont::ArrayHandleVirtual<T> output;
    this->CastAndCall<TypeList, CleanStorageList>(caster, output);
    return output;
  }
#endif //VTKM_NO_DEPRECATED_VIRTUAL

  /// Returns this array cast to a `ArrayHandleMultiplexer` of the given type.
  /// This will attempt to cast the internal array to each supported type of
  /// the multiplexer. If none are supported, an invalid ArrayHandleMultiplexer
  /// is returned.
  ///
  /// As a special case, if one of the arrays in the `ArrayHandleMultiplexer`'s
  /// type list is an `ArrayHandleCast`, then the multiplexer will look for type
  /// type of array being cast rather than an actual cast array.
  ///
  ///@{
  template <typename... T>
  VTKM_CONT void AsMultiplexer(vtkm::cont::ArrayHandleMultiplexer<T...>& result) const
  {
    this->AsArrayHandle(result);
  }

  template <typename ArrayHandleMultiplexerType>
  VTKM_CONT ArrayHandleMultiplexerType AsMultiplexer() const
  {
    ArrayHandleMultiplexerType result;
    this->AsMultiplexer(result);
    return result;
  }
  ///@}

  /// Given a references to an ArrayHandle object, casts this array to the
  /// ArrayHandle's type and sets the given ArrayHandle to this array. Throws
  /// `ErrorBadType` if the cast does not work. Use `IsType` to check
  /// if the cast can happen.
  ///
  /// Note that this is a shallow copy. The data are not copied and a change
  /// in the data in one array will be reflected in the other.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT void CopyTo(ArrayHandleType& array) const
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    array = this->Cast<ArrayHandleType>();
  }

  /// \brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new variant array handle for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  VTKM_CONT VariantArrayHandleCommon NewInstance() const
  {
    return VariantArrayHandleCommon(this->Superclass::NewInstance());
  }
};

/// \brief Holds an array handle without having to specify template parameters.
///
/// `VariantArrayHandle` holds an `ArrayHandle`
/// object using runtime polymorphism to manage different value types and
/// storage rather than compile-time templates. This adds a programming
/// convenience that helps avoid a proliferation of templates. It also provides
/// the management necessary to interface VTK-m with data sources where types
/// will not be known until runtime.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in VTK-m, `VariantArrayHandle` contains a method named `CastAndCall` that
/// will determine the correct type from some known list of types.
/// This mechanism is used internally by VTK-m's worklet invocation
/// mechanism to determine the type when running algorithms.
///
/// By default, `VariantArrayHandle` will assume that the value type in the
/// array matches one of the types specified by `VTKM_DEFAULT_TYPE_LIST`
/// This list can be changed by using the `ResetTypes`. It is
/// worthwhile to match these lists closely to the possible types that might be
/// used. If a type is missing you will get a runtime error. If there are more
/// types than necessary, then the template mechanism will create a lot of
/// object code that is never used, and keep in mind that the number of
/// combinations grows exponentially when using multiple `VariantArrayHandle`
/// objects.
///
/// The actual implementation of `VariantArrayHandle` is in a templated class
/// named `VariantArrayHandleBase`, which is templated on the list of
/// component types.
///
template <typename TypeList>
class VTKM_ALWAYS_EXPORT VTKM_DEPRECATED(
  1.7,
  "VariantArrayHandle classes replaced with UnknownArrayHandle and UncertainArrayHandle.")
  VariantArrayHandleBase : public VariantArrayHandleCommon
{
  VTKM_STATIC_ASSERT_MSG((!std::is_same<TypeList, vtkm::ListUniversal>::value),
                         "Cannot use vtkm::ListUniversal with VariantArrayHandle.");

  using Superclass = VariantArrayHandleCommon;

public:
  VTKM_CONT
  VariantArrayHandleBase() = default;

  template <typename T, typename Storage>
  VTKM_CONT VariantArrayHandleBase(const vtkm::cont::ArrayHandle<T, Storage>& array)
    : Superclass(array)
  {
  }

  VTKM_CONT explicit VariantArrayHandleBase(const VariantArrayHandleCommon& src)
    : Superclass(src)
  {
  }

  VTKM_CONT VariantArrayHandleBase(const vtkm::cont::UnknownArrayHandle& src)
    : Superclass(src)
  {
  }

  VTKM_CONT VariantArrayHandleBase(const VariantArrayHandleBase&) = default;
  VTKM_CONT VariantArrayHandleBase(VariantArrayHandleBase&&) noexcept = default;

  VTKM_CONT
  ~VariantArrayHandleBase() {}

  VTKM_CONT
  VariantArrayHandleBase<TypeList>& operator=(const VariantArrayHandleBase<TypeList>&) = default;

  VTKM_CONT
  VariantArrayHandleBase<TypeList>& operator=(VariantArrayHandleBase<TypeList>&&) noexcept =
    default;

  VTKM_CONT operator vtkm::cont::UncertainArrayHandle<TypeList, VTKM_DEFAULT_STORAGE_LIST>() const
  {
    return vtkm::cont::UncertainArrayHandle<TypeList, VTKM_DEFAULT_STORAGE_LIST>(*this);
  }


#ifndef VTKM_NO_DEPRECATED_VIRTUAL
  /// Returns this array cast to a \c ArrayHandleVirtual of the given type.
  /// This will perform type conversions as necessary, and will log warnings
  /// if the conversion is lossy.
  ///
  /// This method internally uses CastAndCall. A custom storage tag list may
  /// be specified in the second template parameter, which will be passed to
  /// the CastAndCall.
  ///
  template <typename T, typename StorageList = VTKM_DEFAULT_STORAGE_LIST>
  VTKM_CONT VTKM_DEPRECATED(1.6, "ArrayHandleVirtual is no longer suported.")
    vtkm::cont::ArrayHandleVirtual<T> AsVirtual() const
  {
    return this->Superclass::AsVirtual<T, StorageList, TypeList>();
  }
#endif //VTKM_NO_DEPRECATED_VIRTUAL

  /// Changes the types to try casting to when resolving this variant array,
  /// which is specified with a list tag like those in TypeList.h. Since C++
  /// does not allow you to actually change the template arguments, this method
  /// returns a new variant array object. This method is particularly useful to
  /// narrow down (or expand) the types when using an array of particular
  /// constraints.
  ///
  template <typename NewTypeList>
  VTKM_CONT VariantArrayHandleBase<NewTypeList> ResetTypes(NewTypeList = NewTypeList()) const
  {
    VTKM_IS_LIST(NewTypeList);
    return VariantArrayHandleBase<NewTypeList>(*this);
  }

  //@{
  /// \brief Call a functor using the underlying array type.
  ///
  /// `CastAndCall` attempts to cast the held array to a specific value type,
  /// then call the given functor with the cast array. The types
  /// tried in the cast are those in the lists defined by the TypeList.
  /// By default `VariantArrayHandle` set this to `VTKM_DEFAULT_TYPE_LIST`.
  ///
  /// In addition to the value type, an `ArrayHandle` also requires a storage tag.
  /// By default, `CastAndCall` attempts to cast the array using the storage tags
  /// listed in `VTKM_DEFAULT_STORAGE_LIST`. You can optionally give a custom
  /// list of storage tags as the second argument.
  ///
  /// As previous stated, if a storage tag list is provided, it is given in the
  /// first argument. The functor to call with the cast array is given as the next
  /// argument (or the first argument if a storage tag list is not provided).
  /// The remaning arguments, if any, are passed to the functor.
  ///
  /// The functor will be called with the cast array as its first argument. Any
  /// remaining arguments are passed from the arguments to `CastAndCall`.
  ///
  template <typename FunctorOrStorageList, typename... Args>
  VTKM_CONT void CastAndCall(FunctorOrStorageList&& functorOrStorageList, Args&&... args) const
  {
    this->CastAndCallImpl(vtkm::internal::IsList<FunctorOrStorageList>(),
                          std::forward<FunctorOrStorageList>(functorOrStorageList),
                          std::forward<Args>(args)...);
  }

  template <typename Functor>
  VTKM_CONT void CastAndCall(Functor&& f) const
  {
    this->CastAndCallImpl(std::false_type(), std::forward<Functor>(f));
  }
  //@}

  /// \brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new variant array handle for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  VTKM_CONT VariantArrayHandleBase<TypeList> NewInstance() const
  {
    return VariantArrayHandleBase<TypeList>(this->Superclass::NewInstance());
  }

private:
  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCallImpl(std::false_type, Functor&& f, Args&&... args) const
  {
    this->Superclass::CastAndCall<TypeList, VTKM_DEFAULT_STORAGE_LIST>(std::forward<Functor>(f),
                                                                       std::forward<Args>(args)...);
  }

  template <typename StorageList, typename Functor, typename... Args>
  VTKM_CONT void CastAndCallImpl(std::true_type, StorageList, Functor&& f, Args&&... args) const
  {
    this->Superclass::CastAndCall<TypeList, StorageList>(std::forward<Functor>(f),
                                                         std::forward<Args>(args)...);
  }
};

using VariantArrayHandle VTKM_DEPRECATED(
  1.7,
  "VariantArrayHandle classes replaced with UnknownArrayHandle and UncertainArrayHandle.") =
  vtkm::cont::VariantArrayHandleBase<VTKM_DEFAULT_TYPE_LIST>;


//=============================================================================
// Free function casting helpers

/// Returns true if \c variant matches the type of ArrayHandleType.
///
template <typename ArrayHandleType, typename Ts>
VTKM_CONT inline bool IsType(const vtkm::cont::VariantArrayHandleBase<Ts>& variant)
{
  return variant.template IsType<ArrayHandleType>();
}

/// Returns \c variant cast to the given \c ArrayHandle type. Throws \c
/// ErrorBadType if the cast does not work. Use \c IsType
/// to check if the cast can happen.
///
template <typename ArrayHandleType, typename Ts>
VTKM_CONT inline ArrayHandleType Cast(const vtkm::cont::VariantArrayHandleBase<Ts>& variant)
{
  return variant.template Cast<ArrayHandleType>();
}

namespace internal
{

template <typename TypeList>
struct DynamicTransformTraits<vtkm::cont::VariantArrayHandleBase<TypeList>>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal
} // namespace cont
} // namespace vtkm

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace mangled_diy_namespace
{

template <typename TypeList>
struct Serialization<vtkm::cont::VariantArrayHandleBase<TypeList>>
{
private:
  using Type = vtkm::cont::VariantArrayHandleBase<TypeList>;
  using ImplObject = vtkm::cont::UncertainArrayHandle<TypeList, VTKM_DEFAULT_STORAGE_LIST>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    vtkmdiy::save(bb, ImplObject(obj));
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& obj)
  {
    ImplObject implObj;
    vtkmdiy::load(bb, implObj);
    obj = implObj;
  }
};

} // diy
/// @endcond SERIALIZATION

VTKM_DEPRECATED_SUPPRESS_END


#endif //vtk_m_virts_VariantArrayHandle_h
