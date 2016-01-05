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
#ifndef vtk_m_ArrayHandleCompositeVector_h
#define vtk_m_ArrayHandleCompositeVector_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorControlBadValue.h>
#include <vtkm/cont/ErrorControlInternal.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/VecTraits.h>

#include <vtkm/internal/FunctionInterface.h>

#include <sstream>

namespace vtkm {
namespace cont {

namespace internal {

namespace detail {

template<typename ValueType>
struct CompositeVectorSwizzleFunctor
{
  static const vtkm::IdComponent NUM_COMPONENTS =
      vtkm::VecTraits<ValueType>::NUM_COMPONENTS;
  typedef vtkm::Vec<vtkm::IdComponent, NUM_COMPONENTS> ComponentMapType;

  // Caution! This is a reference.
  const ComponentMapType &SourceComponents;

  VTKM_EXEC_CONT_EXPORT
  CompositeVectorSwizzleFunctor(const ComponentMapType &sourceComponents)
    : SourceComponents(sourceComponents) {  }

  // Currently only supporting 1-4 components.
  template<typename T1>
  VTKM_EXEC_CONT_EXPORT
  ValueType operator()(const T1 &p1) const {
    return ValueType(
          vtkm::VecTraits<T1>::GetComponent(p1, this->SourceComponents[0]));
  }

  template<typename T1, typename T2>
  VTKM_EXEC_CONT_EXPORT
  ValueType operator()(const T1 &p1, const T2 &p2) const {
    return ValueType(
          vtkm::VecTraits<T1>::GetComponent(p1, this->SourceComponents[0]),
          vtkm::VecTraits<T2>::GetComponent(p2, this->SourceComponents[1]));
  }

  template<typename T1, typename T2, typename T3>
  VTKM_EXEC_CONT_EXPORT
  ValueType operator()(const T1 &p1, const T2 &p2, const T3 &p3) const {
    return ValueType(
          vtkm::VecTraits<T1>::GetComponent(p1, this->SourceComponents[0]),
          vtkm::VecTraits<T2>::GetComponent(p2, this->SourceComponents[1]),
          vtkm::VecTraits<T3>::GetComponent(p3, this->SourceComponents[2]));
  }

  template<typename T1, typename T2, typename T3, typename T4>
  VTKM_EXEC_CONT_EXPORT
  ValueType operator()(const T1 &p1,
                       const T2 &p2,
                       const T3 &p3,
                       const T4 &p4) const {
    return ValueType(
          vtkm::VecTraits<T1>::GetComponent(p1, this->SourceComponents[0]),
          vtkm::VecTraits<T2>::GetComponent(p2, this->SourceComponents[1]),
          vtkm::VecTraits<T3>::GetComponent(p3, this->SourceComponents[2]),
          vtkm::VecTraits<T4>::GetComponent(p4, this->SourceComponents[3]));
  }
};

template<typename ReturnValueType>
struct CompositeVectorPullValueFunctor
{
  vtkm::Id Index;

  VTKM_EXEC_EXPORT
  CompositeVectorPullValueFunctor(vtkm::Id index) : Index(index) {  }

  // This form is to pull values out of array arguments.
  VTKM_SUPPRESS_EXEC_WARNINGS
  template<typename PortalType>
  VTKM_EXEC_CONT_EXPORT
  typename PortalType::ValueType operator()(const PortalType &portal) const {
    return portal.Get(this->Index);
  }

  // This form is an identity to pass the return value back.
  VTKM_EXEC_CONT_EXPORT
  const ReturnValueType &operator()(const ReturnValueType &value) const {
    return value;
  }
};

struct CompositeVectorArrayToPortalCont {
  template<typename ArrayHandleType, vtkm::IdComponent Index>
  struct ReturnType {
    typedef typename ArrayHandleType::PortalConstControl type;
  };

  template<typename ArrayHandleType, vtkm::IdComponent Index>
  VTKM_CONT_EXPORT
  typename ReturnType<ArrayHandleType, Index>::type
  operator()(const ArrayHandleType &array,
             vtkm::internal::IndexTag<Index>) const {
    return array.GetPortalConstControl();
  }
};

template<typename DeviceAdapterTag>
struct CompositeVectorArrayToPortalExec {
  template<typename ArrayHandleType, vtkm::IdComponent Index>
  struct ReturnType {
    typedef typename ArrayHandleType::template ExecutionTypes<
          DeviceAdapterTag>::PortalConst type;
  };

