//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_DynamicArrayHandle_h
#define vtk_m_cont_DynamicArrayHandle_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/TypeListTag.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/StorageListTag.h>

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/internal/DynamicTransform.h>

namespace vtkm
{

template struct ListCrossProduct<VTKM_DEFAULT_TYPE_LIST_TAG, VTKM_DEFAULT_STORAGE_LIST_TAG>;

namespace cont
{

// Forward declaration
template <typename TypeList, typename StorageList>
class DynamicArrayHandleBase;

namespace detail
{

/// \brief Base class for PolymorphicArrayHandleContainer
///
struct VTKM_CONT_EXPORT PolymorphicArrayHandleContainerBase
{
  PolymorphicArrayHandleContainerBase();

  // This must exist so that subclasses are destroyed correctly.
  virtual ~PolymorphicArrayHandleContainerBase();

  virtual vtkm::IdComponent GetNumberOfComponents() const = 0;
  virtual vtkm::Id GetNumberOfValues() const = 0;

  virtual void PrintSummary(std::ostream& out) const = 0;

  virtual std::shared_ptr<PolymorphicArrayHandleContainerBase> NewInstance() const = 0;
};

/// \brief ArrayHandle container that can use C++ run-time type information.
///
/// The \c PolymorphicArrayHandleContainer is similar to the
/// \c SimplePolymorphicContainer in that it can contain an object of an
/// unknown type. However, this class specifically holds ArrayHandle objects
/// (with different template parameters) so that it can polymorphically answer
/// simple questions about the object.
///
template <typename T, typename Storage>
struct VTKM_ALWAYS_EXPORT PolymorphicArrayHandleContainer
  : public PolymorphicArrayHandleContainerBase
{
  using ArrayHandleType = vtkm::cont::ArrayHandle<T, Storage>;

  ArrayHandleType Array;

  VTKM_CONT
  PolymorphicArrayHandleContainer()
    : Array()
  {
  }

  VTKM_CONT
  PolymorphicArrayHandleContainer(const ArrayHandleType& array)
    : Array(array)
  {
  }

  virtual vtkm::IdComponent GetNumberOfComponents() const
  {
    return vtkm::VecTraits<T>::NUM_COMPONENTS;
  }

  virtual vtkm::Id GetNumberOfValues() const { return this->Array.GetNumberOfValues(); }

  virtual void PrintSummary(std::ostream& out) const
  {
    vtkm::cont::printSummary_ArrayHandle(this->Array, out);
  }

  virtual std::shared_ptr<PolymorphicArrayHandleContainerBase> NewInstance() const
  {
    return std::shared_ptr<PolymorphicArrayHandleContainerBase>(
      new PolymorphicArrayHandleContainer<T, Storage>());
  }
};

// One instance of a template class cannot access the private members of
// another instance of a template class. However, I want to be able to copy
// construct a DynamicArrayHandle from another DynamicArrayHandle of any other
// type. Since you cannot partially specialize friendship, use this accessor
// class to get at the internals for the copy constructor.
struct DynamicArrayHandleCopyHelper
{
  template <typename TypeList, typename StorageList>
  VTKM_CONT static const std::shared_ptr<vtkm::cont::detail::PolymorphicArrayHandleContainerBase>&
  GetArrayHandleContainer(const vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList>& src)
  {
    return src.ArrayContainer;
  }
};

// A simple function to downcast an ArrayHandle encapsulated in a
// PolymorphicArrayHandleContainerBase to the given type of ArrayHandle. If the
// conversion cannot be done, nullptr is returned.
template <typename Type, typename Storage>
VTKM_CONT vtkm::cont::ArrayHandle<Type, Storage>* DynamicArrayHandleTryCast(
  vtkm::cont::detail::PolymorphicArrayHandleContainerBase* arrayContainer)
{
  vtkm::cont::detail::PolymorphicArrayHandleContainer<Type, Storage>* downcastContainer = nullptr;
  downcastContainer = dynamic_cast<decltype(downcastContainer)>(arrayContainer);
  if (downcastContainer != nullptr)
  {
    return &downcastContainer->Array;
  }
  else
  {
    return nullptr;
  }
}

template <typename Type, typename Storage>
VTKM_CONT vtkm::cont::ArrayHandle<Type, Storage>* DynamicArrayHandleTryCast(
  const std::shared_ptr<vtkm::cont::detail::PolymorphicArrayHandleContainerBase>& arrayContainer)
{
  return detail::DynamicArrayHandleTryCast<Type, Storage>(arrayContainer.get());
}

} // namespace detail

/// \brief Holds an array handle without having to specify template parameters.
///
/// \c DynamicArrayHandle holds an \c ArrayHandle object using runtime
/// polymorphism to manage different value types and storage rather than
/// compile-time templates. This adds a programming convenience that helps
/// avoid a proliferation of templates. It also provides the management
/// necessary to interface VTK-m with data sources where types will not be
/// known until runtime.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in VTK-m, \c DynamicArrayHandle contains a method named \c CastAndCall that
/// will determine the correct type from some known list of types and storage
/// objects. This mechanism is used internally by VTK-m's worklet invocation
/// mechanism to determine the type when running algorithms.
///
/// By default, \c DynamicArrayHandle will assume that the value type in the
/// array matches one of the types specified by \c VTKM_DEFAULT_TYPE_LIST_TAG
/// and the storage matches one of the tags specified by \c
/// VTKM_DEFAULT_STORAGE_LIST_TAG. These lists can be changed by using the \c
/// ResetTypeList and \c ResetStorageList methods, respectively. It is
/// worthwhile to match these lists closely to the possible types that might be
/// used. If a type is missing you will get a runtime error. If there are more
/// types than necessary, then the template mechanism will create a lot of
/// object code that is never used, and keep in mind that the number of
/// combinations grows exponentially when using multiple \c DynamicArrayHandle
/// objects.
///
/// The actual implementation of \c DynamicArrayHandle is in a templated class
/// named \c DynamicArrayHandleBase, which is templated on the list of
/// component types and storage types. \c DynamicArrayHandle is really just a
/// typedef of \c DynamicArrayHandleBase with the default type and storage
/// lists.
///
template <typename TypeList, typename StorageList>
class VTKM_ALWAYS_EXPORT DynamicArrayHandleBase
{
public:
  VTKM_CONT
  DynamicArrayHandleBase() {}

