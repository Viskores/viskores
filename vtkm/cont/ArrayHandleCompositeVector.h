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
#ifndef vtk_m_ArrayHandleCompositeVector_h
#define vtk_m_ArrayHandleCompositeVector_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/ErrorInternal.h>

#include <vtkm/StaticAssert.h>
#include <vtkm/VecTraits.h>

#include <vtkm/internal/FunctionInterface.h>

#include <sstream>

namespace vtkm
{
namespace cont
{

namespace internal
{

namespace detail
{

template <typename ValueType>
struct VTKM_ALWAYS_EXPORT CompositeVectorSwizzleFunctor
{
  static const vtkm::IdComponent NUM_COMPONENTS = vtkm::VecTraits<ValueType>::NUM_COMPONENTS;
  using ComponentMapType = vtkm::Vec<vtkm::IdComponent, NUM_COMPONENTS>;

  // Caution! This is a reference.
  const ComponentMapType& SourceComponents;

  VTKM_EXEC_CONT
  CompositeVectorSwizzleFunctor(const ComponentMapType& sourceComponents)
    : SourceComponents(sourceComponents)
  {
  }

  // Currently only supporting 1-4 components.
  template <typename T1>
  VTKM_EXEC_CONT ValueType operator()(const T1& p1) const
  {
    return ValueType(vtkm::VecTraits<T1>::GetComponent(p1, this->SourceComponents[0]));
  }

  template <typename T1, typename T2>
  VTKM_EXEC_CONT ValueType operator()(const T1& p1, const T2& p2) const
  {
    return ValueType(vtkm::VecTraits<T1>::GetComponent(p1, this->SourceComponents[0]),
                     vtkm::VecTraits<T2>::GetComponent(p2, this->SourceComponents[1]));
  }

  template <typename T1, typename T2, typename T3>
  VTKM_EXEC_CONT ValueType operator()(const T1& p1, const T2& p2, const T3& p3) const
  {
    return ValueType(vtkm::VecTraits<T1>::GetComponent(p1, this->SourceComponents[0]),
                     vtkm::VecTraits<T2>::GetComponent(p2, this->SourceComponents[1]),
                     vtkm::VecTraits<T3>::GetComponent(p3, this->SourceComponents[2]));
  }

  template <typename T1, typename T2, typename T3, typename T4>
  VTKM_EXEC_CONT ValueType operator()(const T1& p1, const T2& p2, const T3& p3, const T4& p4) const
  {
    return ValueType(vtkm::VecTraits<T1>::GetComponent(p1, this->SourceComponents[0]),
                     vtkm::VecTraits<T2>::GetComponent(p2, this->SourceComponents[1]),
                     vtkm::VecTraits<T3>::GetComponent(p3, this->SourceComponents[2]),
                     vtkm::VecTraits<T4>::GetComponent(p4, this->SourceComponents[3]));
  }
};

template <typename ReturnValueType>
struct VTKM_ALWAYS_EXPORT CompositeVectorPullValueFunctor
{
  vtkm::Id Index;

  VTKM_EXEC
  CompositeVectorPullValueFunctor(vtkm::Id index)
    : Index(index)
  {
  }

  // This form is to pull values out of array arguments.
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename PortalType>
  VTKM_EXEC_CONT typename PortalType::ValueType operator()(const PortalType& portal) const
  {
    return portal.Get(this->Index);
  }

  // This form is an identity to pass the return value back.
  VTKM_EXEC_CONT
  const ReturnValueType& operator()(const ReturnValueType& value) const { return value; }
};

struct CompositeVectorArrayToPortalCont
{
  template <typename ArrayHandleType, vtkm::IdComponent Index>
  struct ReturnType
  {
    using type = typename ArrayHandleType::PortalConstControl;
  };

  template <typename ArrayHandleType, vtkm::IdComponent Index>
  VTKM_CONT typename ReturnType<ArrayHandleType, Index>::type operator()(
    const ArrayHandleType& array,
    vtkm::internal::IndexTag<Index>) const
  {
    return array.GetPortalConstControl();
  }
};

template <typename DeviceAdapterTag>
struct CompositeVectorArrayToPortalExec
{
  template <typename ArrayHandleType, vtkm::IdComponent Index>
  struct ReturnType
  {
    using type = typename ArrayHandleType::template ExecutionTypes<DeviceAdapterTag>::PortalConst;
  };

