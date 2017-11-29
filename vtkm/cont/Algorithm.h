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
#ifndef vtk_m_cont_Algorithm_h
#define vtk_m_cont_Algorithm_h

#include <vtkm/Types.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/internal/ArrayManagerExecution.h>
#include <vtkm/cont/internal/ArrayManagerExecution.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

namespace vtkm
{
namespace cont
{

namespace
{
struct CopyFunctor
{
  template <typename Device, typename T, typename U, class CIn, class COut>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            vtkm::cont::ArrayHandle<U, COut>& output)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Copy(input, output);
    return true;
  }
};

template <typename T, typename U, class CIn, class CStencil, class COut>
struct CopyIfFunctor
{
  const vtkm::cont::ArrayHandle<T, CIn>& Input;
  const vtkm::cont::ArrayHandle<U, CStencil>& Stencil;
  vtkm::cont::ArrayHandle<T, COut>& Output;

  CopyIfFunctor(const vtkm::cont::ArrayHandle<T, CIn>& input,
                const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                vtkm::cont::ArrayHandle<T, COut>& output)
    : Input(input)
    , Stencil(stencil)
    , Output(output)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::CopyIf(Input, Stencil, Output);
    return true;
  }
};

struct CopyIfPredicateFunctor
{
  template <typename Device,
            typename T,
            typename U,
            class CIn,
            class CStencil,
            class COut,
            class UnaryPredicate>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                            vtkm::cont::ArrayHandle<T, COut>& output,
                            UnaryPredicate unary_predicate)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::CopyIf(input, stencil, output, unary_predicate);
    return true;
  }
};

struct CopySubRangeFunctor
{
  bool valid;

  template <typename Device, typename T, typename U, class CIn, class COut>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            vtkm::Id inputStartIndex,
                            vtkm::Id numberOfElementsToCopy,
                            vtkm::cont::ArrayHandle<U, COut>& output,
                            vtkm::Id outputIndex)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    valid = vtkm::cont::DeviceAdapterAlgorithm<Device>::CopySubRange(
      input, inputStartIndex, numberOfElementsToCopy, output, outputIndex);
    return true;
  }
};

struct LowerBoundsFunctor
{

  template <typename Device, typename T, class CIn, class CVal, class COut>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            const vtkm::cont::ArrayHandle<T, CVal>& values,
                            vtkm::cont::ArrayHandle<vtkm::Id, COut>& output)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::LowerBounds(input, values, output);
    return true;
  }
};

struct LowerBoundsCompareFunctor
{

  template <typename Device, typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            const vtkm::cont::ArrayHandle<T, CVal>& values,
                            vtkm::cont::ArrayHandle<vtkm::Id, COut>& output,
                            BinaryCompare binary_compare)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::LowerBounds(input, values, output, binary_compare);
    return true;
  }
};

struct LowerBoundsInPlaceFunctor
{

  template <typename Device, class CIn, class COut>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<Id, CIn>& input,
                            vtkm::cont::ArrayHandle<Id, COut>& values)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::LowerBounds(input, values);
    return true;
  }
};

template <typename T, typename U, class CIn>
struct ReduceFunctor
{
  const vtkm::cont::ArrayHandle<T, CIn>& Input;
  U InitialValue;
  U Result;

  ReduceFunctor(const vtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
    : Input(input)
    , InitialValue(initialValue)
    , Result(U(0))
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    Result = vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(Input, InitialValue);
    return true;
  }
};

template <typename U>
struct ReduceBinaryFunctor
{
  U result;
  ReduceBinaryFunctor()
    : result(U(0))
  {
  }

  template <typename Device, typename T, class CIn, class BinaryFunctor>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result =
      vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(input, initialValue, binary_functor);
    return true;
  }
};

struct ReduceByKeyFunctor
{
  template <typename Device,
            typename T,
            typename U,
            class CKeyIn,
            class CValIn,
            class CKeyOut,
            class CValOut,
            class BinaryFunctor>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CKeyIn>& keys,
                            const vtkm::cont::ArrayHandle<U, CValIn>& values,
                            vtkm::cont::ArrayHandle<T, CKeyOut>& keys_output,
                            vtkm::cont::ArrayHandle<U, CValOut>& values_output,
                            BinaryFunctor binary_functor)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ReduceByKey(
      keys, values, keys_output, values_output, binary_functor);
    return true;
  }
};

template <typename T>
struct ScanInclusiveFunctor
{
  T result;
  ScanInclusiveFunctor()
    : result(T(0))
  {
  }