  template<typename ArrayHandleType, vtkm::IdComponent Index>
  VTKM_CONT_EXPORT
  typename ReturnType<ArrayHandleType, Index>::type
  operator()(const ArrayHandleType &array,
             vtkm::internal::IndexTag<Index>) const {
    return array.PrepareForInput(DeviceAdapterTag());
  }
};

struct CheckArraySizeFunctor {
  vtkm::Id ExpectedSize;
  CheckArraySizeFunctor(vtkm::Id expectedSize) : ExpectedSize(expectedSize) {  }

  template<typename T, vtkm::IdComponent Index>
  void operator()(const T &a, vtkm::internal::IndexTag<Index>) const {
    if (a.GetNumberOfValues() != this->ExpectedSize)
    {
      std::stringstream message;
      message << "All input arrays to ArrayHandleCompositeVector must be the same size.\n"
              << "Array " << Index-1 << " has " << a.GetNumberOfValues()
              << ". Expected " << this->ExpectedSize << ".";
      throw vtkm::cont::ErrorControlBadValue(message.str().c_str());
    }
  }
};

} // namespace detail

/// \brief A portal that gets values from components of other portals.
///
/// This is the portal used within ArrayHandleCompositeVector.
///
template<typename SignatureWithPortals>
class ArrayPortalCompositeVector
{
  typedef vtkm::internal::FunctionInterface<SignatureWithPortals> PortalTypes;

public:
  typedef typename PortalTypes::ResultType ValueType;
  static const vtkm::IdComponent NUM_COMPONENTS =
      vtkm::VecTraits<ValueType>::NUM_COMPONENTS;

  // Used internally.
  typedef vtkm::Vec<vtkm::IdComponent, NUM_COMPONENTS> ComponentMapType;

  VTKM_STATIC_ASSERT(NUM_COMPONENTS == PortalTypes::ARITY);

  VTKM_EXEC_CONT_EXPORT
  ArrayPortalCompositeVector() {  }