  template <typename ArrayHandleType, vtkm::IdComponent Index>
  VTKM_CONT typename ReturnType<ArrayHandleType, Index>::type operator()(
    const ArrayHandleType& array,
    vtkm::internal::IndexTag<Index>) const
  {
    return array.PrepareForInput(DeviceAdapterTag());
  }
};

struct CheckArraySizeFunctor
{
  vtkm::Id ExpectedSize;
  CheckArraySizeFunctor(vtkm::Id expectedSize)
    : ExpectedSize(expectedSize)
  {
  }

  template <typename T, vtkm::IdComponent Index>
  void operator()(const T& a, vtkm::internal::IndexTag<Index>) const
  {
    if (a.GetNumberOfValues() != this->ExpectedSize)
    {
      std::stringstream message;
      message << "All input arrays to ArrayHandleCompositeVector must be the same size.\n"
              << "Array " << Index - 1 << " has " << a.GetNumberOfValues() << ". Expected "
              << this->ExpectedSize << ".";
      throw vtkm::cont::ErrorBadValue(message.str().c_str());
    }
  }
};

} // namespace detail

/// \brief A portal that gets values from components of other portals.
///
/// This is the portal used within ArrayHandleCompositeVector.
///
template <typename SignatureWithPortals>
class VTKM_ALWAYS_EXPORT ArrayPortalCompositeVector
{
  using PortalTypes = vtkm::internal::FunctionInterface<SignatureWithPortals>;

public:
  using ValueType = typename PortalTypes::ResultType;
  static const vtkm::IdComponent NUM_COMPONENTS = vtkm::VecTraits<ValueType>::NUM_COMPONENTS;

  // Used internally.
  using ComponentMapType = vtkm::Vec<vtkm::IdComponent, NUM_COMPONENTS>;

  VTKM_STATIC_ASSERT(NUM_COMPONENTS == PortalTypes::ARITY);

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalCompositeVector() {}

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_CONT
  ArrayPortalCompositeVector(const PortalTypes portals,
                             vtkm::Vec<vtkm::IdComponent, NUM_COMPONENTS> sourceComponents)
    : Portals(portals)
    , SourceComponents(sourceComponents)
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return this->Portals.template GetParameter<1>().GetNumberOfValues();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    // This might be inefficient because we are copying all the portals only
    // because they are coupled with the return value.
    PortalTypes localPortals = this->Portals;
    localPortals.InvokeExec(
      detail::CompositeVectorSwizzleFunctor<ValueType>(this->SourceComponents),
      detail::CompositeVectorPullValueFunctor<ValueType>(index));
    return localPortals.GetReturnValue();
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id vtkmNotUsed(index), const ValueType& vtkmNotUsed(value)) const
  {
    // There is no technical reason why this cannot be implemented. As of this
    // writing no one has needed to write to a composite vector yet.
    VTKM_ASSERT(false &&
                "Set not yet implemented for composite vector. Do you volunteer to implement it?");
  }

private:
  PortalTypes Portals;
  ComponentMapType SourceComponents;
};

template <typename SignatureWithArrays>
struct VTKM_ALWAYS_EXPORT StorageTagCompositeVector
{
};

/// A convenience class that provides a typedef to the appropriate tag for
/// a composite storage.
template <typename SignatureWithArrays>
struct ArrayHandleCompositeVectorTraits
{
  using Tag = vtkm::cont::internal::StorageTagCompositeVector<SignatureWithArrays>;
  using ValueType = typename vtkm::internal::FunctionInterface<SignatureWithArrays>::ResultType;
  using StorageType = vtkm::cont::internal::Storage<ValueType, Tag>;
  using Superclass = vtkm::cont::ArrayHandle<ValueType, Tag>;
};

// It may seem weird that this specialization throws an exception for
// everything, but that is because all the functionality is handled in the
// ArrayTransfer class.
template <typename SignatureWithArrays>
class Storage<typename ArrayHandleCompositeVectorTraits<SignatureWithArrays>::ValueType,
              vtkm::cont::internal::StorageTagCompositeVector<SignatureWithArrays>>
{
  using FunctionInterfaceWithArrays = vtkm::internal::FunctionInterface<SignatureWithArrays>;
  static const vtkm::IdComponent NUM_COMPONENTS = FunctionInterfaceWithArrays::ARITY;
  using ComponentMapType = vtkm::Vec<vtkm::IdComponent, NUM_COMPONENTS>;

  using FunctionInterfaceWithPortals =
    typename FunctionInterfaceWithArrays::template StaticTransformType<
      detail::CompositeVectorArrayToPortalCont>::type;
  using SignatureWithPortals = typename FunctionInterfaceWithPortals::Signature;

public:
  using PortalType = ArrayPortalCompositeVector<SignatureWithPortals>;
  using PortalConstType = PortalType;
  using ValueType = typename PortalType::ValueType;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const FunctionInterfaceWithArrays& arrays, const ComponentMapType& sourceComponents)
    : Arrays(arrays)
    , SourceComponents(sourceComponents)
    , Valid(true)
  {
    arrays.ForEachCont(detail::CheckArraySizeFunctor(this->GetNumberOfValues()));
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    throw vtkm::cont::ErrorBadValue("Composite vector arrays are read only.");
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    if (!this->Valid)
    {
      throw vtkm::cont::ErrorBadValue(
        "Tried to use an ArrayHandleCompositeHandle without dependent arrays.");
    }
    return PortalConstType(
      this->Arrays.StaticTransformCont(detail::CompositeVectorArrayToPortalCont()),
      this->SourceComponents);
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    if (!this->Valid)
    {
      throw vtkm::cont::ErrorBadValue(
        "Tried to use an ArrayHandleCompositeHandle without dependent arrays.");
    }
    return this->Arrays.template GetParameter<1>().GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorInternal(

      "The allocate method for the composite vector storage should never "
      "have been called. The allocate is generally only called by the "
      "execution array manager, and the array transfer for the composite "
      "storage should prevent the execution array manager from being "
      "directly used.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadValue("Composite vector arrays are read-only.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
    if (this->Valid)
    {
      // TODO: Implement this.
    }
  }

  VTKM_CONT
  const FunctionInterfaceWithArrays& GetArrays() const
  {
    VTKM_ASSERT(this->Valid);
    return this->Arrays;
  }

  VTKM_CONT
  const ComponentMapType& GetSourceComponents() const
  {
    VTKM_ASSERT(this->Valid);
    return this->SourceComponents;
  }

private:
  FunctionInterfaceWithArrays Arrays;
  ComponentMapType SourceComponents;
  bool Valid;
};

template <typename SignatureWithArrays, typename DeviceAdapterTag>
class ArrayTransfer<typename ArrayHandleCompositeVectorTraits<SignatureWithArrays>::ValueType,
                    vtkm::cont::internal::StorageTagCompositeVector<SignatureWithArrays>,
                    DeviceAdapterTag>
{
  VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapterTag);