  template <typename Device, class CIn, class COut>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusive(input, output);
    return true;
  }
};

template <typename T>
struct StreamingScanExclusiveFunctor
{
  T result;
  StreamingScanExclusiveFunctor()
    : result(T(0))
  {
  }

  template <typename Device, class CIn, class COut>
  VTKM_CONT bool operator()(Device,
                            const vtkm::Id numBlocks,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result =
      vtkm::cont::DeviceAdapterAlgorithm<Device>::StreamingScanExclusive(numBlocks, input, output);
    return true;
  }
};

template <typename T>
struct ScanInclusiveBinaryFunctor
{
  T result;
  ScanInclusiveBinaryFunctor()
    : result(T(0))
  {
  }

  template <typename Device, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            vtkm::cont::ArrayHandle<T, COut>& output,
                            BinaryFunctor binary_functor)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result =
      vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusive(input, output, binary_functor);
    return true;
  }
};

struct ScanInclusiveByKeyBinaryFunctor
{
  template <typename Device,
            typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, KIn>& keys,
                            const vtkm::cont::ArrayHandle<U, VIn>& values,
                            vtkm::cont::ArrayHandle<U, VOut>& values_output,
                            BinaryFunctor binary_functor)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusiveByKey(
      keys, values, values_output, binary_functor);
    return true;
  }
};

struct ScanInclusiveByKeyFunctor
{

  template <typename Device, typename T, typename U, typename KIn, typename VIn, typename VOut>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, KIn>& keys,
                            const vtkm::cont::ArrayHandle<U, VIn>& values,
                            vtkm::cont::ArrayHandle<U, VOut>& values_output)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusiveByKey(keys, values, values_output);
    return true;
  }
};

template <typename T, class CIn, class COut>
struct ScanExclusiveFunctor
{
  T Result;
  ScanExclusiveFunctor()
    : Result(T(0))
  {
  }

  const vtkm::cont::ArrayHandle<T, CIn>& Input;
  vtkm::cont::ArrayHandle<T, COut>& Output;
  ScanExclusiveFunctor(const vtkm::cont::ArrayHandle<T, CIn>& input,
                       vtkm::cont::ArrayHandle<T, COut>& output)
    : Input(input)
    , Output(output)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    Result = vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(Input, Output);
    return true;
  }
};

template <typename T>
struct ScanExclusiveBinaryFunctor
{
  T result;
  ScanExclusiveBinaryFunctor()
    : result(T(0))
  {
  }

  template <typename Device, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            vtkm::cont::ArrayHandle<T, COut>& output,
                            BinaryFunctor binary_functor,
                            const T& initialValue)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(
      input, output, binary_functor, initialValue);
    return true;
  }
};

struct ScanExclusiveByKeyBinaryFunctor
{
  template <typename Device,
            typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            class BinaryFunctor>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, KIn>& keys,
                            const vtkm::cont::ArrayHandle<U, VIn>& values,
                            vtkm::cont::ArrayHandle<U, VOut>& output,
                            const U& initialValue,
                            BinaryFunctor binaryFunctor)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusiveByKey(
      keys, values, output, initialValue, binaryFunctor);
    return true;
  }
};

struct ScanExclusiveByKeyFunctor
{
  template <typename Device, typename T, typename U, class KIn, typename VIn, typename VOut>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, KIn>& keys,
                            const vtkm::cont::ArrayHandle<U, VIn>& values,
                            vtkm::cont::ArrayHandle<U, VOut>& output)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusiveByKey(keys, values, output);
    return true;
  }
};

struct ScheduleFunctor
{
  template <typename Device, class Functor>
  VTKM_CONT bool operator()(Device, Functor functor, vtkm::Id numInstances)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(functor, numInstances);
    return true;
  }
};

struct Schedule3DFunctor
{
  template <typename Device, class Functor>
  VTKM_CONT bool operator()(Device, Functor functor, vtkm::Id3 rangeMax)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(functor, rangeMax);
    return true;
  }
};

struct SortFunctor
{
  template <typename Device, typename T, class Storage>
  VTKM_CONT bool operator()(Device, vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Sort(values);
    return true;
  }
};

template <typename T, class Storage, class BinaryCompare>
struct SortBinaryCompareFunctor
{
  vtkm::cont::ArrayHandle<T, Storage>& Values;
  BinaryCompare Binary_compare;

  SortBinaryCompareFunctor(vtkm::cont::ArrayHandle<T, Storage>& values,
                           BinaryCompare binary_compare)
    : Values(values)
    , Binary_compare(binary_compare)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Sort(Values, Binary_compare);
    return true;
  }
};

