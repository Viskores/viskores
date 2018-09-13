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

#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/internal/ArrayManagerExecution.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

namespace vtkm
{
namespace cont
{

namespace detail
{
template <typename Device, typename T>
inline auto DoPrepareArgForExec(T&& object, std::true_type)
  -> decltype(std::declval<T>().PrepareForExecution(Device()))
{
  VTKM_IS_EXECUTION_OBJECT(T);
  return object.PrepareForExecution(Device{});
}

template <typename Device, typename T>
inline T&& DoPrepareArgForExec(T&& object, std::false_type)
{
  static_assert(!vtkm::cont::internal::IsExecutionObjectBase<T>::value,
                "Internal error: failed to detect execution object.");
  return std::forward<T>(object);
}

template <typename Device, typename T>
auto PrepareArgForExec(T&& object)
  -> decltype(DoPrepareArgForExec<Device>(std::forward<T>(object),
                                          vtkm::cont::internal::IsExecutionObjectBase<T>{}))
{
  return DoPrepareArgForExec<Device>(std::forward<T>(object),
                                     vtkm::cont::internal::IsExecutionObjectBase<T>{});
}

struct CopyFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Copy(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct CopyIfFunctor
{

  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::CopyIf(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct CopySubRangeFunctor
{
  bool valid;

  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    valid = vtkm::cont::DeviceAdapterAlgorithm<Device>::CopySubRange(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct LowerBoundsFunctor
{

  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::LowerBounds(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

template <typename U>
struct ReduceFunctor
{
  U result;

  ReduceFunctor()
    : result(U(0))
  {
  }

  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct ReduceByKeyFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ReduceByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

template <typename T>
struct ScanInclusiveResultFunctor
{
  T result;
  ScanInclusiveResultFunctor()
    : result(T(0))
  {
  }

  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusive(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
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

struct ScanInclusiveByKeyFunctor
{
  ScanInclusiveByKeyFunctor() {}

  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusiveByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

template <typename T>
struct ScanExclusiveFunctor
{
  T result;
  ScanExclusiveFunctor()
    : result(T(0))
  {
  }

  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    result = vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct ScanExclusiveByKeyFunctor
{
  ScanExclusiveByKeyFunctor() {}

  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusiveByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct ScheduleFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct SortFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Sort(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct SortByKeyFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::SortByKey(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
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

struct TransformFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Transform(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct UniqueFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Unique(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};

struct UpperBoundsFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::UpperBounds(
      PrepareArgForExec<Device>(std::forward<Args>(args))...);
    return true;
  }
};
} // namespace detail

struct Algorithm
{

  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static void Copy(vtkm::cont::DeviceAdapterId devId,
                             const vtkm::cont::ArrayHandle<T, CIn>& input,
                             vtkm::cont::ArrayHandle<U, COut>& output)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::CopyFunctor(), input, output);
  }
  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static void Copy(const vtkm::cont::ArrayHandle<T, CIn>& input,
                             vtkm::cont::ArrayHandle<U, COut>& output)
  {
    Copy(vtkm::cont::DeviceAdapterTagAny(), input, output);
  }


  template <typename T, typename U, class CIn, class CStencil, class COut>
  VTKM_CONT static void CopyIf(vtkm::cont::DeviceAdapterId devId,
                               const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::CopyIfFunctor(), input, stencil, output);
  }
  template <typename T, typename U, class CIn, class CStencil, class COut>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output)
  {
    CopyIf(vtkm::cont::DeviceAdapterTagAny(), input, stencil, output);
  }


  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VTKM_CONT static void CopyIf(vtkm::cont::DeviceAdapterId devId,
                               const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output,
                               UnaryPredicate unary_predicate)
  {
    vtkm::cont::TryExecuteOnDevice(
      devId, detail::CopyIfFunctor(), input, stencil, output, unary_predicate);
  }
  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output,
                               UnaryPredicate unary_predicate)
  {
    CopyIf(vtkm::cont::DeviceAdapterTagAny(), input, stencil, output, unary_predicate);
  }


  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static bool CopySubRange(vtkm::cont::DeviceAdapterId devId,
                                     const vtkm::cont::ArrayHandle<T, CIn>& input,
                                     vtkm::Id inputStartIndex,
                                     vtkm::Id numberOfElementsToCopy,
                                     vtkm::cont::ArrayHandle<U, COut>& output,
                                     vtkm::Id outputIndex = 0)
  {
    detail::CopySubRangeFunctor functor;
    vtkm::cont::TryExecuteOnDevice(
      devId, functor, input, inputStartIndex, numberOfElementsToCopy, output, outputIndex);
    return functor.valid;
  }
  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static bool CopySubRange(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                     vtkm::Id inputStartIndex,
                                     vtkm::Id numberOfElementsToCopy,
                                     vtkm::cont::ArrayHandle<U, COut>& output,
                                     vtkm::Id outputIndex = 0)
  {
    return CopySubRange(vtkm::cont::DeviceAdapterTagAny(),
                        input,
                        inputStartIndex,
                        numberOfElementsToCopy,
                        output,
                        outputIndex);
  }


  template <typename T, class CIn, class CVal, class COut>
  VTKM_CONT static void LowerBounds(vtkm::cont::DeviceAdapterId devId,
                                    const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::LowerBoundsFunctor(), input, values, output);
  }
  template <typename T, class CIn, class CVal, class COut>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output)
  {
    LowerBounds(vtkm::cont::DeviceAdapterTagAny(), input, values, output);
  }


  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT static void LowerBounds(vtkm::cont::DeviceAdapterId devId,
                                    const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output,
                                    BinaryCompare binary_compare)
  {
    vtkm::cont::TryExecuteOnDevice(
      devId, detail::LowerBoundsFunctor(), input, values, output, binary_compare);
  }
  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output,
                                    BinaryCompare binary_compare)
  {
    LowerBounds(vtkm::cont::DeviceAdapterTagAny(), input, values, output, binary_compare);
  }


  template <class CIn, class COut>
  VTKM_CONT static void LowerBounds(vtkm::cont::DeviceAdapterId devId,
                                    const vtkm::cont::ArrayHandle<vtkm::Id, CIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& values_output)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::LowerBoundsFunctor(), input, values_output);
  }
  template <class CIn, class COut>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<vtkm::Id, CIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& values_output)
  {
    LowerBounds(vtkm::cont::DeviceAdapterTagAny(), input, values_output);
  }


  template <typename T, typename U, class CIn>
  VTKM_CONT static U Reduce(vtkm::cont::DeviceAdapterId devId,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue)
  {
    detail::ReduceFunctor<U> functor;
    vtkm::cont::TryExecuteOnDevice(devId, functor, input, initialValue);
    return functor.result;
  }
  template <typename T, typename U, class CIn>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    return Reduce(vtkm::cont::DeviceAdapterTagAny(), input, initialValue);
  }


  template <typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(vtkm::cont::DeviceAdapterId devId,
                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    detail::ReduceFunctor<U> functor;
    vtkm::cont::TryExecuteOnDevice(devId, functor, input, initialValue, binary_functor);
    return functor.result;
  }
  template <typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    return Reduce(vtkm::cont::DeviceAdapterTagAny(), input, initialValue, binary_functor);
  }


  template <typename T,
            typename U,
            class CKeyIn,
            class CValIn,
            class CKeyOut,
            class CValOut,
            class BinaryFunctor>
  VTKM_CONT static void ReduceByKey(vtkm::cont::DeviceAdapterId devId,
                                    const vtkm::cont::ArrayHandle<T, CKeyIn>& keys,
                                    const vtkm::cont::ArrayHandle<U, CValIn>& values,
                                    vtkm::cont::ArrayHandle<T, CKeyOut>& keys_output,
                                    vtkm::cont::ArrayHandle<U, CValOut>& values_output,
                                    BinaryFunctor binary_functor)
  {
    vtkm::cont::TryExecuteOnDevice(devId,
                                   detail::ReduceByKeyFunctor(),
                                   keys,
                                   values,
                                   keys_output,
                                   values_output,
                                   binary_functor);
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
    ReduceByKey(
      vtkm::cont::DeviceAdapterTagAny(), keys, values, keys_output, values_output, binary_functor);
  }


  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanInclusive(vtkm::cont::DeviceAdapterId devId,
                                   const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanInclusiveResultFunctor<T> functor;
    vtkm::cont::TryExecuteOnDevice(devId, functor, input, output);
    return functor.result;
  }
  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    return ScanInclusive(vtkm::cont::DeviceAdapterTagAny(), input, output);
  }