  VTKM_CONT_EXPORT
  ArrayPortalCompositeVector(
      const PortalTypes portals,
      vtkm::Vec<vtkm::IdComponent, NUM_COMPONENTS> sourceComponents)
    : Portals(portals), SourceComponents(sourceComponents) {  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    return this->Portals.template GetParameter<1>().GetNumberOfValues();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT_EXPORT
  ValueType Get(vtkm::Id index) const {
    // This might be inefficient because we are copying all the portals only
    // because they are coupled with the return value.
    PortalTypes localPortals = this->Portals;
    localPortals.InvokeExec(
          detail::CompositeVectorSwizzleFunctor<ValueType>(this->SourceComponents),
          detail::CompositeVectorPullValueFunctor<ValueType>(index));
    return localPortals.GetReturnValue();
  }

private:
  PortalTypes Portals;
  ComponentMapType SourceComponents;
};

template<typename SignatureWithArrays>
struct StorageTagCompositeVector {  };

/// A convenience class that provides a typedef to the appropriate tag for
/// a composite storage.
template<typename SignatureWithArrays>
struct ArrayHandleCompositeVectorTraits
{
  typedef vtkm::cont::internal::StorageTagCompositeVector<SignatureWithArrays>
          Tag;
  typedef typename vtkm::internal::FunctionInterface<SignatureWithArrays>::ResultType
          ValueType;
  typedef vtkm::cont::internal::Storage<ValueType, Tag> StorageType;
  typedef vtkm::cont::ArrayHandle<ValueType, Tag> Superclass;
};

// It may seem weird that this specialization throws an exception for
// everything, but that is because all the functionality is handled in the
// ArrayTransfer class.
template<typename SignatureWithArrays>
class Storage<
    typename ArrayHandleCompositeVectorTraits<SignatureWithArrays>::ValueType,
    vtkm::cont::internal::StorageTagCompositeVector<SignatureWithArrays> >
{
  typedef vtkm::internal::FunctionInterface<SignatureWithArrays>
      FunctionInterfaceWithArrays;
  static const vtkm::IdComponent NUM_COMPONENTS = FunctionInterfaceWithArrays::ARITY;
  typedef vtkm::Vec<vtkm::IdComponent, NUM_COMPONENTS> ComponentMapType;

  typedef typename FunctionInterfaceWithArrays::template StaticTransformType<
        detail::CompositeVectorArrayToPortalCont>::type
      FunctionInterfaceWithPortals;
  typedef typename FunctionInterfaceWithPortals::Signature SignatureWithPortals;

public:
  typedef ArrayPortalCompositeVector<SignatureWithPortals> PortalType;
  typedef PortalType PortalConstType;
  typedef typename PortalType::ValueType ValueType;

  VTKM_CONT_EXPORT
  Storage() : Valid(false) {  }

  VTKM_CONT_EXPORT
  Storage(const FunctionInterfaceWithArrays &arrays,
          const ComponentMapType &sourceComponents)
    : Arrays(arrays), SourceComponents(sourceComponents), Valid(true)
  {
    arrays.ForEachCont(
          detail::CheckArraySizeFunctor(this->GetNumberOfValues()));
  }

  VTKM_CONT_EXPORT
  PortalType GetPortal() {
    throw vtkm::cont::ErrorControlBadValue(
          "Composite vector arrays are read only.");
  }

  VTKM_CONT_EXPORT
  PortalConstType GetPortalConst() const {
    if (!this->Valid)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Tried to use an ArrayHandleCompositeHandle without dependent arrays.");
    }
    return PortalConstType(this->Arrays.StaticTransformCont(
                             detail::CompositeVectorArrayToPortalCont()),
                           this->SourceComponents);
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    if (!this->Valid)
    {
      throw vtkm::cont::ErrorControlBadValue(
            "Tried to use an ArrayHandleCompositeHandle without dependent arrays.");
    }
    return this->Arrays.template GetParameter<1>().GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlInternal(

          "The allocate method for the composite vector storage should never "
          "have been called. The allocate is generally only called by the "
          "execution array manager, and the array transfer for the composite "
          "storage should prevent the execution array manager from being "
          "directly used.");
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues)) {
    throw vtkm::cont::ErrorControlBadValue(
          "Composite vector arrays are read-only.");
  }

  VTKM_CONT_EXPORT
  void ReleaseResources() {
    if (this->Valid)
    {
      // TODO: Implement this.
    }
  }

  VTKM_CONT_EXPORT
  const FunctionInterfaceWithArrays &GetArrays() const {
    VTKM_ASSERT_CONT(this->Valid);
    return this->Arrays;
  }

  VTKM_CONT_EXPORT
  const ComponentMapType &GetSourceComponents() const {
    VTKM_ASSERT_CONT(this->Valid);
    return this->SourceComponents;
  }

private:
  FunctionInterfaceWithArrays Arrays;
  ComponentMapType SourceComponents;
  bool Valid;
};

template<typename SignatureWithArrays, typename DeviceAdapterTag>
class ArrayTransfer<
    typename ArrayHandleCompositeVectorTraits<SignatureWithArrays>::ValueType,
    vtkm::cont::internal::StorageTagCompositeVector<SignatureWithArrays>,
    DeviceAdapterTag>
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  typedef typename ArrayHandleCompositeVectorTraits<SignatureWithArrays>::StorageType
      StorageType;

  typedef vtkm::internal::FunctionInterface<SignatureWithArrays>
      FunctionWithArrays;
  typedef typename FunctionWithArrays::template StaticTransformType<
        detail::CompositeVectorArrayToPortalExec<DeviceAdapterTag> >::type
      FunctionWithPortals;
  typedef typename FunctionWithPortals::Signature SignatureWithPortals;

public:
  typedef typename ArrayHandleCompositeVectorTraits<SignatureWithArrays>::ValueType
      ValueType;

  // These are not currently fully implemented.
  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  typedef ArrayPortalCompositeVector<SignatureWithPortals> PortalExecution;
  typedef ArrayPortalCompositeVector<SignatureWithPortals> PortalConstExecution;

  VTKM_CONT_EXPORT
  ArrayTransfer(StorageType *storage) : Storage(storage) {  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    return this->Storage->GetNumberOfValues();
  }