  template <typename Type, typename Storage>
  VTKM_CONT DynamicArrayHandleBase(const vtkm::cont::ArrayHandle<Type, Storage>& array)
    : ArrayContainer(new vtkm::cont::detail::PolymorphicArrayHandleContainer<Type, Storage>(array))
  {
  }

  VTKM_CONT
  DynamicArrayHandleBase(const DynamicArrayHandleBase<TypeList, StorageList>& src)
    : ArrayContainer(src.ArrayContainer)
  {
  }

  template <typename OtherTypeList, typename OtherStorageList>
  VTKM_CONT explicit DynamicArrayHandleBase(
    const DynamicArrayHandleBase<OtherTypeList, OtherStorageList>& src)
    : ArrayContainer(detail::DynamicArrayHandleCopyHelper::GetArrayHandleContainer(src))
  {
  }

  VTKM_CONT
  ~DynamicArrayHandleBase() {}

  VTKM_CONT
  vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList>& operator=(
    const vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList>& src)
  {
    this->ArrayContainer = src.ArrayContainer;
    return *this;
  }

  /// Returns true if this array is of the provided type and uses the provided
  /// storage.
  ///
  template <typename Type, typename Storage>
  VTKM_CONT bool IsTypeAndStorage() const
  {
    return (detail::DynamicArrayHandleTryCast<Type, Storage>(this->ArrayContainer) != nullptr);
  }

  /// Returns true if this array matches the array handle type passed in.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT bool IsType()
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    using ValueType = typename ArrayHandleType::ValueType;
    using StorageTag = typename ArrayHandleType::StorageTag;
    return this->IsTypeAndStorage<ValueType, StorageTag>();
  }

  /// Returns true if the array held in this object is the same (or equivalent)
  /// type as the object given.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT bool IsSameType(const ArrayHandleType&)
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    return this->IsType<ArrayHandleType>();
  }

