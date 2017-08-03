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
#ifndef vtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
#define vtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <vtkm/cont/internal/IteratorFromArrayPortal.h>
#include <vtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>
#include <vtkm/cont/tbb/internal/FunctorsTBB.h>

#include <vtkm/exec/tbb/internal/TaskTiling.h>

namespace vtkm
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>
  : vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>,
      vtkm::cont::DeviceAdapterTagTBB>
{
public:
  template <typename T, typename U, class CIn>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    return Reduce(input, initialValue, vtkm::Add());
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    return tbb::ReducePortals(
      input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()), initialValue, binary_functor);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    return tbb::ScanInclusivePortals(
      input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
      output.PrepareForOutput(input.GetNumberOfValues(), vtkm::cont::DeviceAdapterTagTBB()),
      vtkm::Add());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor)
  {
    return tbb::ScanInclusivePortals(
      input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
      output.PrepareForOutput(input.GetNumberOfValues(), vtkm::cont::DeviceAdapterTagTBB()),
      binary_functor);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    return tbb::ScanExclusivePortals(
      input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
      output.PrepareForOutput(input.GetNumberOfValues(), vtkm::cont::DeviceAdapterTagTBB()),
      vtkm::Add(),
      vtkm::TypeTraits<T>::ZeroInitialization());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor,
                                   const T& initialValue)
  {
    return tbb::ScanExclusivePortals(
      input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
      output.PrepareForOutput(input.GetNumberOfValues(), vtkm::cont::DeviceAdapterTagTBB()),
      binary_functor,
      initialValue);
  }

  VTKM_CONT_EXPORT static void ScheduleTask(vtkm::exec::tbb::internal::TaskTiling1D& functor,
                                            vtkm::Id size);
  VTKM_CONT_EXPORT static void ScheduleTask(vtkm::exec::tbb::internal::TaskTiling3D& functor,
                                            vtkm::Id3 size);

  template <class FunctorType>
  VTKM_CONT static inline void Schedule(FunctorType functor, vtkm::Id numInstances)
  {
    vtkm::exec::tbb::internal::TaskTiling1D kernel(functor);
    ScheduleTask(kernel, numInstances);
  }

  template <class FunctorType>
  VTKM_CONT static inline void Schedule(FunctorType functor, vtkm::Id3 rangeMax)
  {
    vtkm::exec::tbb::internal::TaskTiling3D kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

  template <typename T, class Container>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Container>& values)
  {
    //this is required to get sort to work with zip handles
    std::less<T> lessOp;
    Sort(values, lessOp);
  }

  template <typename T, class Container, class BinaryCompare>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Container>& values,
                             BinaryCompare binary_compare)
  {
    typedef typename vtkm::cont::ArrayHandle<T, Container>::template ExecutionTypes<
      vtkm::cont::DeviceAdapterTagTBB>::Portal PortalType;
    PortalType arrayPortal = values.PrepareForInPlace(vtkm::cont::DeviceAdapterTagTBB());

    typedef vtkm::cont::ArrayPortalToIterators<PortalType> IteratorsType;
    IteratorsType iterators(arrayPortal);

    internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
    ::tbb::parallel_sort(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    SortByKey(keys, values, std::less<T>());
  }

  template <typename T, typename U, class StorageT, class StorageU, class Compare>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values,
                                  Compare comp)
  {
    typedef vtkm::cont::ArrayHandle<T, StorageT> KeyType;
    if (sizeof(U) > sizeof(vtkm::Id))
    {
      /// More efficient sort:
      /// Move value indexes when sorting and reorder the value array at last

      typedef vtkm::cont::ArrayHandle<U, StorageU> ValueType;
      typedef vtkm::cont::ArrayHandle<vtkm::Id> IndexType;
      typedef vtkm::cont::ArrayHandleZip<KeyType, IndexType> ZipHandleType;

      IndexType indexArray;
      ValueType valuesScattered;
      const vtkm::Id size = values.GetNumberOfValues();

      Copy(ArrayHandleIndex(keys.GetNumberOfValues()), indexArray);

      ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys, indexArray);
      Sort(zipHandle, vtkm::cont::internal::KeyCompare<T, vtkm::Id, Compare>(comp));

      tbb::ScatterPortal(values.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
                         indexArray.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
                         valuesScattered.PrepareForOutput(size, vtkm::cont::DeviceAdapterTagTBB()));

      Copy(valuesScattered, values);
    }
    else
    {
      typedef vtkm::cont::ArrayHandle<U, StorageU> ValueType;
      typedef vtkm::cont::ArrayHandleZip<KeyType, ValueType> ZipHandleType;

      ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys, values);
      Sort(zipHandle, vtkm::cont::internal::KeyCompare<T, U, Compare>(comp));
    }
  }

  VTKM_CONT static void Synchronize()
  {
    // Nothing to do. This device schedules all of its operations using a
    // split/join paradigm. This means that the if the control threaad is
    // calling this method, then nothing should be running in the execution
    // environment.
  }
};

/// TBB contains its own high resolution timer.
///
template <>
class DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagTBB>
{
public:
  VTKM_CONT DeviceAdapterTimerImplementation() { this->Reset(); }
  VTKM_CONT void Reset()
  {
    vtkm::cont::DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>::Synchronize();
    this->StartTime = ::tbb::tick_count::now();
  }
  VTKM_CONT vtkm::Float64 GetElapsedTime()
  {
    vtkm::cont::DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>::Synchronize();
    ::tbb::tick_count currentTime = ::tbb::tick_count::now();
    ::tbb::tick_count::interval_t elapsedTime = currentTime - this->StartTime;
    return static_cast<vtkm::Float64>(elapsedTime.seconds());
  }

private:
  ::tbb::tick_count StartTime;
};

template <>
class DeviceTaskTypes<vtkm::cont::DeviceAdapterTagTBB>
{
public:
  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::serial::internal::TaskTiling1D MakeTask(const WorkletType& worklet,
                                                             const InvocationType& invocation,
                                                             vtkm::Id,
                                                             vtkm::Id globalIndexOffset = 0)
  {
    return vtkm::exec::tbb::internal::TaskTiling1D(worklet, invocation, globalIndexOffset);
  }

  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::serial::internal::TaskTiling3D MakeTask(const WorkletType& worklet,
                                                             const InvocationType& invocation,
                                                             vtkm::Id3,
                                                             vtkm::Id globalIndexOffset = 0)
  {
    return vtkm::exec::tbb::internal::TaskTiling3D(worklet, invocation, globalIndexOffset);
  }
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
