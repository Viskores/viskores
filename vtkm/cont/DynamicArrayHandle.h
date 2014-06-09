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
//  Copyright 2014. Los Alamos National Security
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

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ContainerListTag.h>
#include <vtkm/cont/ErrorControlBadValue.h>

#include <vtkm/cont/internal/DynamicTransform.h>
#include <vtkm/cont/internal/SimplePolymorphicContainer.h>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/utility/enable_if.hpp>

namespace vtkm {
namespace cont {

namespace internal {

/// Behaves like (and is interchangable with) a DynamicArrayHandle. The
/// difference is that the list of types and list of containers to try when
/// calling CastAndCall is set to the class template arguments.
///
template<typename TypeList, typename ContainerList>
class DynamicArrayHandleCast;

} // namespace internal

/// \brief Holds an array handle without having to specify template parameters.
///
/// \c DynamicArrayHandle holds an \c ArrayHandle object using runtime
/// polymorphism to manage different value types and storage rather than
/// compile-time templates. This adds a programming convienience that helps
/// avoid a proliferation of templates. It also provides the management
/// necessary to interface VTK-m with data sources where types will not be
/// known until runtime.
///
/// To interface between the runtime polymorphism and the templated algorithms
/// in VTK-m, \c DynamicArrayHandle contains a method named \c CastAndCall that
/// will determine the correct type from some known list of types and
/// containers. This mechanism is used internally by VTK-m's worklet invocation
/// mechanism to determine the type when running algorithms.
///
/// By default, \c DynamicArrayHandle will assume that the value type in the
/// array matches one of the types specified by \c VTKM_DEFAULT_TYPE_LIST_TAG
/// and the container matches one of the tags specified by \c
/// VTKM_DEFAULT_CONTAINER_LIST_TAG. These lists can be changed by using the \c
/// ResetTypeList and \c ResetContainerList methods, respectively. It is
/// worthwhile to match these lists closely to the possible types that might be
/// used. If a type is missing you will get a runtime error. If there are more
/// types than necessary, then the template mechanism will create a lot of
/// object code that is never used, and keep in mind that the number of
/// combinations grows exponentally when using multiple \c DynamicArrayHandle
/// objects.
///
class DynamicArrayHandle
{
public:
  VTKM_CONT_EXPORT
  DynamicArrayHandle() {  }

  template<typename Type, typename Container>
  VTKM_CONT_EXPORT
  DynamicArrayHandle(const vtkm::cont::ArrayHandle<Type,Container> &array)
    : ArrayContainer(new vtkm::cont::internal::SimplePolymorphicContainer<
                     vtkm::cont::ArrayHandle<Type,Container> >(array))
  {  }

  template<typename TypeList, typename ContainerList>
  VTKM_CONT_EXPORT
  DynamicArrayHandle(
      const internal::DynamicArrayHandleCast<TypeList,ContainerList> &dynamicArray)
    : ArrayContainer(dynamicArray.ArrayContainer) {  }

  /// Returns true if this array is of the provided type and uses the provided
  /// container.
  ///
  template<typename Type, typename Container>
  VTKM_CONT_EXPORT
  bool IsTypeAndContainer(Type = Type(), Container = Container()) const {
    return (this->TryCastArrayContainer<Type,Container>() != NULL);
  }

  /// Returns this array cast to an ArrayHandle object of the given type and
  /// container. Throws ErrorControlBadValue if the cast does not work. Use
  /// IsTypeAndContainer to check if the cast can happen.
  ///
  template<typename Type, typename Container>
  VTKM_CONT_EXPORT
  vtkm::cont::ArrayHandle<Type, Container>
  CastToArrayHandle(Type = Type(), Container = Container()) const {
    vtkm::cont::internal::SimplePolymorphicContainer<
      vtkm::cont::ArrayHandle<Type,Container> > *container =
        this->TryCastArrayContainer<Type,Container>();
    if (container == NULL)
    {
      throw vtkm::cont::ErrorControlBadValue("Bad cast of dynamic array.");
    }
    return container->Item;
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
  internal::DynamicArrayHandleCast<NewTypeList,VTKM_DEFAULT_CONTAINER_LIST_TAG>
  ResetTypeList(NewTypeList = NewTypeList()) const {
    return internal::DynamicArrayHandleCast<
        NewTypeList,VTKM_DEFAULT_CONTAINER_LIST_TAG>(*this);
  }

  /// Changes the array containers to try casting to when resolving this
  /// dynamic array, which is specified with a list tag like those in
  /// ContainerListTag.h. Since C++ does not allow you to actually change the
  /// template arguments, this method returns a new dynamic array object. This
  /// method is particularly useful to narrow down (or expand) the types when
  /// using an array of particular constraints.
  ///
  template<typename NewContainerList>
  VTKM_CONT_EXPORT
  internal::DynamicArrayHandleCast<VTKM_DEFAULT_TYPE_LIST_TAG,NewContainerList>
  ResetContainerList(NewContainerList = NewContainerList()) const {
    return internal::DynamicArrayHandleCast<
        VTKM_DEFAULT_TYPE_LIST_TAG,NewContainerList>(*this);
  }

  /// Attempts to cast the held array to a specific value type and container,
  /// then call the given functor with the cast array. The types and containers
  /// tried in the cast are those in the lists defined by
  /// VTKM_DEFAULT_TYPE_LIST_TAG and VTKM_DEFAULT_CONTAINER_LIST_TAG,
  /// respectively, unless they have been changed with a previous call to
  /// ResetTypeList or ResetContainerList.
  ///
  template<typename Functor>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f) const
  {
    this->CastAndCall(
          f, VTKM_DEFAULT_TYPE_LIST_TAG(), VTKM_DEFAULT_CONTAINER_LIST_TAG());
  }

