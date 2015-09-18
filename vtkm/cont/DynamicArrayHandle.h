//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_DynamicArrayHandle_h
#define vtk_m_cont_DynamicArrayHandle_h

#include <vtkm/TypeListTag.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/StorageListTag.h>

#include <vtkm/cont/internal/DynamicTransform.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/utility/enable_if.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace cont {

// Forward declaration
template<typename TypeList, typename StorageList>
class DynamicArrayHandleBase;

namespace detail {

/// \brief Base class for PolymorphicArrayHandleContainer
///
struct PolymorphicArrayHandleContainerBase
{
  // This must exist so that subclasses are destroyed correctly.
  virtual ~PolymorphicArrayHandleContainerBase() {  }

  virtual vtkm::IdComponent GetNumberOfComponents() const = 0;
  virtual vtkm::Id GetNumberOfValues() const = 0;

  virtual void PrintSummary(std::ostream &out) const = 0;

  virtual boost::shared_ptr<PolymorphicArrayHandleContainerBase>
  NewInstance() const = 0;
};

/// \brief ArrayHandle container that can use C++ run-time type information.
///
/// The \c PolymorphicArrayHandleContainer is similar to the
/// \c SimplePolymorphicContainer in that it can contain an object of an
/// unkown type. However, this class specifically holds ArrayHandle objects
/// (with different template parameters) so that it can polymorphically answer
/// simple questions about the object.
///
template<typename T, typename Storage>
struct PolymorphicArrayHandleContainer
    : public PolymorphicArrayHandleContainerBase
{
  typedef vtkm::cont::ArrayHandle<T, Storage> ArrayHandleType;

  ArrayHandleType Array;

  VTKM_CONT_EXPORT
  PolymorphicArrayHandleContainer() : Array() {  }

  VTKM_CONT_EXPORT
  PolymorphicArrayHandleContainer(const ArrayHandleType &array)
    : Array(array) {  }

  virtual vtkm::IdComponent GetNumberOfComponents() const
  {
    return vtkm::VecTraits<T>::NUM_COMPONENTS;
  }

  virtual vtkm::Id GetNumberOfValues() const
  {
    return this->Array.GetNumberOfValues();
  }

  virtual void PrintSummary(std::ostream &out) const
  {
    vtkm::cont::printSummary_ArrayHandle(this->Array, out);
  }

  virtual boost::shared_ptr<PolymorphicArrayHandleContainerBase>
  NewInstance() const
  {
    return boost::shared_ptr<PolymorphicArrayHandleContainerBase>(
          new PolymorphicArrayHandleContainer<T,Storage>());
  }
};

// One instance of a template class cannot access the private members of
// another instance of a template class. However, I want to be able to copy
// construct a DynamicArrayHandle from another DynamicArrayHandle of any other
// type. Since you cannot partially specialize friendship, use this accessor
// class to get at the internals for the copy constructor.
struct DynamicArrayHandleCopyHelper {
  template<typename TypeList, typename StorageList>
  VTKM_CONT_EXPORT
  static
  boost::shared_ptr<vtkm::cont::detail::PolymorphicArrayHandleContainerBase>
  GetArrayHandleContainer(const vtkm::cont::DynamicArrayHandleBase<TypeList,StorageList> &src)
  {
    return src.ArrayContainer;
  }
};

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
template<typename TypeList, typename StorageList>
class DynamicArrayHandleBase
{
public:
  VTKM_CONT_EXPORT
  DynamicArrayHandleBase() {  }

  template<typename Type, typename Storage>
  VTKM_CONT_EXPORT
  DynamicArrayHandleBase(const vtkm::cont::ArrayHandle<Type,Storage> &array)
    : ArrayContainer(new vtkm::cont::detail::PolymorphicArrayHandleContainer<
                     Type,Storage>(array))
  {  }

  VTKM_CONT_EXPORT
  DynamicArrayHandleBase(
      const DynamicArrayHandleBase<TypeList,StorageList> &src)
    : ArrayContainer(src.ArrayContainer) {  }

  template<typename OtherTypeList, typename OtherStorageList>
  VTKM_CONT_EXPORT
  explicit DynamicArrayHandleBase(
      const DynamicArrayHandleBase<OtherTypeList,OtherStorageList> &src)
    : ArrayContainer(
        detail::DynamicArrayHandleCopyHelper::GetArrayHandleContainer(src))
  {  }