  using StorageType = typename ArrayHandleCompositeVectorTraits<SignatureWithArrays>::StorageType;

  using FunctionWithArrays = vtkm::internal::FunctionInterface<SignatureWithArrays>;
  using FunctionWithPortals = typename FunctionWithArrays::template StaticTransformType<
    detail::CompositeVectorArrayToPortalExec<DeviceAdapterTag>>::type;
  using SignatureWithPortals = typename FunctionWithPortals::Signature;

public:
  using ValueType = typename ArrayHandleCompositeVectorTraits<SignatureWithArrays>::ValueType;

  // These are not currently fully implemented.
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = ArrayPortalCompositeVector<SignatureWithPortals>;
  using PortalConstExecution = ArrayPortalCompositeVector<SignatureWithPortals>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Storage(storage)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData)) const
  {
    return PortalConstExecution(this->Storage->GetArrays().StaticTransformCont(
                                  detail::CompositeVectorArrayToPortalExec<DeviceAdapterTag>()),
                                this->Storage->GetSourceComponents());
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    // It may be the case a composite vector could be used for in place
    // operations, but this is not implemented currently.
    throw vtkm::cont::ErrorBadValue(
      "Composite vector arrays cannot be used for output or in place.");
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    // It may be the case a composite vector could be used for output if you
    // want the delegate arrays to be resized, but this is not implemented
    // currently.
    throw vtkm::cont::ErrorBadValue("Composite vector arrays cannot be used for output.");
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    throw vtkm::cont::ErrorBadValue("Composite vector arrays cannot be used for output.");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadValue("Composite vector arrays cannot be resized.");
  }

  VTKM_CONT
  void ReleaseResources() { this->Storage->ReleaseResources(); }