  /// A version of CastAndCall that tries specified lists of types and
  /// containers.
  ///
  template<typename Functor, typename TypeList, typename ContainerList>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f, TypeList, ContainerList) const;

private:
  boost::shared_ptr<vtkm::cont::internal::SimplePolymorphicContainerBase>
    ArrayContainer;

  template<typename Type, typename Container>
  VTKM_CONT_EXPORT
  vtkm::cont::internal::SimplePolymorphicContainer<
    vtkm::cont::ArrayHandle<Type,Container> > *
  TryCastArrayContainer() const {
    return
        dynamic_cast<
          vtkm::cont::internal::SimplePolymorphicContainer<
            vtkm::cont::ArrayHandle<Type,Container> > *>(
          this->ArrayContainer.get());
  }
};

namespace detail {

template<typename Functor, typename Type>
struct DynamicArrayHandleTryContainer {
  const DynamicArrayHandle Array;
  const Functor &Function;
  bool FoundCast;

  VTKM_CONT_EXPORT
  DynamicArrayHandleTryContainer(const DynamicArrayHandle &array,
                                 const Functor &f)
    : Array(array), Function(f), FoundCast(false) {  }

  template<typename Container>
  VTKM_CONT_EXPORT
  typename boost::enable_if<
    typename vtkm::cont::internal::IsValidArrayHandle<Type,Container>::type
    >::type
  operator()(Container) {
    if (!this->FoundCast &&
        this->Array.IsTypeAndContainer(Type(), Container()))
    {
      this->Function(this->Array.CastToArrayHandle(Type(), Container()));
      this->FoundCast = true;
    }
  }

  template<typename Container>
  VTKM_CONT_EXPORT
  typename boost::disable_if<
    typename vtkm::cont::internal::IsValidArrayHandle<Type,Container>::type
    >::type
  operator()(Container) {
    // This type of array handle cannot exist, so do nothing.
  }
};

template<typename Functor, typename ContainerList>
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
    typedef DynamicArrayHandleTryContainer<Functor, Type> TryContainerType;
    TryContainerType tryContainer =
        TryContainerType(this->Array, this->Function);
    vtkm::ListForEach(tryContainer, ContainerList());
    if (tryContainer.FoundCast)
    {
      this->FoundCast = true;
    }
  }
};

} // namespace detail

template<typename Functor, typename TypeList, typename ContainerList>
VTKM_CONT_EXPORT
void DynamicArrayHandle::CastAndCall(const Functor &f,
                                     TypeList,
                                     ContainerList) const
{
  typedef detail::DynamicArrayHandleTryType<Functor, ContainerList> TryTypeType;
  TryTypeType tryType = TryTypeType(*this, f);
  vtkm::ListForEach(tryType, TypeList());
  if (!tryType.FoundCast)
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Could not find appropriate cast for array in CastAndCall.");
  }
}

namespace internal {

template<typename TypeList, typename ContainerList>
class DynamicArrayHandleCast : public vtkm::cont::DynamicArrayHandle
{
public:
  VTKM_CONT_EXPORT
  DynamicArrayHandleCast() : DynamicArrayHandle() {  }

  VTKM_CONT_EXPORT
  DynamicArrayHandleCast(const vtkm::cont::DynamicArrayHandle &array)
    : DynamicArrayHandle(array) {  }

  template<typename SrcTypeList, typename SrcContainerList>
  VTKM_CONT_EXPORT
  DynamicArrayHandleCast(
      const DynamicArrayHandleCast<SrcTypeList,SrcContainerList> &array)
    : DynamicArrayHandle(array) {  }

  template<typename NewTypeList>
  VTKM_CONT_EXPORT
  DynamicArrayHandleCast<NewTypeList,ContainerList>
  ResetTypeList(NewTypeList = NewTypeList()) const {
    return DynamicArrayHandleCast<NewTypeList,ContainerList>(*this);
  }

  template<typename NewContainerList>
  VTKM_CONT_EXPORT
  internal::DynamicArrayHandleCast<TypeList,NewContainerList>
  ResetContainerList(NewContainerList = NewContainerList()) const {
    return internal::DynamicArrayHandleCast<TypeList,NewContainerList>(*this);
  }

  template<typename Functor>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f) const
  {
    this->CastAndCall(f, TypeList(), ContainerList());
  }

  template<typename Functor, typename TL, typename CL>
  VTKM_CONT_EXPORT
  void CastAndCall(const Functor &f, TL, CL) const
  {
    this->DynamicArrayHandle::CastAndCall(f, TL(), CL());
  }
};

template<>
struct DynamicTransformTraits<vtkm::cont::DynamicArrayHandle> {
  typedef vtkm::cont::internal::DynamicTransformTagCastAndCall DynamicTag;
};

template<typename TypeList, typename ContainerList>
struct DynamicTransformTraits<
    vtkm::cont::internal::DynamicArrayHandleCast<TypeList,ContainerList> >
{
  typedef vtkm::cont::internal::DynamicTransformTagCastAndCall DynamicTag;
};

} // namespace internal

}
} // namespace vtkm::cont

#endif //vtk_m_cont_DynamicArrayHandle_h
