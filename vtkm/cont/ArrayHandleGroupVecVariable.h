//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ArrayHandleGroupVecVariable_h
#define vtk_m_cont_ArrayHandleGroupVecVariable_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayPortal.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/TryExecute.h>

#include <vtkm/Assert.h>
#include <vtkm/VecFromPortal.h>

#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>

namespace vtkm
{
namespace exec
{

namespace internal
{

template <typename SourcePortalType, typename OffsetsPortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalGroupVecVariable
{
public:
  using ComponentType = typename std::remove_const<typename SourcePortalType::ValueType>::type;
  using ValueType = vtkm::VecFromPortal<SourcePortalType>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalGroupVecVariable()
    : SourcePortal()
    , OffsetsPortal()
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ArrayPortalGroupVecVariable(const SourcePortalType& sourcePortal,
                              const OffsetsPortalType& offsetsPortal)
    : SourcePortal(sourcePortal)
    , OffsetsPortal(offsetsPortal)
  {
  }

  /// Copy constructor for any other ArrayPortalConcatenate with a portal type
  /// that can be copied to this portal type. This allows us to do any type
  /// casting that the portals do (like the non-const to const cast).
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OtherSourcePortalType, typename OtherOffsetsPortalType>
  VTKM_EXEC_CONT ArrayPortalGroupVecVariable(
    const ArrayPortalGroupVecVariable<OtherSourcePortalType, OtherOffsetsPortalType>& src)
    : SourcePortal(src.GetSourcePortal())
    , OffsetsPortal(src.GetOffsetsPortal())
  {
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->OffsetsPortal.GetNumberOfValues(); }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    vtkm::Id offsetIndex = this->OffsetsPortal.Get(index);
    vtkm::Id nextOffsetIndex;
    if (index + 1 < this->GetNumberOfValues())
    {
      nextOffsetIndex = this->OffsetsPortal.Get(index + 1);
    }
    else
    {
      nextOffsetIndex = this->SourcePortal.GetNumberOfValues();
    }

    return ValueType(this->SourcePortal,
                     static_cast<vtkm::IdComponent>(nextOffsetIndex - offsetIndex),
                     offsetIndex);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  void Set(vtkm::Id vtkmNotUsed(index), const ValueType& vtkmNotUsed(value)) const
  {
    // The ValueType (VecFromPortal) operates on demand. Thus, if you set
    // something in the value, it has already been passed to the array. Perhaps
    // we should check to make sure that the value used matches the location
    // you are trying to set in the array, but we don't do that.
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const SourcePortalType& GetSourcePortal() const { return this->SourcePortal; }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC_CONT
  const OffsetsPortalType& GetOffsetsPortal() const { return this->OffsetsPortal; }

private:
  SourcePortalType SourcePortal;
  OffsetsPortalType OffsetsPortal;
};

} // namespace internal (in vtkm::exec)

namespace arg
{

// We need to override the fetch for output fields using
// ArrayPortalGroupVecVariable because this portal does not behave like most
// ArrayPortals. Usually you ignore the Load and implement the Store. But if
// you ignore the Load, the VecFromPortal gets no portal to set values into.
// Instead, you need to implement the Load to point to the array portal. You
// can also ignore the Store because the data is already set in the array at
// that point.
template <typename ThreadIndicesType, typename SourcePortalType, typename OffsetsPortalType>
struct Fetch<vtkm::exec::arg::FetchTagArrayDirectOut,
             vtkm::exec::arg::AspectTagDefault,
             ThreadIndicesType,
             vtkm::exec::internal::ArrayPortalGroupVecVariable<SourcePortalType, OffsetsPortalType>>
{
  using ExecObjectType =
    vtkm::exec::internal::ArrayPortalGroupVecVariable<SourcePortalType, OffsetsPortalType>;
  using ValueType = typename ExecObjectType::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType& arrayPortal) const
  {
    return arrayPortal.Get(indices.GetOutputIndex());
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // We can actually ignore this because the VecFromPortal will already have
    // set new values in the array.
  }
};

} // namespace arg (in vtkm::exec)
}
} // namespace vtkm::exec

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename SourceArrayHandleType, typename OffsetsArrayHandleType>
struct VTKM_ALWAYS_EXPORT StorageTagGroupVecVariable
{
};