  VTKM_CONT_EXPORT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData)) const
  {
    return
        PortalConstExecution(
          this->Storage->GetArrays().StaticTransformCont(
            detail::CompositeVectorArrayToPortalExec<DeviceAdapterTag>()),
          this->Storage->GetSourceComponents());
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    // It may be the case a composite vector could be used for in place
    // operations, but this is not implemented currently.
    throw vtkm::cont::ErrorControlBadValue(
          "Composite vector arrays cannot be used for output or in place.");
  }

  VTKM_CONT_EXPORT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    // It may be the case a composite vector could be used for output if you
    // want the delegate arrays to be resized, but this is not implemented
    // currently.
    throw vtkm::cont::ErrorControlBadValue(
          "Composite vector arrays cannot be used for output.");
  }

  VTKM_CONT_EXPORT
  void RetrieveOutputData(StorageType *vtkmNotUsed(storage)) const
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Composite vector arrays cannot be used for output.");
  }

  VTKM_CONT_EXPORT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorControlBadValue(
          "Composite vector arrays cannot be resized.");
  }

  VTKM_CONT_EXPORT
  void ReleaseResources() {
    this->Storage->ReleaseResources();
  }

private:
  StorageType *Storage;
};

} // namespace internal

/// \brief An \c ArrayHandle that combines components from other arrays.
///
/// \c ArrayHandleCompositeVector is a specialization of \c ArrayHandle that
/// derives its content from other arrays. It takes up to 4 other \c
/// ArrayHandle objects and mimics an array that contains vectors with
/// components that come from these delegate arrays.
///
/// The easiest way to create and type an \c ArrayHandleCompositeVector is
/// to use the \c make_ArrayHandleCompositeVector functions.
///
template<typename Signature>
class ArrayHandleCompositeVector
    : public internal::ArrayHandleCompositeVectorTraits<Signature>::Superclass
{
  typedef typename internal::ArrayHandleCompositeVectorTraits<Signature>::StorageType
      StorageType;
  typedef typename internal::ArrayPortalCompositeVector<Signature>::ComponentMapType
      ComponentMapType;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
      ArrayHandleCompositeVector,
      (ArrayHandleCompositeVector<Signature>),
      (typename internal::ArrayHandleCompositeVectorTraits<Signature>::Superclass));

  VTKM_CONT_EXPORT
  ArrayHandleCompositeVector(
      const vtkm::internal::FunctionInterface<Signature> &arrays,
      const ComponentMapType &sourceComponents)
    : Superclass(StorageType(arrays, sourceComponents))
  {  }

  /// Template constructors for passing in types. You'll get weird compile
  /// errors if the argument types do not actually match the types in the
  /// signature.
  ///
  template<typename ArrayHandleType1>
  VTKM_CONT_EXPORT
  ArrayHandleCompositeVector(const ArrayHandleType1 &array1,
                             vtkm::IdComponent sourceComponent1)
    : Superclass(StorageType(
                   vtkm::internal::make_FunctionInterface<ValueType>(array1),
                   ComponentMapType(sourceComponent1)))
  {  }
  template<typename ArrayHandleType1,
           typename ArrayHandleType2>
  VTKM_CONT_EXPORT
  ArrayHandleCompositeVector(const ArrayHandleType1 &array1,
                             vtkm::IdComponent sourceComponent1,
                             const ArrayHandleType2 &array2,
                             vtkm::IdComponent sourceComponent2)
    : Superclass(StorageType(
                   vtkm::internal::make_FunctionInterface<ValueType>(
                     array1, array2),
                   ComponentMapType(sourceComponent1,
                                    sourceComponent2)))
  {  }
  template<typename ArrayHandleType1,
           typename ArrayHandleType2,
           typename ArrayHandleType3>
  VTKM_CONT_EXPORT
  ArrayHandleCompositeVector(const ArrayHandleType1 &array1,
                             vtkm::IdComponent sourceComponent1,
                             const ArrayHandleType2 &array2,
                             vtkm::IdComponent sourceComponent2,
                             const ArrayHandleType3 &array3,
                             vtkm::IdComponent sourceComponent3)
    : Superclass(StorageType(
                   vtkm::internal::make_FunctionInterface<ValueType>(
                     array1, array2, array3),
                   ComponentMapType(sourceComponent1,
                                    sourceComponent2,
                                    sourceComponent3)))
  {  }
  template<typename ArrayHandleType1,
           typename ArrayHandleType2,
           typename ArrayHandleType3,
           typename ArrayHandleType4>
  VTKM_CONT_EXPORT
  ArrayHandleCompositeVector(const ArrayHandleType1 &array1,
                             vtkm::IdComponent sourceComponent1,
                             const ArrayHandleType2 &array2,
                             vtkm::IdComponent sourceComponent2,
                             const ArrayHandleType3 &array3,
                             vtkm::IdComponent sourceComponent3,
                             const ArrayHandleType4 &array4,
                             vtkm::IdComponent sourceComponent4)
    : Superclass(StorageType(
                   vtkm::internal::make_FunctionInterface<ValueType>(
                     array1, array2, array3, array4),
                   ComponentMapType(sourceComponent1,
                                    sourceComponent2,
                                    sourceComponent3,
                                    sourceComponent4)))
  {  }
};

