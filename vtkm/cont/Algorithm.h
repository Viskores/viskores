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
#include <vtkm/cont/internal/DeviceAdapterTag.h>

namespace vtkm
{
namespace cont
{

namespace
{
struct CopyFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Copy(std::forward<Args>(args)...);
    return true;
  }
};

struct CopyIfFunctor
{

  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::CopyIf(std::forward<Args>(args)...);
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
    valid = vtkm::cont::DeviceAdapterAlgorithm<Device>::CopySubRange(std::forward<Args>(args)...);
    return true;
  }
};

struct LowerBoundsFunctor
{

  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::LowerBounds(std::forward<Args>(args)...);
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
    result = vtkm::cont::DeviceAdapterAlgorithm<Device>::Reduce(std::forward<Args>(args)...);
    return true;
  }
};

struct ReduceByKeyFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ReduceByKey(std::forward<Args>(args)...);
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
    result = vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusive(std::forward<Args>(args)...);
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
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanInclusiveByKey(std::forward<Args>(args)...);
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
    result = vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(std::forward<Args>(args)...);
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
    vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusiveByKey(std::forward<Args>(args)...);
    return true;
  }
};

struct ScheduleFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(std::forward<Args>(args)...);
    return true;
  }
};

struct SortFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Sort(std::forward<Args>(args)...);
    return true;
  }
};

struct SortByKeyFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::SortByKey(std::forward<Args>(args)...);
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
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Transform(std::forward<Args>(args)...);
    return true;
  }
};

struct UniqueFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Unique(std::forward<Args>(args)...);
    return true;
  }
};

struct UpperBoundsFunctor
{
  template <typename Device, typename... Args>
  VTKM_CONT bool operator()(Device, Args&&... args) const
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    vtkm::cont::DeviceAdapterAlgorithm<Device>::UpperBounds(std::forward<Args>(args)...);
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
    vtkm::cont::TryExecute(CopyIfFunctor(), input, stencil, output);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output,
                               UnaryPredicate unary_predicate)
  {
    vtkm::cont::TryExecute(CopyIfFunctor(), input, stencil, output, unary_predicate);
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
    vtkm::cont::TryExecute(LowerBoundsFunctor(), input, values, output, binary_compare);
  }

  template <class CIn, class COut>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<vtkm::Id, CIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& values_output)
  {
    vtkm::cont::TryExecute(LowerBoundsFunctor(), input, values_output);
  }

  template <typename T, typename U, class CIn>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    ReduceFunctor<U> functor;
    vtkm::cont::TryExecute(functor, input, initialValue);
    return functor.result;
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    ReduceFunctor<U> functor;
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
    ScanInclusiveResultFunctor<T> functor;
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
    ScanInclusiveResultFunctor<T> functor;
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
      ScanInclusiveByKeyFunctor(), keys, values, values_output, binary_functor);
  }

  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanInclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& values_output)
  {
    vtkm::cont::TryExecute(ScanInclusiveByKeyFunctor(), keys, values, values_output);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    ScanExclusiveFunctor<T> functor;
    vtkm::cont::TryExecute(functor, input, output);
    return functor.result;
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor,
                                   const T& initialValue)
  {
    ScanExclusiveFunctor<T> functor;
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
    vtkm::cont::TryExecute(
      ScanExclusiveByKeyFunctor(), keys, values, output, initialValue, binaryFunctor);
  }

  template <typename T, typename U, class KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanExclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output)
  {
    vtkm::cont::TryExecute(ScanExclusiveByKeyFunctor(), keys, values, output);
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id numInstances)
  {
    vtkm::cont::TryExecute(ScheduleFunctor(), functor, numInstances);
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id3 rangeMax)
  {
    vtkm::cont::TryExecute(ScheduleFunctor(), functor, rangeMax);
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
    vtkm::cont::TryExecute(SortFunctor(), values, binary_compare);
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
    vtkm::cont::TryExecute(SortByKeyFunctor(), keys, values, binary_compare);
  }

  VTKM_CONT static void Synchronize() { vtkm::cont::TryExecute(SynchronizeFunctor()); }

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
    vtkm::cont::TryExecute(TransformFunctor(), input1, input2, output, binaryFunctor);
  }

  template <typename T, class Storage>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    vtkm::cont::TryExecute(UniqueFunctor(), values);
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    vtkm::cont::TryExecute(UniqueFunctor(), values, binary_compare);
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
    vtkm::cont::TryExecute(UpperBoundsFunctor(), input, values, output, binary_compare);
  }

  template <class CIn, class COut>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<vtkm::Id, CIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, COut>& values_output)
  {
    vtkm::cont::TryExecute(UpperBoundsFunctor(), input, values_output);
  }
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_Algorithm_h