private:
  StorageType* Storage;
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
template <typename Signature>
class ArrayHandleCompositeVector
  : public internal::ArrayHandleCompositeVectorTraits<Signature>::Superclass
{
  using StorageType = typename internal::ArrayHandleCompositeVectorTraits<Signature>::StorageType;
  using ComponentMapType =
    typename internal::ArrayPortalCompositeVector<Signature>::ComponentMapType;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleCompositeVector,
    (ArrayHandleCompositeVector<Signature>),
    (typename internal::ArrayHandleCompositeVectorTraits<Signature>::Superclass));

  VTKM_CONT
  ArrayHandleCompositeVector(const vtkm::internal::FunctionInterface<Signature>& arrays,
                             const ComponentMapType& sourceComponents)
    : Superclass(StorageType(arrays, sourceComponents))
  {
  }

  /// Template constructors for passing in types. You'll get weird compile
  /// errors if the argument types do not actually match the types in the
  /// signature.
  ///
  template <typename ArrayHandleType1>
  VTKM_CONT ArrayHandleCompositeVector(const ArrayHandleType1& array1,
                                       vtkm::IdComponent sourceComponent1)
    : Superclass(StorageType(vtkm::internal::make_FunctionInterface<ValueType>(array1),
                             ComponentMapType(sourceComponent1)))
  {
  }
  template <typename ArrayHandleType1, typename ArrayHandleType2>
  VTKM_CONT ArrayHandleCompositeVector(const ArrayHandleType1& array1,
                                       vtkm::IdComponent sourceComponent1,
                                       const ArrayHandleType2& array2,
                                       vtkm::IdComponent sourceComponent2)
    : Superclass(StorageType(vtkm::internal::make_FunctionInterface<ValueType>(array1, array2),
                             ComponentMapType(sourceComponent1, sourceComponent2)))
  {
  }
  template <typename ArrayHandleType1, typename ArrayHandleType2, typename ArrayHandleType3>
  VTKM_CONT ArrayHandleCompositeVector(const ArrayHandleType1& array1,
                                       vtkm::IdComponent sourceComponent1,
                                       const ArrayHandleType2& array2,
                                       vtkm::IdComponent sourceComponent2,
                                       const ArrayHandleType3& array3,
                                       vtkm::IdComponent sourceComponent3)
    : Superclass(
        StorageType(vtkm::internal::make_FunctionInterface<ValueType>(array1, array2, array3),
                    ComponentMapType(sourceComponent1, sourceComponent2, sourceComponent3)))
  {
  }
  template <typename ArrayHandleType1,
            typename ArrayHandleType2,
            typename ArrayHandleType3,
            typename ArrayHandleType4>
  VTKM_CONT ArrayHandleCompositeVector(const ArrayHandleType1& array1,
                                       vtkm::IdComponent sourceComponent1,
                                       const ArrayHandleType2& array2,
                                       vtkm::IdComponent sourceComponent2,
                                       const ArrayHandleType3& array3,
                                       vtkm::IdComponent sourceComponent3,
                                       const ArrayHandleType4& array4,
                                       vtkm::IdComponent sourceComponent4)
    : Superclass(StorageType(
        vtkm::internal::make_FunctionInterface<ValueType>(array1, array2, array3, array4),
        ComponentMapType(sourceComponent1, sourceComponent2, sourceComponent3, sourceComponent4)))
  {
  }
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
template <typename ArrayHandleType1,
          typename ArrayHandleType2 = void,
          typename ArrayHandleType3 = void,
          typename ArrayHandleType4 = void>
struct ArrayHandleCompositeVectorType
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType1);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType2);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType3);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType4);

private:
  using ComponentType =
    typename vtkm::VecTraits<typename ArrayHandleType1::ValueType>::ComponentType;
  typedef vtkm::Vec<ComponentType, 4> Signature(ArrayHandleType1,
                                                ArrayHandleType2,
                                                ArrayHandleType3,
                                                ArrayHandleType4);

public:
  using type = vtkm::cont::ArrayHandleCompositeVector<Signature>;
};

template <typename ArrayHandleType1, typename ArrayHandleType2, typename ArrayHandleType3>
struct ArrayHandleCompositeVectorType<ArrayHandleType1, ArrayHandleType2, ArrayHandleType3>
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType1);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType2);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType3);

private:
  using ComponentType =
    typename vtkm::VecTraits<typename ArrayHandleType1::ValueType>::ComponentType;
  typedef vtkm::Vec<ComponentType, 3> Signature(ArrayHandleType1,
                                                ArrayHandleType2,
                                                ArrayHandleType3);

public:
  using type = vtkm::cont::ArrayHandleCompositeVector<Signature>;
};