/// \brief Get the type for an ArrayHandleCompositeVector
///
/// The ArrayHandleCompositeVector has a difficult template specification.
/// Use this helper template to covert a list of array handle types to a
/// composite vector of these array handles. Here is a simple example.
///
/// \code{.cpp}
/// typedef vtkm::cont::ArrayHandleCompositeVector<
///     vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
///     vtkm::cont::ArrayHandle<vtkm::FloatDefault> >::type OutArrayType;
/// OutArrayType outArray = vtkm::cont::make_ArrayHandleCompositeVector(a1,a2);
/// \endcode
///
template<typename ArrayHandleType1,
         typename ArrayHandleType2 = void,
         typename ArrayHandleType3 = void,
         typename ArrayHandleType4 = void>
struct ArrayHandleCompositeVectorType
{
private:
  typedef typename vtkm::VecTraits<typename ArrayHandleType1::ValueType>::ComponentType
      ComponentType;
  typedef vtkm::Vec<ComponentType,4> Signature(
      ArrayHandleType1,ArrayHandleType2,ArrayHandleType3,ArrayHandleType4);
public:
  typedef vtkm::cont::ArrayHandleCompositeVector<Signature> type;
};

template<typename ArrayHandleType1,
         typename ArrayHandleType2,
         typename ArrayHandleType3>
struct ArrayHandleCompositeVectorType<
    ArrayHandleType1,ArrayHandleType2,ArrayHandleType3>
{
private:
  typedef typename vtkm::VecTraits<typename ArrayHandleType1::ValueType>::ComponentType
      ComponentType;
  typedef vtkm::Vec<ComponentType,3> Signature(
      ArrayHandleType1,ArrayHandleType2,ArrayHandleType3);
public:
  typedef vtkm::cont::ArrayHandleCompositeVector<Signature> type;
};

template<typename ArrayHandleType1,
         typename ArrayHandleType2>
struct ArrayHandleCompositeVectorType<ArrayHandleType1,ArrayHandleType2>
{
private:
  typedef typename vtkm::VecTraits<typename ArrayHandleType1::ValueType>::ComponentType
      ComponentType;
  typedef vtkm::Vec<ComponentType,2> Signature(
      ArrayHandleType1,ArrayHandleType2);
public:
  typedef vtkm::cont::ArrayHandleCompositeVector<Signature> type;
};

template<typename ArrayHandleType1>
struct ArrayHandleCompositeVectorType<ArrayHandleType1>
{
private:
  typedef typename vtkm::VecTraits<typename ArrayHandleType1::ValueType>::ComponentType
      ComponentType;
  typedef ComponentType Signature(ArrayHandleType1);
public:
  typedef vtkm::cont::ArrayHandleCompositeVector<Signature> type;
};

/// Create a composite vector array from other arrays.
///
template<typename ValueType1, typename Storage1>
VTKM_CONT_EXPORT
typename ArrayHandleCompositeVectorType<
  vtkm::cont::ArrayHandle<ValueType1,Storage1> >::type
make_ArrayHandleCompositeVector(
    const vtkm::cont::ArrayHandle<ValueType1,Storage1> &array1,
    vtkm::IdComponent sourceComponent1)
{
  return typename ArrayHandleCompositeVectorType<
      vtkm::cont::ArrayHandle<ValueType1,Storage1> >::type(array1,
                                                           sourceComponent1);
}
template<typename ValueType1, typename Storage1,
         typename ValueType2, typename Storage2>
