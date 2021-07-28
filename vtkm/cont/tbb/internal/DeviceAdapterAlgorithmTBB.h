//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
#define vtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>
#include <vtkm/cont/internal/IteratorFromArrayPortal.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>
#include <vtkm/cont/tbb/internal/FunctorsTBB.h>
#include <vtkm/cont/tbb/internal/ParallelSortTBB.h>

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
  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static void Copy(const vtkm::cont::ArrayHandle<T, CIn>& input,
                             vtkm::cont::ArrayHandle<U, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::Token token;

    const vtkm::Id inSize = input.GetNumberOfValues();
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagTBB(), token);
    auto outputPortal = output.PrepareForOutput(inSize, DeviceAdapterTagTBB(), token);

    tbb::CopyPortals(inputPortal, outputPortal, 0, 0, inSize);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    ::vtkm::NotZeroInitialized unary_predicate;
    CopyIf(input, stencil, output, unary_predicate);
  }

  template <typename T, typename U, class CIn, class CStencil, class COut, class UnaryPredicate>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<T, CIn>& input,
                               const vtkm::cont::ArrayHandle<U, CStencil>& stencil,
                               vtkm::cont::ArrayHandle<T, COut>& output,
                               UnaryPredicate unary_predicate)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::Token token;

    vtkm::Id inputSize = input.GetNumberOfValues();
    VTKM_ASSERT(inputSize == stencil.GetNumberOfValues());
    vtkm::Id outputSize =
      tbb::CopyIfPortals(input.PrepareForInput(DeviceAdapterTagTBB(), token),
                         stencil.PrepareForInput(DeviceAdapterTagTBB(), token),
                         output.PrepareForOutput(inputSize, DeviceAdapterTagTBB(), token),
                         unary_predicate);
    token.DetachFromAll();
    output.Allocate(outputSize, vtkm::CopyFlag::On);
  }

  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static bool CopySubRange(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                     vtkm::Id inputStartIndex,
                                     vtkm::Id numberOfElementsToCopy,
                                     vtkm::cont::ArrayHandle<U, COut>& output,
                                     vtkm::Id outputIndex = 0)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id inSize = input.GetNumberOfValues();

    // Check if the ranges overlap and fail if they do.
    if (input == output &&
        ((outputIndex >= inputStartIndex &&
          outputIndex < inputStartIndex + numberOfElementsToCopy) ||
         (inputStartIndex >= outputIndex &&
          inputStartIndex < outputIndex + numberOfElementsToCopy)))
    {
      return false;
    }

    if (inputStartIndex < 0 || numberOfElementsToCopy < 0 || outputIndex < 0 ||
        inputStartIndex >= inSize)
    { //invalid parameters
      return false;
    }

    //determine if the numberOfElementsToCopy needs to be reduced
    if (inSize < (inputStartIndex + numberOfElementsToCopy))
    { //adjust the size
      numberOfElementsToCopy = (inSize - inputStartIndex);
    }

    const vtkm::Id outSize = output.GetNumberOfValues();
    const vtkm::Id copyOutEnd = outputIndex + numberOfElementsToCopy;
    if (outSize < copyOutEnd)
    { //output is not large enough
      if (outSize == 0)
      { //since output has nothing, just need to allocate to correct length
        output.Allocate(copyOutEnd);
      }
      else
      { //we currently have data in this array, so preserve it in the new
        //resized array
        vtkm::cont::ArrayHandle<U, COut> temp;
        temp.Allocate(copyOutEnd);
        CopySubRange(output, 0, outSize, temp);
        output = temp;
      }
    }

    vtkm::cont::Token token;
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagTBB(), token);
    auto outputPortal = output.PrepareForInPlace(DeviceAdapterTagTBB(), token);

    tbb::CopyPortals(
      inputPortal, outputPortal, inputStartIndex, outputIndex, numberOfElementsToCopy);

    return true;
  }

  template <typename T, typename U, class CIn>
  VTKM_CONT static auto Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
    -> decltype(Reduce(input, initialValue, vtkm::Add{}))
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    return Reduce(input, initialValue, vtkm::Add());
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static auto Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input,
                               U initialValue,
                               BinaryFunctor binary_functor)
    -> decltype(tbb::ReducePortals(input.ReadPortal(), initialValue, binary_functor))
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::Token token;
    return tbb::ReducePortals(input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB(), token),
                              initialValue,
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
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::Token token;

    vtkm::Id inputSize = keys.GetNumberOfValues();
    VTKM_ASSERT(inputSize == values.GetNumberOfValues());
    vtkm::Id outputSize = tbb::ReduceByKeyPortals(
      keys.PrepareForInput(DeviceAdapterTagTBB(), token),
      values.PrepareForInput(DeviceAdapterTagTBB(), token),
      keys_output.PrepareForOutput(inputSize, DeviceAdapterTagTBB(), token),
      values_output.PrepareForOutput(inputSize, DeviceAdapterTagTBB(), token),
      binary_functor);
    token.DetachFromAll();
    keys_output.Allocate(outputSize, vtkm::CopyFlag::On);
    values_output.Allocate(outputSize, vtkm::CopyFlag::On);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::Token token;
    return tbb::ScanInclusivePortals(
      input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB(), token),
      output.PrepareForOutput(input.GetNumberOfValues(), vtkm::cont::DeviceAdapterTagTBB(), token),
      vtkm::Add());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::Token token;
    return tbb::ScanInclusivePortals(
      input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB(), token),
      output.PrepareForOutput(input.GetNumberOfValues(), vtkm::cont::DeviceAdapterTagTBB(), token),
      binary_functor);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::Token token;
    return tbb::ScanExclusivePortals(
      input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB(), token),
      output.PrepareForOutput(input.GetNumberOfValues(), vtkm::cont::DeviceAdapterTagTBB(), token),
      vtkm::Add(),
      vtkm::TypeTraits<T>::ZeroInitialization());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binary_functor,
                                   const T& initialValue)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::Token token;
    return tbb::ScanExclusivePortals(
      input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB(), token),
      output.PrepareForOutput(input.GetNumberOfValues(), vtkm::cont::DeviceAdapterTagTBB(), token),
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
    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                   "Schedule TBB 1D: '%s'",
                   vtkm::cont::TypeToString(functor).c_str());

    vtkm::exec::tbb::internal::TaskTiling1D kernel(functor);
    ScheduleTask(kernel, numInstances);
  }

  template <class FunctorType>
  VTKM_CONT static inline void Schedule(FunctorType functor, vtkm::Id3 rangeMax)
  {
    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf,
                   "Schedule TBB 3D: '%s'",
                   vtkm::cont::TypeToString(functor).c_str());

    vtkm::exec::tbb::internal::TaskTiling3D kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

  //1. We need functions for each of the following


  template <typename T, class Container>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Container>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    //this is required to get sort to work with zip handles
    std::less<T> lessOp;
    vtkm::cont::tbb::sort::parallel_sort(values, lessOp);
  }

  template <typename T, class Container, class BinaryCompare>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Container>& values,
                             BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::tbb::sort::parallel_sort(values, binary_compare);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::tbb::sort::parallel_sort_bykey(keys, values, std::less<T>());
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::cont::tbb::sort::parallel_sort_bykey(keys, values, binary_compare);
  }

  template <typename T, class Storage>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    Unique(values, std::equal_to<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id outputSize;
    {
      vtkm::cont::Token token;
      outputSize =
        tbb::UniquePortals(values.PrepareForInPlace(DeviceAdapterTagTBB(), token), binary_compare);
    }
    values.Allocate(outputSize, vtkm::CopyFlag::On);
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
    this->StartReady = false;
    this->StopReady = false;
  }

  VTKM_CONT void Start()
  {
    this->Reset();
    this->StartTime = this->GetCurrentTime();
    this->StartReady = true;
  }

  VTKM_CONT void Stop()
  {
    this->StopTime = this->GetCurrentTime();
    this->StopReady = true;
  }

  VTKM_CONT bool Started() const { return this->StartReady; }

  VTKM_CONT bool Stopped() const { return this->StopReady; }

  VTKM_CONT bool Ready() const { return true; }

  VTKM_CONT vtkm::Float64 GetElapsedTime() const
  {
    assert(this->StartReady);
    if (!this->StartReady)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Error,
                 "Start() function should be called first then trying to call Stop() and"
                 " GetElapsedTime().");
      return 0;
    }

    ::tbb::tick_count startTime = this->StartTime;
    ::tbb::tick_count stopTime = this->StopReady ? this->StopTime : this->GetCurrentTime();

    ::tbb::tick_count::interval_t elapsedTime = stopTime - startTime;

    return static_cast<vtkm::Float64>(elapsedTime.seconds());
  }

  VTKM_CONT::tbb::tick_count GetCurrentTime() const
  {
    vtkm::cont::DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>::Synchronize();
    return ::tbb::tick_count::now();
  }

private:
  bool StartReady;
  bool StopReady;
  ::tbb::tick_count StartTime;
  ::tbb::tick_count StopTime;
};

template <>
class DeviceTaskTypes<vtkm::cont::DeviceAdapterTagTBB>
{
public:
  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::tbb::internal::TaskTiling1D MakeTask(WorkletType& worklet,
                                                          InvocationType& invocation,
                                                          vtkm::Id)
  {
    return vtkm::exec::tbb::internal::TaskTiling1D(worklet, invocation);
  }

  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::tbb::internal::TaskTiling3D MakeTask(WorkletType& worklet,
                                                          InvocationType& invocation,
                                                          vtkm::Id3)
  {
    return vtkm::exec::tbb::internal::TaskTiling3D(worklet, invocation);
  }
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