template <typename ArrayHandleType1, typename ArrayHandleType2>
struct ArrayHandleCompositeVectorType<ArrayHandleType1, ArrayHandleType2>
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType1);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType2);

private:
  using ComponentType =
    typename vtkm::VecTraits<typename ArrayHandleType1::ValueType>::ComponentType;
  typedef vtkm::Vec<ComponentType, 2> Signature(ArrayHandleType1, ArrayHandleType2);

public:
  using type = vtkm::cont::ArrayHandleCompositeVector<Signature>;
};

template <typename ArrayHandleType1>
struct ArrayHandleCompositeVectorType<ArrayHandleType1>
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType1);

private:
  using ComponentType =
    typename vtkm::VecTraits<typename ArrayHandleType1::ValueType>::ComponentType;
  typedef ComponentType Signature(ArrayHandleType1);

public:
  using type = vtkm::cont::ArrayHandleCompositeVector<Signature>;
};

// clang-format off
/// Create a composite vector array from other arrays.
///
template <typename ValueType1, typename Storage1>
VTKM_CONT
typename ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<ValueType1, Storage1>>::type
make_ArrayHandleCompositeVector(const vtkm::cont::ArrayHandle<ValueType1, Storage1>& array1,
                                vtkm::IdComponent sourceComponent1)
{
  return
    typename ArrayHandleCompositeVectorType<vtkm::cont::ArrayHandle<ValueType1, Storage1>>::type(
      array1, sourceComponent1);
}
// clang-format on

template <typename ArrayHandleType1>
VTKM_CONT typename ArrayHandleCompositeVectorType<ArrayHandleType1>::type
make_ArrayHandleCompositeVector(const ArrayHandleType1& array1, vtkm::IdComponent sourceComponent1)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType1);
  return typename ArrayHandleCompositeVectorType<ArrayHandleType1>::type(array1, sourceComponent1);
}
template <typename ArrayHandleType1, typename ArrayHandleType2>
VTKM_CONT typename ArrayHandleCompositeVectorType<ArrayHandleType1, ArrayHandleType2>::type
make_ArrayHandleCompositeVector(const ArrayHandleType1& array1,
                                vtkm::IdComponent sourceComponent1,
                                const ArrayHandleType2& array2,
                                vtkm::IdComponent sourceComponent2)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType1);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType2);
  return typename ArrayHandleCompositeVectorType<ArrayHandleType1, ArrayHandleType2>::type(
    array1, sourceComponent1, array2, sourceComponent2);
}
template <typename ArrayHandleType1, typename ArrayHandleType2, typename ArrayHandleType3>
VTKM_CONT typename ArrayHandleCompositeVectorType<ArrayHandleType1,
                                                  ArrayHandleType2,
                                                  ArrayHandleType3>::type
make_ArrayHandleCompositeVector(const ArrayHandleType1& array1,
                                vtkm::IdComponent sourceComponent1,
                                const ArrayHandleType2& array2,
                                vtkm::IdComponent sourceComponent2,
                                const ArrayHandleType3& array3,
                                vtkm::IdComponent sourceComponent3)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType1);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType2);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType3);
  return
    typename ArrayHandleCompositeVectorType<ArrayHandleType1, ArrayHandleType2, ArrayHandleType3>::
      type(array1, sourceComponent1, array2, sourceComponent2, array3, sourceComponent3);
}
template <typename ArrayHandleType1,
          typename ArrayHandleType2,
          typename ArrayHandleType3,
          typename ArrayHandleType4>
VTKM_CONT typename ArrayHandleCompositeVectorType<ArrayHandleType1,
                                                  ArrayHandleType2,
                                                  ArrayHandleType3,
                                                  ArrayHandleType4>::type
make_ArrayHandleCompositeVector(const ArrayHandleType1& array1,
                                vtkm::IdComponent sourceComponent1,
                                const ArrayHandleType2& array2,
                                vtkm::IdComponent sourceComponent2,
                                const ArrayHandleType3& array3,
                                vtkm::IdComponent sourceComponent3,
                                const ArrayHandleType4& array4,
                                vtkm::IdComponent sourceComponent4)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType1);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType2);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType3);
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType4);
  return typename ArrayHandleCompositeVectorType<ArrayHandleType1,
                                                 ArrayHandleType2,
                                                 ArrayHandleType3,
                                                 ArrayHandleType4>::type(array1,
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