  /// Returns true if this array is of the provided type and uses the provided
  /// storage.
  ///
  template<typename Type, typename Storage>
  VTKM_CONT_EXPORT
  bool IsTypeAndStorage(Type = Type(), Storage = Storage()) const {
    return (this->TryCastContainer<Type,Storage>() != NULL);
  }

  /// Returns this array cast to an ArrayHandle object of the given type and
  /// storage. Throws \c ErrorControlBadValue if the cast does not work. Use
  /// \c IsTypeAndStorage to check if the cast can happen.
  ///
  template<typename Type, typename Storage>
  VTKM_CONT_EXPORT
  vtkm::cont::ArrayHandle<Type, Storage>
  CastToArrayHandle(Type = Type(), Storage = Storage()) const {
    vtkm::cont::detail::PolymorphicArrayHandleContainer<Type,Storage> *container
        = this->TryCastContainer<Type,Storage>();
    if (container == NULL)
    {
      throw vtkm::cont::ErrorControlBadValue("Bad cast of dynamic array.");
    }
    return container->Array;
  }

  /// Given a refernce to an ArrayHandle object, casts this array to the
  /// ArrayHandle's type and sets the given ArrayHandle to this array. Throws
  /// \c ErrorControlBadValue if the cast does not work. Use \c
  /// IsTypeAndStorage to check if the cast can happen.
  ///
  template<typename ArrayHandleType>
  VTKM_CONT_EXPORT
  void CastToArrayHandle(ArrayHandleType &array) const {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    typedef typename ArrayHandleType::ValueType ValueType;
    typedef typename ArrayHandleType::StorageTag StorageTag;
    array = this->CastToArrayHandle(ValueType(), StorageTag());
  }

  /// Changes the types to try casting to when resolving this dynamic array,
  /// which is specified with a list tag like those in TypeListTag.h. Since C++
  /// does not allow you to actually change the template arguments, this method
  /// returns a new dynamic array object. This method is particularly useful to
  /// narrow down (or expand) the types when using an array of particular
  /// constraints.
  ///
  template<typename NewTypeList>
  VTKM_CONT_EXPORT
  DynamicArrayHandleBase<NewTypeList,StorageList>
  ResetTypeList(NewTypeList = NewTypeList()) const {
    VTKM_IS_LIST_TAG(NewTypeList);
    return DynamicArrayHandleBase<NewTypeList,StorageList>(*this);
  }

  /// Changes the array storage types to try casting to when resolving this
  /// dynamic array, which is specified with a list tag like those in
  /// StorageListTag.h. Since C++ does not allow you to actually change the
  /// template arguments, this method returns a new dynamic array object. This
  /// method is particularly useful to narrow down (or expand) the types when
  /// using an array of particular constraints.
  ///
  template<typename NewStorageList>
  VTKM_CONT_EXPORT
  DynamicArrayHandleBase<TypeList,NewStorageList>
  ResetStorageList(NewStorageList = NewStorageList()) const {
    VTKM_IS_LIST_TAG(NewStorageList);
    return DynamicArrayHandleBase<TypeList,NewStorageList>(*this);
  }

  /// Attempts to cast the held array to a specific value type and storage,
  /// then call the given functor with the cast array. The types and storage
  /// tried in the cast are those in the lists defined by the TypeList and
  /// StorageList, respectively. The default \c DynamicArrayHandle sets these
  /// two lists to VTKM_DEFAULT_TYPE_LIST_TAG and VTK_DEFAULT_STORAGE_LIST_TAG,
  /// respectively.
  ///
  template<typename Functor>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f) const;

  /// \brief Create a new array of the same type as this array.
  ///
  /// This method creates a new array that is the same type as this one and
  /// returns a new dynamic array handle for it. This method is convenient when
  /// creating output arrays that should be the same type as some input array.
  ///
  VTKM_CONT_EXPORT
  DynamicArrayHandleBase<TypeList,StorageList> NewInstance() const
  {
    DynamicArrayHandleBase<TypeList,StorageList> newArray;
    newArray.ArrayContainer = this->ArrayContainer->NewInstance();
    return newArray;
  }

  /// \brief Get the number of components in each array value.
  ///
  /// This method will query the array type for the number of components in
  /// each value of the array. The number of components is determined by
  /// the \c VecTraits::NUM_COMPONENTS trait class.
  ///
  VTKM_CONT_EXPORT
  vtkm::IdComponent GetNumberOfComponents() const
  {
    return this->ArrayContainer->GetNumberOfComponents();
  }