  template <typename T, class CIn, class COut>
  VTKM_CONT static T StreamingScanExclusive(vtkm::cont::DeviceAdapterId devId,
                                            const vtkm::Id numBlocks,
                                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                                            vtkm::cont::ArrayHandle<T, COut>& output)
  {
    detail::StreamingScanExclusiveFunctor<T> functor;
    vtkm::cont::TryExecuteOnDevice(devId, functor, numBlocks, input, output);
    return functor.result;
  }
  template <typename T, class CIn, class COut>
  VTKM_CONT static T StreamingScanExclusive(const vtkm::Id numBlocks,
                                            const vtkm::cont::ArrayHandle<T, CIn>& input,
                                            vtkm::cont::ArrayHandle<T, COut>& output)
  {
    return StreamingScanExclusive(vtkm::cont::DeviceAdapterTagAny(), numBlocks, input, output);
  }


  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanInclusive(vtkm::cont::DeviceAdapterId devId,
                                   const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor)
  {
    detail::ScanInclusiveResultFunctor<T> functor;
    vtkm::cont::TryExecuteOnDevice(devId, functor, input, output, binary_functor);
    return functor.result;
  }
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor)
  {
    return ScanInclusive(vtkm::cont::DeviceAdapterTagAny(), input, output, binary_functor);
  }