struct SortByKeyFunctor
{
  template <typename Device, typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT bool operator()(Device,
                            vtkm::cont::ArrayHandle<T, StorageT>& keys,
                            vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::SortByKey(keys, values);
    return true;
  }
};

struct SortByKeyBinaryFunctor
{
  template <typename Device,
            typename T,
            typename U,
            class StorageT,
            class StorageU,
            class BinaryCompare>
  VTKM_CONT bool operator()(Device,
                            vtkm::cont::ArrayHandle<T, StorageT>& keys,
                            vtkm::cont::ArrayHandle<U, StorageU>& values,
                            BinaryCompare binary_compare)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::SortByKey(keys, values, binary_compare);
    return true;
  }
};

struct SynchronizeFunctor
{
  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Synchronize();
    return true;
  }
};

struct UniqueFunctor
{
  template <typename Device, typename T, class Storage>
  VTKM_CONT bool operator()(Device, vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Unique(values);
    return true;
  }
};

struct UniqueBinaryFunctor
{
  template <typename Device, typename T, class Storage, class BinaryCompare>
  VTKM_CONT bool operator()(Device,
                            vtkm::cont::ArrayHandle<T, Storage>& values,
                            BinaryCompare binary_compare)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Unique(values, binary_compare);
    return true;
  }
};

struct UpperBoundsFunctor
{
  template <typename Device, typename T, class CIn, class CVal, class COut>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            const vtkm::cont::ArrayHandle<T, CVal>& values,
                            vtkm::cont::ArrayHandle<vtkm::Id, COut>& output)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::UpperBounds(input, values, output);
    return true;
  }
};

struct UpperBoundsBinaryFunctor
{
  template <typename Device, typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            const vtkm::cont::ArrayHandle<T, CVal>& values,
                            vtkm::cont::ArrayHandle<vtkm::Id, COut>& output,
                            BinaryCompare binary_compare)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::UpperBounds(input, values, output, binary_compare);
    return true;
  }
};

struct UpperBoundsInPlaceFunctor
{
  template <typename Device, class CIn, class COut>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<Id, CIn>& input,
                            vtkm::cont::ArrayHandle<vtkm::Id, COut>& values_output)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::UpperBounds(input, values_output);
    return true;
  }
};

} // annonymous namespace

struct Algorithm
{

  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static void Copy(const vtkm::cont::ArrayHandle<T, CIn>& input,
                             vtkm::cont::ArrayHandle<U, COut>& output)
  {
    vtkm::cont::TryExecute(CopyFunctor(), input, output);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output)
  {
    CopyIfFunctor<T, U, CIn, CStencil, COut> functor(input, stencil, output);
    vtkm::cont::TryExecute(functor);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output,
                               UnaryPredicate unary_predicate)
  {
    vtkm::cont::TryExecute(CopyIfPredicateFunctor(), input, stencil, output, unary_predicate);
  }

  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static bool CopySubRange(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                     vtkm::Id inputStartIndex,
                                     vtkm::Id numberOfElementsToCopy,
                                     vtkm::cont::ArrayHandle<U, COut>& output,
                                     vtkm::Id outputIndex = 0)
  {
    CopySubRangeFunctor functor;
    vtkm::cont::TryExecute(
      functor, input, inputStartIndex, numberOfElementsToCopy, output, outputIndex);
    return functor.valid;
  }