  /// Returns this array cast to an ArrayHandle object of the given type and
  /// storage. Throws \c ErrorBadType if the cast does not work. Use
  /// \c IsTypeAndStorage to check if the cast can happen.
  ///
  ///
  template <typename Type, typename Storage>
  VTKM_CONT vtkm::cont::ArrayHandle<Type, Storage> CastToTypeStorage() const
  {
    vtkm::cont::ArrayHandle<Type, Storage>* downcastArray =
      detail::DynamicArrayHandleTryCast<Type, Storage>(this->ArrayContainer);
    if (downcastArray == nullptr)
    {
      throw vtkm::cont::ErrorBadType("Bad cast of dynamic array.");
    }
    // Technically, this method returns a copy of the \c ArrayHandle. But
    // because \c ArrayHandle acts like a shared pointer, it is valid to
    // do the copy.
    return *downcastArray;
  }

  /// Returns this array cast to the given \c ArrayHandle type. Throws \c
  /// ErrorBadType if the cast does not work. Use \c IsType
  /// to check if the cast can happen.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT ArrayHandleType Cast() const
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    using ValueType = typename ArrayHandleType::ValueType;
    using StorageTag = typename ArrayHandleType::StorageTag;
    // Technically, this method returns a copy of the \c ArrayHandle. But
    // because \c ArrayHandle acts like a shared pointer, it is valid to
    // do the copy.
    return this->CastToTypeStorage<ValueType, StorageTag>();
  }

  /// Given a references to an ArrayHandle object, casts this array to the
  /// ArrayHandle's type and sets the given ArrayHandle to this array. Throws
  /// \c ErrorBadType if the cast does not work. Use \c
  /// ArrayHandleType to check if the cast can happen.
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

  /// Changes the types to try casting to when resolving this dynamic array,
  /// which is specified with a list tag like those in TypeListTag.h. Since C++
  /// does not allow you to actually change the template arguments, this method
  /// returns a new dynamic array object. This method is particularly useful to
  /// narrow down (or expand) the types when using an array of particular
  /// constraints.
  ///
  template <typename NewTypeList>
  VTKM_CONT DynamicArrayHandleBase<NewTypeList, StorageList> ResetTypeList(
    NewTypeList = NewTypeList()) const
  {
    VTKM_IS_LIST_TAG(NewTypeList);
    return DynamicArrayHandleBase<NewTypeList, StorageList>(*this);
  }

  /// Changes the array storage types to try casting to when resolving this
  /// dynamic array, which is specified with a list tag like those in
  /// StorageListTag.h. Since C++ does not allow you to actually change the
  /// template arguments, this method returns a new dynamic array object. This
  /// method is particularly useful to narrow down (or expand) the types when
  /// using an array of particular constraints.
  ///
  template <typename NewStorageList>
  VTKM_CONT DynamicArrayHandleBase<TypeList, NewStorageList> ResetStorageList(
    NewStorageList = NewStorageList()) const
  {
    VTKM_IS_LIST_TAG(NewStorageList);
    return DynamicArrayHandleBase<TypeList, NewStorageList>(*this);
  }

  /// Changes the value, and array storage types to try casting to when
  /// resolving this dynamic array. Since C++ does not allow you to actually
  /// change the template arguments, this method returns a new dynamic array
  /// object. This method is particularly useful when you have both custom
  /// types and traits.
  ///
  template <typename NewTypeList, typename NewStorageList>
  VTKM_CONT DynamicArrayHandleBase<NewTypeList, NewStorageList> ResetTypeAndStorageLists(
    NewTypeList = NewTypeList(),
    NewStorageList = NewStorageList()) const
  {
    VTKM_IS_LIST_TAG(NewTypeList);
    VTKM_IS_LIST_TAG(NewStorageList);
    return DynamicArrayHandleBase<NewTypeList, NewStorageList>(*this);
  }

  /// Attempts to cast the held array to a specific value type and storage,
  /// then call the given functor with the cast array. The types and storage
  /// tried in the cast are those in the lists defined by the TypeList and
  /// StorageList, respectively. The default \c DynamicArrayHandle sets these
  /// two lists to VTKM_DEFAULT_TYPE_LIST_TAG and VTK_DEFAULT_STORAGE_LIST_TAG,
  /// respectively.
  ///
  template <typename Functor, typename... Args>
  VTKM_CONT void CastAndCall(Functor&& f, Args&&...) const;

  /// \brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new dynamic array handle for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  VTKM_CONT
  DynamicArrayHandleBase<TypeList, StorageList> NewInstance() const
  {
    DynamicArrayHandleBase<TypeList, StorageList> newArray;
    newArray.ArrayContainer = this->ArrayContainer->NewInstance();
    return newArray;
  }

  /// \brief Get the number of components in each array value.
  ///
  /// This method will query the array type for the number of components in
  /// each value of the array. The number of components is determined by
  /// the \c VecTraits::NUM_COMPONENTS trait class.
  ///
  VTKM_CONT
  vtkm::IdComponent GetNumberOfComponents() const
  {
    return this->ArrayContainer->GetNumberOfComponents();
  }

  /// \brief Get the number of values in the array.
  ///
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->ArrayContainer->GetNumberOfValues(); }

  VTKM_CONT
  void PrintSummary(std::ostream& out) const { this->ArrayContainer->PrintSummary(out); }