template <typename SourceArrayHandleType, typename OffsetsArrayHandleType>
class Storage<
  vtkm::VecFromPortal<typename SourceArrayHandleType::PortalControl>,
  vtkm::cont::internal::StorageTagGroupVecVariable<SourceArrayHandleType, OffsetsArrayHandleType>>
{
  using ComponentType = typename SourceArrayHandleType::ValueType;

public:
  using ValueType = vtkm::VecFromPortal<typename SourceArrayHandleType::PortalControl>;

  using PortalType = vtkm::exec::internal::ArrayPortalGroupVecVariable<
    typename SourceArrayHandleType::PortalControl,
    typename OffsetsArrayHandleType::PortalControl>;
  using PortalConstType = vtkm::exec::internal::ArrayPortalGroupVecVariable<
    typename SourceArrayHandleType::PortalConstControl,
    typename OffsetsArrayHandleType::PortalConstControl>;

  VTKM_CONT
  Storage()
    : Valid(false)
  {
  }

  VTKM_CONT
  Storage(const SourceArrayHandleType& sourceArray, const OffsetsArrayHandleType& offsetsArray)
    : SourceArray(sourceArray)
    , OffsetsArray(offsetsArray)
    , Valid(true)
  {
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    return PortalType(this->SourceArray.GetPortalControl(), this->OffsetsArray.GetPortalControl());
  }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->SourceArray.GetPortalConstControl(),
                           this->OffsetsArray.GetPortalConstControl());
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->Valid);
    return this->OffsetsArray.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    VTKM_ASSERT("Allocate not supported for ArrayhandleGroupVecVariable" && false);
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(this->Valid);
    this->OffsetsArray.Shrink(numberOfValues);
  }

  VTKM_CONT
  void ReleaseResources()
  {
    if (this->Valid)
    {
      this->SourceArray.ReleaseResources();
      this->OffsetsArray.ReleaseResources();
    }
  }

  // Required for later use in ArrayTransfer class
  VTKM_CONT
  const SourceArrayHandleType& GetSourceArray() const
  {
    VTKM_ASSERT(this->Valid);
    return this->SourceArray;
  }

  // Required for later use in ArrayTransfer class
  VTKM_CONT
  const OffsetsArrayHandleType& GetOffsetsArray() const
  {
    VTKM_ASSERT(this->Valid);
    return this->OffsetsArray;
  }

private:
  SourceArrayHandleType SourceArray;
  OffsetsArrayHandleType OffsetsArray;
  bool Valid;
};

