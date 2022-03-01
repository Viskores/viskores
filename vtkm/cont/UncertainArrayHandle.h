//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_UncertainArrayHandle_h
#define vtk_m_cont_UncertainArrayHandle_h

#include <vtkm/cont/CastAndCall.h>
#include <vtkm/cont/UnknownArrayHandle.h>

namespace vtkm
{
namespace cont
{

/// \brief An ArrayHandle of an uncertain value type and storage.
///
/// `UncertainArrayHandle` holds an `ArrayHandle` object using runtime polymorphism
/// to manage different value and storage types. It behaves like its superclass,
/// `UnknownArrayHandle`, except that it also contains two template parameters that
/// provide `vtkm::List`s of potential value and storage types, respectively.
///
/// These potential value and storage types come into play when the `CastAndCall`
/// method is called (or the `UncertainArrayHandle` is used in the
/// `vtkm::cont::CastAndCall` function). In this case, the `CastAndCall` will
/// search for `ArrayHandle`s of types that match these two lists.
///
/// Both `UncertainArrayHandle` and `UnknownArrayHandle` have a method named
/// `ResetTypes` that redefine the lists of potential value and storage types
/// by returning a new `UncertainArrayHandle` containing the same `ArrayHandle`
/// but with the new value and storage type lists.
///
template <typename ValueTypeList, typename StorageTypeList>
class VTKM_ALWAYS_EXPORT UncertainArrayHandle : public vtkm::cont::UnknownArrayHandle
{
  VTKM_IS_LIST(ValueTypeList);
  VTKM_IS_LIST(StorageTypeList);

  VTKM_STATIC_ASSERT_MSG((!std::is_same<ValueTypeList, vtkm::ListUniversal>::value),
                         "Cannot use vtkm::ListUniversal with UncertainArrayHandle.");
  VTKM_STATIC_ASSERT_MSG((!std::is_same<StorageTypeList, vtkm::ListUniversal>::value),
                         "Cannot use vtkm::ListUniversal with UncertainArrayHandle.");

  using Superclass = UnknownArrayHandle;
  using Thisclass = UncertainArrayHandle<ValueTypeList, StorageTypeList>;

public:
  VTKM_CONT UncertainArrayHandle() = default;

  template <typename T, typename S>
  VTKM_CONT UncertainArrayHandle(const vtkm::cont::ArrayHandle<T, S>& array)
    : Superclass(array)
  {
  }

  explicit VTKM_CONT UncertainArrayHandle(const vtkm::cont::UnknownArrayHandle& src)
    : Superclass(src)
  {
  }

  template <typename OtherValues, typename OtherStorage>
  VTKM_CONT UncertainArrayHandle(const UncertainArrayHandle<OtherValues, OtherStorage>& src)
    : Superclass(src)
  {
  }

  /// \brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new `UncertainArrayHandle` for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  VTKM_CONT Thisclass NewInstance() const { return Thisclass(this->Superclass::NewInstance()); }

  /// Like `ResetTypes` except it only resets the value types.
  ///
  template <typename NewValueTypeList>
  VTKM_CONT UncertainArrayHandle<NewValueTypeList, StorageTypeList> ResetValueTypes(
    NewValueTypeList = NewValueTypeList{}) const
  {
    return this->ResetTypes<NewValueTypeList, StorageTypeList>();
  }

  /// Like `ResetTypes` except it only resets the storage types.
  ///
  template <typename NewStorageTypeList>
  VTKM_CONT UncertainArrayHandle<ValueTypeList, NewStorageTypeList> ResetStorageTypes(
    NewStorageTypeList = NewStorageTypeList{}) const
  {
    return this->ResetTypes<ValueTypeList, NewStorageTypeList>();
  }

  /// \brief Call a functor using the underlying array type.
  ///
  /// `CastAndCall` attempts to cast the held array to a specific value type,
  /// and then calls the given functor with the cast array.
  ///
  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCall(Functor&& functor, Args&&... args) const
  {
    this->CastAndCallForTypes<ValueTypeList, StorageTypeList>(std::forward<Functor>(functor),
                                                              std::forward<Args>(args)...);
  }