  template <typename T, class CIn, class CVal, class COut>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output)
  {
    vtkm::cont::TryExecute(LowerBoundsFunctor(), input, values, output);
  }

  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output,
                                    BinaryCompare binary_compare)
  {
    vtkm::cont::TryExecute(LowerBoundsCompareFunctor(), input, values, output, binary_compare);
  }

  template <class CIn, class COut>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<vtkm::Id, CIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& values_output)
  {
    vtkm::cont::TryExecute(LowerBoundsInPlaceFunctor(), input, values_output);
  }

  template <typename T, typename U, class CIn>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    ReduceFunctor<T, U, CIn> functor(input, initialValue);
    vtkm::cont::TryExecute(functor);
    return functor.Result;
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    ReduceBinaryFunctor<U> functor;
    vtkm::cont::TryExecute(functor, input, initialValue, binary_functor);
    return functor.result;
  }

  template <typename T,
            typename U,
            class CKeyIn,
            class CValIn,
            class CKeyOut,
            class CValOut,
            class BinaryFunctor>
  VTKM_CONT static void ReduceByKey(const vtkm::cont::ArrayHandle<T, CKeyIn>& keys,
                                    const vtkm::cont::ArrayHandle<U, CValIn>& values,
                                    vtkm::cont::ArrayHandle<T, CKeyOut>& keys_output,
                                    vtkm::cont::ArrayHandle<U, CValOut>& values_output,
                                    BinaryFunctor binary_functor)
  {
    vtkm::cont::TryExecute(
      ReduceByKeyFunctor(), keys, values, keys_output, values_output, binary_functor);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    ScanInclusiveFunctor<T> functor;
    vtkm::cont::TryExecute(functor, input, output);
    return functor.result;
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T StreamingScanExclusive(const vtkm::Id numBlocks,
                                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                                            vtkm::cont::ArrayHandle<T, COut>& output)
  {
    StreamingScanExclusiveFunctor<T> functor;
    vtkm::cont::TryExecute(functor, numBlocks, input, output);
    return functor.result;
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor)
  {
    ScanInclusiveBinaryFunctor<T> functor;
    vtkm::cont::TryExecute(functor, input, output, binary_functor);
    return functor.result;
  }

  template <typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  VTKM_CONT static void ScanInclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& values_output,
                                           BinaryFunctor binary_functor)
  {
    vtkm::cont::TryExecute(
      ScanInclusiveByKeyBinaryFunctor(), keys, values, values_output, binary_functor);
  }

  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanInclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& values_output)
  {
    vtkm::cont::TryExecute(ScanInclusiveByKeyFunctor(), keys, values, values_output);
  }

  //template <typename T, class CIn, class COut, class BinaryFunctor>
  //VTKM_CONT static T StreamingScanInclusive(const vtkm::Id numBlocks,
  //                                          const vtkm::cont::ArrayHandle<T, CIn>& input,
  //                                          vtkm::cont::ArrayHandle<T, COut>& output,
  //                                          BinaryFunctor binary_functor);

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    ScanExclusiveFunctor<T, CIn, COut> functor(input, output);
    vtkm::cont::TryExecute(functor);
    return functor.Result;
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor,
                                   const T& initialValue)
  {
    ScanExclusiveBinaryFunctor<T> functor;
    vtkm::cont::TryExecute(functor, input, output, binaryFunctor, initialValue);
    return functor.result;
  }

  template <typename T, typename U, typename KIn, typename VIn, typename VOut, class BinaryFunctor>
  VTKM_CONT static void ScanExclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output,
                                           const U& initialValue,
                                           BinaryFunctor binaryFunctor)
  {
    ScanExclusiveByKeyBinaryFunctor functor;
    vtkm::cont::TryExecute(functor, keys, values, output, initialValue, binaryFunctor);
  }

  template <typename T, typename U, class KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanExclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output)
  {
    ScanExclusiveByKeyFunctor functor;
    vtkm::cont::TryExecute(functor, keys, values, output);
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id numInstances)
  {
    vtkm::cont::TryExecute(ScheduleFunctor(), functor, numInstances);
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id3 rangeMax)
  {
    vtkm::cont::TryExecute(Schedule3DFunctor(), functor, rangeMax);
  }

  template <typename T, class Storage>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    vtkm::cont::TryExecute(SortFunctor(), values);
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    SortBinaryCompareFunctor<T, Storage, BinaryCompare> functor(values, binary_compare);
    vtkm::cont::TryExecute(functor);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    vtkm::cont::TryExecute(SortByKeyFunctor(), keys, values);
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    vtkm::cont::TryExecute(SortByKeyBinaryFunctor(), keys, values, binary_compare);
  }

  VTKM_CONT static void Synchronize() { vtkm::cont::TryExecute(SynchronizeFunctor()); }

  template <typename T, class Storage>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    vtkm::cont::TryExecute(UniqueFunctor(), values);
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    vtkm::cont::TryExecute(UniqueBinaryFunctor(), values, binary_compare);
  }

  template <typename T, class CIn, class CVal, class COut>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output)
  {
    vtkm::cont::TryExecute(UpperBoundsFunctor(), input, values, output);
  }

  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output,
                                    BinaryCompare binary_compare)
  {
    vtkm::cont::TryExecute(UpperBoundsBinaryFunctor(), input, values, output, binary_compare);
  }

  template <class CIn, class COut>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<vtkm::Id, CIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& values_output)
  {
    vtkm::cont::TryExecute(UpperBoundsInPlaceFunctor(), input, values_output);
  }
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_Algorithm_h