VTKM_CONT_EXPORT
typename ArrayHandleCompositeVectorType<
  vtkm::cont::ArrayHandle<ValueType1,Storage1>,
  vtkm::cont::ArrayHandle<ValueType2,Storage2> >::type
make_ArrayHandleCompositeVector(
    const vtkm::cont::ArrayHandle<ValueType1,Storage1> &array1,
    vtkm::IdComponent sourceComponent1,
    const vtkm::cont::ArrayHandle<ValueType2,Storage2> &array2,
    vtkm::IdComponent sourceComponent2)
{
  return typename ArrayHandleCompositeVectorType<
      vtkm::cont::ArrayHandle<ValueType1,Storage1>,
      vtkm::cont::ArrayHandle<ValueType2,Storage2> >::type(array1,
                                                           sourceComponent1,
                                                           array2,
                                                           sourceComponent2);
}
template<typename ValueType1, typename Storage1,
         typename ValueType2, typename Storage2,
         typename ValueType3, typename Storage3>
VTKM_CONT_EXPORT
typename ArrayHandleCompositeVectorType<
  vtkm::cont::ArrayHandle<ValueType1,Storage1>,
  vtkm::cont::ArrayHandle<ValueType2,Storage2>,
  vtkm::cont::ArrayHandle<ValueType3,Storage3> >::type
make_ArrayHandleCompositeVector(
    const vtkm::cont::ArrayHandle<ValueType1,Storage1> &array1,
    vtkm::IdComponent sourceComponent1,
    const vtkm::cont::ArrayHandle<ValueType2,Storage2> &array2,
    vtkm::IdComponent sourceComponent2,
    const vtkm::cont::ArrayHandle<ValueType3,Storage3> &array3,
    vtkm::IdComponent sourceComponent3)
{
  return typename ArrayHandleCompositeVectorType<
      vtkm::cont::ArrayHandle<ValueType1,Storage1>,
      vtkm::cont::ArrayHandle<ValueType2,Storage2>,
      vtkm::cont::ArrayHandle<ValueType3,Storage3> >::type(array1,
                                                           sourceComponent1,
                                                           array2,
                                                           sourceComponent2,
                                                           array3,
                                                           sourceComponent3);
}
template<typename ValueType1, typename Storage1,
         typename ValueType2, typename Storage2,
         typename ValueType3, typename Storage3,
         typename ValueType4, typename Storage4>
VTKM_CONT_EXPORT
typename ArrayHandleCompositeVectorType<
  vtkm::cont::ArrayHandle<ValueType1,Storage1>,
  vtkm::cont::ArrayHandle<ValueType2,Storage2>,
  vtkm::cont::ArrayHandle<ValueType3,Storage3>,
  vtkm::cont::ArrayHandle<ValueType4,Storage4> >::type
make_ArrayHandleCompositeVector(
    const vtkm::cont::ArrayHandle<ValueType1,Storage1> &array1,
    vtkm::IdComponent sourceComponent1,
    const vtkm::cont::ArrayHandle<ValueType2,Storage2> &array2,
    vtkm::IdComponent sourceComponent2,
    const vtkm::cont::ArrayHandle<ValueType3,Storage3> &array3,
    vtkm::IdComponent sourceComponent3,
    const vtkm::cont::ArrayHandle<ValueType4,Storage4> &array4,
    vtkm::IdComponent sourceComponent4)
{
  return typename ArrayHandleCompositeVectorType<
      vtkm::cont::ArrayHandle<ValueType1,Storage1>,
      vtkm::cont::ArrayHandle<ValueType2,Storage2>,
      vtkm::cont::ArrayHandle<ValueType3,Storage3>,
      vtkm::cont::ArrayHandle<ValueType4,Storage4> >::type(array1,
                                                           sourceComponent1,
                                                           array2,
                                                           sourceComponent2,
                                                           array3,
                                                           sourceComponent3,
                                                           array4,
                                                           sourceComponent4);
}

}
} // namespace vtkm::cont

#endif //vtk_m_ArrayHandleCompositeVector_h