template <typename SourceArrayHandleType, typename OffsetsArrayHandleType, typename Device>
class ArrayTransfer<
  vtkm::VecFromPortal<typename SourceArrayHandleType::PortalControl>,
  vtkm::cont::internal::StorageTagGroupVecVariable<SourceArrayHandleType, OffsetsArrayHandleType>,
  Device>
{
public:
  using ComponentType = typename SourceArrayHandleType::ValueType;
  using ValueType = vtkm::VecFromPortal<typename SourceArrayHandleType::PortalControl>;

private:
  using StorageTag =
    vtkm::cont::internal::StorageTagGroupVecVariable<SourceArrayHandleType, OffsetsArrayHandleType>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;

  using PortalExecution = vtkm::exec::internal::ArrayPortalGroupVecVariable<
    typename SourceArrayHandleType::template ExecutionTypes<Device>::Portal,
    typename OffsetsArrayHandleType::template ExecutionTypes<Device>::PortalConst>;
  using PortalConstExecution = vtkm::exec::internal::ArrayPortalGroupVecVariable<
    typename SourceArrayHandleType::template ExecutionTypes<Device>::PortalConst,
    typename OffsetsArrayHandleType::template ExecutionTypes<Device>::PortalConst>;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : SourceArray(storage->GetSourceArray())
    , OffsetsArray(storage->GetOffsetsArray())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->OffsetsArray.GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->SourceArray.PrepareForInput(Device()),
                                this->OffsetsArray.PrepareForInput(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(this->SourceArray.PrepareForInPlace(Device()),
                           this->OffsetsArray.PrepareForInput(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    // Cannot reallocate an ArrayHandleGroupVecVariable
    VTKM_ASSERT(numberOfValues == this->OffsetsArray.GetNumberOfValues());
    return PortalExecution(
      this->SourceArray.PrepareForOutput(this->SourceArray.GetNumberOfValues(), Device()),
      this->OffsetsArray.PrepareForInput(Device()));
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // Implementation of this method should be unnecessary. The internal
    // array handles should automatically retrieve the output data as
    // necessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->OffsetsArray.Shrink(numberOfValues); }

  VTKM_CONT
  void ReleaseResources()
  {
    this->SourceArray.ReleaseResourcesExecution();
    this->OffsetsArray.ReleaseResourcesExecution();
  }

private:
  SourceArrayHandleType SourceArray;
  OffsetsArrayHandleType OffsetsArray;
};

} // namespace internal

/// \brief Fancy array handle that groups values into vectors of different sizes.
///
/// It is sometimes the case that you need to run a worklet with an input or
/// output that has a different number of values per instance. For example, the
/// cells of a CellCetExplicit can have different numbers of points in each
/// cell. If inputting or outputting cells of this type, each instance of the
/// worklet might need a \c Vec of a different length. This fance array handle
/// takes an array of values and an array of offsets and groups the consecutive
/// values in Vec-like objects. The values are treated as tightly packed, so
/// that each Vec contains the values from one offset to the next. The last
/// value contains values from the last offset to the end of the array.
///
/// For example, if you have an array handle with the 9 values
/// 0,1,2,3,4,5,6,7,8 an offsets array handle with the 3 values 0,4,6 and give
/// them to an \c ArrayHandleGroupVecVariable, you get an array that looks like
/// it contains three values of Vec-like objects with the data [0,1,2,3],
/// [4,5], and [6,7,8].
///
/// Note that this version of \c ArrayHandle breaks some of the assumptions
/// about \c ArrayHandle a little bit. Typically, there is exactly one type for
/// every value in the array, and this value is also the same between the
/// control and execution environment. However, this class uses \c
/// VecFromPortal it implement a Vec-like class that has a variable number of
/// values, and this type can change between control and execution
/// environments.
///
/// The offsets array is often derived from a list of sizes for each of the
/// entries. You can use the convenience function \c
/// ConvertNumComponentsToOffsets to take an array of sizes (i.e. the number of
/// components for each entry) and get an array of offsets needed for \c
/// ArrayHandleGroupVecVariable.
///
template <typename SourceArrayHandleType, typename OffsetsArrayHandleType>
class ArrayHandleGroupVecVariable
  : public vtkm::cont::ArrayHandle<
      vtkm::VecFromPortal<typename SourceArrayHandleType::PortalControl>,
      vtkm::cont::internal::StorageTagGroupVecVariable<SourceArrayHandleType,
                                                       OffsetsArrayHandleType>>
{
  VTKM_IS_ARRAY_HANDLE(SourceArrayHandleType);
  VTKM_IS_ARRAY_HANDLE(OffsetsArrayHandleType);

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(
    ArrayHandleGroupVecVariable,
    (ArrayHandleGroupVecVariable<SourceArrayHandleType, OffsetsArrayHandleType>),
    (vtkm::cont::ArrayHandle<
      vtkm::VecFromPortal<typename SourceArrayHandleType::PortalControl>,
      vtkm::cont::internal::StorageTagGroupVecVariable<SourceArrayHandleType,
                                                       OffsetsArrayHandleType>>));

  using ComponentType = typename SourceArrayHandleType::ValueType;

private:
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;

public:
  VTKM_CONT
  ArrayHandleGroupVecVariable(const SourceArrayHandleType& sourceArray,
                              const OffsetsArrayHandleType& offsetsArray)
    : Superclass(StorageType(sourceArray, offsetsArray))
  {
  }
};

/// \c make_ArrayHandleGroupVecVariable is convenience function to generate an
/// ArrayHandleGroupVecVariable. It takes in an ArrayHandle of values and an
/// array handle of offsets and returns an array handle with consecutive
/// entries grouped in a Vec.
///
template <typename SourceArrayHandleType, typename OffsetsArrayHandleType>
VTKM_CONT vtkm::cont::ArrayHandleGroupVecVariable<SourceArrayHandleType, OffsetsArrayHandleType>
make_ArrayHandleGroupVecVariable(const SourceArrayHandleType& sourceArray,
                                 const OffsetsArrayHandleType& offsetsArray)
{
  return vtkm::cont::ArrayHandleGroupVecVariable<SourceArrayHandleType, OffsetsArrayHandleType>(
    sourceArray, offsetsArray);
}

/// \c ConvertNumComponentsToOffsets takes an array of Vec sizes (i.e. the number of components in
/// each Vec) and returns an array of offsets to a packed array of such Vecs. The resulting array
/// can be used with \c ArrayHandleGroupVecVariable.
///
/// The first parameter is always the input array that specifies the number of components in each
/// group Vec.
///
/// The next parameter is the output \c ArrayHandle, which must have a value type of \c vtkm::Id.
/// If the output \c ArrayHandle is not given, it is returned.
///
/// The next optional parameter is a reference to a \c vtkm::Id and is filled with the expected
/// size of the source values array.
///
/// The final optional parameter is either a device adapter tag or a \c RuntimeDeviceTracker. If a
/// device is not specified, then devices specified by the global \c RuntimeDeviceTracker are used.
///
template <typename NumComponentsArrayType, typename OffsetsStorage, typename Device>
VTKM_CONT void ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage>& offsetsArray,
  vtkm::Id& sourceArraySize,
  Device)
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  sourceArraySize = vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(
    vtkm::cont::make_ArrayHandleCast<vtkm::Id>(numComponentsArray), offsetsArray);
}