  /// \brief Call a functor using the underlying array type with a float cast fallback.
  ///
  /// `CastAndCallWithFloatFallback` attempts to cast the held array to a specific value type,
  /// and then calls the given functor with the cast array. If the underlying array
  /// does not match any of the requested array types, the array is copied to a new
  /// `ArrayHandleBasic` with `FloatDefault` components in its value and attempts to
  /// cast to those types.
  ///
  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCallWithFloatFallback(Functor&& functor, Args&&... args) const
  {
    this->template CastAndCallForTypesWithFloatFallback<ValueTypeList, StorageTypeList>(
      std::forward<Functor>(functor), std::forward<Args>(args)...);
  }
};

// Defined here to avoid circular dependencies between UnknownArrayHandle and UncertainArrayHandle.
template <typename NewValueTypeList, typename NewStorageTypeList>
VTKM_CONT vtkm::cont::UncertainArrayHandle<NewValueTypeList, NewStorageTypeList>
  UnknownArrayHandle::ResetTypes(NewValueTypeList, NewStorageTypeList) const
{
  return vtkm::cont::UncertainArrayHandle<NewValueTypeList, NewStorageTypeList>(*this);
}

namespace internal
{

template <typename ValueTypeList, typename StorageTypeList>
struct DynamicTransformTraits<vtkm::cont::UncertainArrayHandle<ValueTypeList, StorageTypeList>>
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

template <typename ValueTypeList, typename StorageTypeList>
struct SerializableTypeString<vtkm::cont::UncertainArrayHandle<ValueTypeList, StorageTypeList>>
{
  static VTKM_CONT std::string Get() { return "UncertainAH"; }
};
}
} // namespace vtkm::cont

namespace mangled_diy_namespace
{

namespace internal
{

struct UncertainArrayHandleSerializeFunctor
{
  template <typename ArrayHandleType>
  void operator()(const ArrayHandleType& ah, BinaryBuffer& bb) const
  {
    vtkmdiy::save(bb, vtkm::cont::SerializableTypeString<ArrayHandleType>::Get());
    vtkmdiy::save(bb, ah);
  }
};

struct UncertainArrayHandleDeserializeFunctor
{
  template <typename T, typename S>
  void operator()(vtkm::List<T, S>,
                  vtkm::cont::UnknownArrayHandle& unknownArray,
                  const std::string& typeString,
                  bool& success,
                  BinaryBuffer& bb) const
  {
    using ArrayHandleType = vtkm::cont::ArrayHandle<T, S>;

    if (!success && (typeString == vtkm::cont::SerializableTypeString<ArrayHandleType>::Get()))
    {
      ArrayHandleType knownArray;
      vtkmdiy::load(bb, knownArray);
      unknownArray = knownArray;
      success = true;
    }
  }
};

} // internal

template <typename ValueTypeList, typename StorageTypeList>
struct Serialization<vtkm::cont::UncertainArrayHandle<ValueTypeList, StorageTypeList>>
{
  using Type = vtkm::cont::UncertainArrayHandle<ValueTypeList, StorageTypeList>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& obj)
  {
    obj.CastAndCall(internal::UncertainArrayHandleSerializeFunctor{}, bb);
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& obj)
  {
    std::string typeString;
    vtkmdiy::load(bb, typeString);

    bool success = false;
    vtkm::ListForEach(internal::UncertainArrayHandleDeserializeFunctor{},
                      vtkm::cont::internal::ListAllArrayTypes<ValueTypeList, StorageTypeList>{},
                      obj,
                      typeString,
                      success,
                      bb);

    if (!success)
    {
      throw vtkm::cont::ErrorBadType(
        "Error deserializing Unknown/UncertainArrayHandle. Message TypeString: " + typeString);
    }
  }
};

} // namespace mangled_diy_namespace

/// @endcond SERIALIZATION

#endif //vtk_m_cont_UncertainArrayHandle_h