private:
  std::shared_ptr<vtkm::cont::detail::PolymorphicArrayHandleContainerBase> ArrayContainer;

  friend struct detail::DynamicArrayHandleCopyHelper;
};

using DynamicArrayHandle =
  vtkm::cont::DynamicArrayHandleBase<VTKM_DEFAULT_TYPE_LIST_TAG, VTKM_DEFAULT_STORAGE_LIST_TAG>;

namespace detail
{

struct DynamicArrayHandleTry
{
  template <typename T, typename U, typename Functor, typename... Args>
  void operator()(brigand::list<T, U>,
                  Functor&& f,
                  bool& called,
                  const PolymorphicArrayHandleContainerBase* const container,
                  Args&&... args) const
  {
    if (!called)
    {
      using downcastType = const vtkm::cont::detail::PolymorphicArrayHandleContainer<T, U>* const;
      downcastType downcastContainer = dynamic_cast<downcastType>(container);
      if (downcastContainer)
      {
        f(downcastContainer->Array, std::forward<Args>(args)...);
        called = true;
      }
    }
  }
};

VTKM_CONT_EXPORT void ThrowCastAndCallException(PolymorphicArrayHandleContainerBase*,
                                                const std::type_info*,
                                                const std::type_info*);
} // namespace detail


template <typename T>
struct IsUndefinedStorage
{
};
template <typename T, typename U>
struct IsUndefinedStorage<brigand::list<T, U>> : vtkm::cont::internal::IsInValidArrayHandle<T, U>
{
};

template <typename TypeList, typename StorageList>
struct ListTagDynamicTypes : vtkm::detail::ListRoot
{
  using crossProduct = typename vtkm::ListCrossProduct<TypeList, StorageList>;
  // using list = typename crossProduct::list;
  using list = ::brigand::remove_if<typename crossProduct::list, IsUndefinedStorage<brigand::_1>>;
};


template <typename TypeList, typename StorageList>
template <typename Functor, typename... Args>
VTKM_CONT void DynamicArrayHandleBase<TypeList, StorageList>::CastAndCall(Functor&& f,
                                                                          Args&&... args) const
{
  //For optimizations we should compile once the cross product for the default types
  //and make it extern
  using crossProduct = ListTagDynamicTypes<TypeList, StorageList>;

  bool called = false;
  auto* ptr = this->ArrayContainer.get();
  vtkm::ListForEach(detail::DynamicArrayHandleTry{},
                    crossProduct{},
                    std::forward<Functor>(f),
                    called,
                    ptr,
                    std::forward<Args>(args)...);
  if (!called)
  {
    // throw an exception
    detail::ThrowCastAndCallException(ptr, &typeid(TypeList), &typeid(StorageList));
  }
}

namespace internal
{

template <typename TypeList, typename StorageList>
struct DynamicTransformTraits<vtkm::cont::DynamicArrayHandleBase<TypeList, StorageList>>
{
  using DynamicTag = vtkm::cont::internal::DynamicTransformTagCastAndCall;
};

} // namespace internal
}

} // namespace vtkm::cont

#endif //vtk_m_cont_DynamicArrayHandle_h