template <typename NumComponentsArrayType, typename OffsetsStorage, typename Device>
VTKM_CONT void ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage>& offsetsArray,
  Device)
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  vtkm::Id dummy;
  vtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, offsetsArray, dummy, Device());
}

template <typename NumComponentsArrayType, typename Device>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::Id& sourceArraySize,
  Device)
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray;
  vtkm::cont::ConvertNumComponentsToOffsets(
    numComponentsArray, offsetsArray, sourceArraySize, Device());
  return offsetsArray;
}

template <typename NumComponentsArrayType, typename Device>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  Device)
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);
  VTKM_IS_DEVICE_ADAPTER_TAG(Device);

  vtkm::Id dummy;
  return vtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, dummy, Device());
}

namespace detail
{

template <typename NumComponentsArrayType, typename OffsetsArrayType>
struct ConvertNumComponentsToOffsetsFunctor
{
  const NumComponentsArrayType NumComponentsArray;
  OffsetsArrayType OffsetsArray;
  vtkm::Id SourceArraySize;

  VTKM_CONT
  ConvertNumComponentsToOffsetsFunctor(const NumComponentsArrayType& numCompArray)
    : NumComponentsArray(numCompArray)
    , SourceArraySize(0)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    vtkm::cont::ConvertNumComponentsToOffsets(
      this->NumComponentsArray, this->OffsetsArray, this->SourceArraySize, Device());

    return true;
  }
};

template <typename NumComponentsArrayType, typename OffsetsArrayType>
VTKM_CONT void DoConvertNumComponentsToOffsets(const NumComponentsArrayType& numComponentsArray,
                                               OffsetsArrayType& offsetsArray,
                                               vtkm::Id& sourceArraySize,
                                               vtkm::cont::RuntimeDeviceTracker tracker)
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);
  VTKM_IS_ARRAY_HANDLE(OffsetsArrayType);

  detail::ConvertNumComponentsToOffsetsFunctor<NumComponentsArrayType, OffsetsArrayType> functor(
    numComponentsArray);
  bool success = vtkm::cont::TryExecute(functor, tracker);

  if (!success)
  {
    // Internal error? Maybe need to make a failed to execute error.
    throw vtkm::cont::ErrorInternal("Failed to run ExclusiveScan on any device.");
  }

  sourceArraySize = functor.SourceArraySize;
  offsetsArray = functor.OffsetsArray;
}

} // namespace detail

template <typename NumComponentsArrayType, typename OffsetsStorage>
VTKM_CONT void ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage>& offsetsArray,
  vtkm::Id& sourceArraySize,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  detail::DoConvertNumComponentsToOffsets(
    numComponentsArray, offsetsArray, sourceArraySize, tracker);
}

template <typename NumComponentsArrayType, typename OffsetsStorage>
VTKM_CONT void ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage>& offsetsArray,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  vtkm::Id dummy;
  vtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, offsetsArray, dummy, tracker);
}

template <typename NumComponentsArrayType>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::Id& sourceArraySize,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  vtkm::cont::ArrayHandle<vtkm::Id> offsetsArray;
  vtkm::cont::ConvertNumComponentsToOffsets(
    numComponentsArray, offsetsArray, sourceArraySize, tracker);
  return offsetsArray;
}

template <typename NumComponentsArrayType>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumComponentsToOffsets(
  const NumComponentsArrayType& numComponentsArray,
  vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker())
{
  VTKM_IS_ARRAY_HANDLE(NumComponentsArrayType);

  vtkm::Id dummy;
  return vtkm::cont::ConvertNumComponentsToOffsets(numComponentsArray, dummy, tracker);
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_ArrayHandleGroupVecVariable_h