  template <typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  VTKM_CONT static void ScanInclusiveByKey(vtkm::cont::DeviceAdapterId devId,
                                           const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& values_output,
                                           BinaryFunctor binary_functor)
  {
    vtkm::cont::TryExecuteOnDevice(
      devId, detail::ScanInclusiveByKeyFunctor(), keys, values, values_output, binary_functor);
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
    ScanInclusiveByKey(
      vtkm::cont::DeviceAdapterTagAny(), keys, values, values_output, binary_functor);
  }


  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanInclusiveByKey(vtkm::cont::DeviceAdapterId devId,
                                           const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& values_output)
  {
    vtkm::cont::TryExecuteOnDevice(
      devId, detail::ScanInclusiveByKeyFunctor(), keys, values, values_output);
  }
  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanInclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& values_output)
  {
    ScanInclusiveByKey(vtkm::cont::DeviceAdapterTagAny(), keys, values, values_output);
  }


  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanExclusive(vtkm::cont::DeviceAdapterId devId,
                                   const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    detail::ScanExclusiveFunctor<T> functor;
    vtkm::cont::TryExecuteOnDevice(devId, functor, input, output);
    return functor.result;
  }
  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    return ScanExclusive(vtkm::cont::DeviceAdapterTagAny(), input, output);
  }


  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(vtkm::cont::DeviceAdapterId devId,
                                   const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor,
                                   const T& initialValue)
  {
    detail::ScanExclusiveFunctor<T> functor;
    vtkm::cont::TryExecuteOnDevice(devId, functor, input, output, binaryFunctor, initialValue);
    return functor.result;
  }
  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor,
                                   const T& initialValue)
  {
    return ScanExclusive(
      vtkm::cont::DeviceAdapterTagAny(), input, output, binaryFunctor, initialValue);
  }


  template <typename T, typename U, typename KIn, typename VIn, typename VOut, class BinaryFunctor>
  VTKM_CONT static void ScanExclusiveByKey(vtkm::cont::DeviceAdapterId devId,
                                           const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output,
                                           const U& initialValue,
                                           BinaryFunctor binaryFunctor)
  {
    vtkm::cont::TryExecuteOnDevice(devId,
                                   detail::ScanExclusiveByKeyFunctor(),
                                   keys,
                                   values,
                                   output,
                                   initialValue,
                                   binaryFunctor);
  }
  template <typename T, typename U, typename KIn, typename VIn, typename VOut, class BinaryFunctor>
  VTKM_CONT static void ScanExclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output,
                                           const U& initialValue,
                                           BinaryFunctor binaryFunctor)
  {
    ScanExclusiveByKey(
      vtkm::cont::DeviceAdapterTagAny(), keys, values, output, initialValue, binaryFunctor);
  }


  template <typename T, typename U, class KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanExclusiveByKey(vtkm::cont::DeviceAdapterId devId,
                                           const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output)
  {
    vtkm::cont::TryExecuteOnDevice(
      devId, detail::ScanExclusiveByKeyFunctor(), keys, values, output);
  }
  template <typename T, typename U, class KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanExclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output)
  {
    ScanExclusiveByKey(vtkm::cont::DeviceAdapterTagAny(), keys, values, output);
  }


  template <class Functor>
  VTKM_CONT static void Schedule(vtkm::cont::DeviceAdapterId devId,
                                 Functor functor,
                                 vtkm::Id numInstances)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::ScheduleFunctor(), functor, numInstances);
  }
  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id numInstances)
  {
    Schedule(vtkm::cont::DeviceAdapterTagAny(), functor, numInstances);
  }


  template <class Functor>
  VTKM_CONT static void Schedule(vtkm::cont::DeviceAdapterId devId,
                                 Functor functor,
                                 vtkm::Id3 rangeMax)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::ScheduleFunctor(), functor, rangeMax);
  }
  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id3 rangeMax)
  {
    Schedule(vtkm::cont::DeviceAdapterTagAny(), functor, rangeMax);
  }


  template <typename T, class Storage>
  VTKM_CONT static void Sort(vtkm::cont::DeviceAdapterId devId,
                             vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::SortFunctor(), values);
  }
  template <typename T, class Storage>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    Sort(vtkm::cont::DeviceAdapterTagAny(), values);
  }


  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Sort(vtkm::cont::DeviceAdapterId devId,
                             vtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::SortFunctor(), values, binary_compare);
  }
  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    Sort(vtkm::cont::DeviceAdapterTagAny(), values, binary_compare);
  }


  template <typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT static void SortByKey(vtkm::cont::DeviceAdapterId devId,
                                  vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::SortByKeyFunctor(), keys, values);
  }
  template <typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    SortByKey(vtkm::cont::DeviceAdapterTagAny(), keys, values);
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKey(vtkm::cont::DeviceAdapterId devId,
                                  vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::SortByKeyFunctor(), keys, values, binary_compare);
  }
  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    SortByKey(vtkm::cont::DeviceAdapterTagAny(), keys, values, binary_compare);
  }


  VTKM_CONT static void Synchronize(vtkm::cont::DeviceAdapterId devId)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::SynchronizeFunctor());
  }
  VTKM_CONT static void Synchronize() { Synchronize(vtkm::cont::DeviceAdapterTagAny()); }


  template <typename T,
            typename U,
            typename V,
            typename StorageT,
            typename StorageU,
            typename StorageV,
            typename BinaryFunctor>
  VTKM_CONT static void Transform(vtkm::cont::DeviceAdapterId devId,
                                  const vtkm::cont::ArrayHandle<T, StorageT>& input1,
                                  const vtkm::cont::ArrayHandle<U, StorageU>& input2,
                                  vtkm::cont::ArrayHandle<V, StorageV>& output,
                                  BinaryFunctor binaryFunctor)
  {
    vtkm::cont::TryExecuteOnDevice(
      devId, detail::TransformFunctor(), input1, input2, output, binaryFunctor);
  }
  template <typename T,
            typename U,
            typename V,
            typename StorageT,
            typename StorageU,
            typename StorageV,
            typename BinaryFunctor>
  VTKM_CONT static void Transform(const vtkm::cont::ArrayHandle<T, StorageT>& input1,
                                  const vtkm::cont::ArrayHandle<U, StorageU>& input2,
                                  vtkm::cont::ArrayHandle<V, StorageV>& output,
                                  BinaryFunctor binaryFunctor)
  {
    Transform(vtkm::cont::DeviceAdapterTagAny(), input1, input2, output, binaryFunctor);
  }


  template <typename T, class Storage>
  VTKM_CONT static void Unique(vtkm::cont::DeviceAdapterId devId,
                               vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::UniqueFunctor(), values);
  }
  template <typename T, class Storage>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    Unique(vtkm::cont::DeviceAdapterTagAny(), values);
  }


  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Unique(vtkm::cont::DeviceAdapterId devId,
                               vtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::UniqueFunctor(), values, binary_compare);
  }
  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    Unique(vtkm::cont::DeviceAdapterTagAny(), values, binary_compare);
  }


  template <typename T, class CIn, class CVal, class COut>
  VTKM_CONT static void UpperBounds(vtkm::cont::DeviceAdapterId devId,
                                    const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::UpperBoundsFunctor(), input, values, output);
  }
  template <typename T, class CIn, class CVal, class COut>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output)
  {
    UpperBounds(vtkm::cont::DeviceAdapterTagAny(), input, values, output);
  }


  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT static void UpperBounds(vtkm::cont::DeviceAdapterId devId,
                                    const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output,
                                    BinaryCompare binary_compare)
  {
    vtkm::cont::TryExecuteOnDevice(
      devId, detail::UpperBoundsFunctor(), input, values, output, binary_compare);
  }
  template <typename T, class CIn, class CVal, class COut, class BinaryCompare>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                    const vtkm::cont::ArrayHandle<T, CVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& output,
                                    BinaryCompare binary_compare)
  {
    UpperBounds(vtkm::cont::DeviceAdapterTagAny(), input, values, output, binary_compare);
  }


  template <class CIn, class COut>
  VTKM_CONT static void UpperBounds(vtkm::cont::DeviceAdapterId devId,
                                    const vtkm::cont::ArrayHandle<vtkm::Id, CIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& values_output)
  {
    vtkm::cont::TryExecuteOnDevice(devId, detail::UpperBoundsFunctor(), input, values_output);
  }
  template <class CIn, class COut>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<vtkm::Id, CIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& values_output)
  {
    UpperBounds(vtkm::cont::DeviceAdapterTagAny(), input, values_output);
  }
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_Algorithm_h