  /// \brief Get the number of values in the array.
  ///
  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const
  {
    return this->ArrayContainer->GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  virtual void PrintSummary(std::ostream &out) const
  {
    this->ArrayContainer->PrintSummary(out);
  }

private:
  boost::shared_ptr<vtkm::cont::detail::PolymorphicArrayHandleContainerBase>
    ArrayContainer;

  friend struct detail::DynamicArrayHandleCopyHelper;

  template<typename Type, typename Storage>
  VTKM_CONT_EXPORT
  vtkm::cont::detail::PolymorphicArrayHandleContainer<Type,Storage> *
  TryCastContainer() const {
    return
        dynamic_cast<
          vtkm::cont::detail::PolymorphicArrayHandleContainer<Type,Storage> *>(
            this->ArrayContainer.get());
  }
};

typedef vtkm::cont::DynamicArrayHandleBase<
    VTKM_DEFAULT_TYPE_LIST_TAG, VTKM_DEFAULT_STORAGE_LIST_TAG>
    DynamicArrayHandle;

namespace detail {

template<typename Functor, typename Type>
struct DynamicArrayHandleTryStorage {
  const DynamicArrayHandle Array;
  const Functor &Function;
  bool FoundCast;

  VTKM_CONT_EXPORT
  DynamicArrayHandleTryStorage(const DynamicArrayHandle &array,
                               const Functor &f)
    : Array(array), Function(f), FoundCast(false) {  }

  template<typename Storage>
  VTKM_CONT_EXPORT
  void operator()(Storage) {
    this->DoCast(Storage(),
                 typename vtkm::cont::internal::IsValidArrayHandle<Type,Storage>::type());
  }

private:
  template<typename Storage>
  void DoCast(Storage, boost::mpl::bool_<true>)
  {
    if (!this->FoundCast &&
        this->Array.IsTypeAndStorage(Type(), Storage()))
    {
      this->Function(this->Array.CastToArrayHandle(Type(), Storage()));
      this->FoundCast = true;
    }
  }

  template<typename Storage>
  void DoCast(Storage, boost::mpl::bool_<false>)
  {
    // This type of array handle cannot exist, so do nothing.
  }
};

template<typename Functor, typename StorageList>
struct DynamicArrayHandleTryType {
  const DynamicArrayHandle Array;
  const Functor &Function;
  bool FoundCast;

  VTKM_CONT_EXPORT
  DynamicArrayHandleTryType(const DynamicArrayHandle &array, const Functor &f)
    : Array(array), Function(f), FoundCast(false) {  }

  template<typename Type>
  VTKM_CONT_EXPORT
  void operator()(Type) {
    if (this->FoundCast) { return; }
    typedef DynamicArrayHandleTryStorage<Functor, Type> TryStorageType;
    TryStorageType tryStorage =
        TryStorageType(this->Array, this->Function);
    vtkm::ListForEach(tryStorage, StorageList());
    if (tryStorage.FoundCast)
    {
      this->FoundCast = true;
    }
  }
};

} // namespace detail

template<typename TypeList, typename StorageList>
template<typename Functor>
VTKM_CONT_EXPORT
void DynamicArrayHandleBase<TypeList,StorageList>::
    CastAndCall(const Functor &f) const
{
  VTKM_IS_LIST_TAG(TypeList);
  VTKM_IS_LIST_TAG(StorageList);
  typedef detail::DynamicArrayHandleTryType<Functor, StorageList> TryTypeType;
  // We cast this to a DynamicArrayHandle because at this point we are ignoring
  // the type/storage lists in it. There is no sense in adding more unnecessary
  // template cases.
  TryTypeType tryType = TryTypeType(DynamicArrayHandle(*this), f);
  vtkm::ListForEach(tryType, TypeList());
  if (!tryType.FoundCast)
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Could not find appropriate cast for array in CastAndCall.");
  }
}

namespace internal {

template<typename TypeList, typename StorageList>
struct DynamicTransformTraits<
    vtkm::cont::DynamicArrayHandleBase<TypeList,StorageList> >
{
  typedef vtkm::cont::internal::DynamicTransformTagCastAndCall DynamicTag;
};

} // namespace internal

}
} // namespace vtkm::cont

#endif //vtk_m_cont_DynamicArrayHandle_h
