//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_openmp_internal_DeviceAdapterAlgorithmOpenMP_h
#define vtk_m_cont_openmp_internal_DeviceAdapterAlgorithmOpenMP_h

#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Error.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>

#include <vtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>
#include <vtkm/cont/openmp/internal/FunctorsOpenMP.h>
#include <vtkm/cont/openmp/internal/ParallelScanOpenMP.h>
#include <vtkm/cont/openmp/internal/ParallelSortOpenMP.h>
#include <vtkm/exec/openmp/internal/TaskTilingOpenMP.h>

#include <omp.h>

#include <algorithm>
#include <type_traits>

namespace vtkm
{
namespace cont
{

template <>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagOpenMP>
  : vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagOpenMP>,
      vtkm::cont::DeviceAdapterTagOpenMP>
{
  using DevTag = DeviceAdapterTagOpenMP;

public:
  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static void Copy(const vtkm::cont::ArrayHandle<T, CIn>& input,
                             vtkm::cont::ArrayHandle<U, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    using namespace vtkm::cont::openmp;

    const vtkm::Id inSize = input.GetNumberOfValues();
    if (inSize == 0)
    {
      output.Allocate(0);
      return;
    }
    auto inputPortal = input.PrepareForInput(DevTag());
    auto outputPortal = output.PrepareForOutput(inSize, DevTag());
    CopyHelper(inputPortal, outputPortal, 0, 0, inSize);
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

    using namespace vtkm::cont::openmp;

    vtkm::Id inSize = input.GetNumberOfValues();
    if (inSize == 0)
    {
      output.Allocate(0);
      return;
    }
    auto inputPortal = input.PrepareForInput(DevTag());
    auto stencilPortal = stencil.PrepareForInput(DevTag());
    auto outputPortal = output.PrepareForOutput(inSize, DevTag());

    auto inIter = vtkm::cont::ArrayPortalToIteratorBegin(inputPortal);
    auto stencilIter = vtkm::cont::ArrayPortalToIteratorBegin(stencilPortal);
    auto outIter = vtkm::cont::ArrayPortalToIteratorBegin(outputPortal);

    CopyIfHelper helper;

    VTKM_OPENMP_DIRECTIVE(parallel default(shared))
    {

      VTKM_OPENMP_DIRECTIVE(single)
      {
        // Calls omp_get_num_threads, thus must be used inside a parallel section.
        helper.Initialize(inSize, sizeof(T));
      }

      VTKM_OPENMP_DIRECTIVE(for schedule(static))
      for (vtkm::Id i = 0; i < helper.NumChunks; ++i)
      {
        helper.CopyIf(inIter, stencilIter, outIter, unary_predicate, i);
      }
    }

    vtkm::Id numValues = helper.Reduce(outIter);
    output.Shrink(numValues);
  }


  template <typename T, typename U, class CIn, class COut>
  VTKM_CONT static bool CopySubRange(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                     vtkm::Id inputStartIndex,
                                     vtkm::Id numberOfValuesToCopy,
                                     vtkm::cont::ArrayHandle<U, COut>& output,
                                     vtkm::Id outputIndex = 0)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    using namespace vtkm::cont::openmp;

    const vtkm::Id inSize = input.GetNumberOfValues();

    // Check if the ranges overlap and fail if they do.
    if (input == output &&
        ((outputIndex >= inputStartIndex && outputIndex < inputStartIndex + numberOfValuesToCopy) ||
         (inputStartIndex >= outputIndex && inputStartIndex < outputIndex + numberOfValuesToCopy)))
    {
      return false;
    }

    if (inputStartIndex < 0 || numberOfValuesToCopy < 0 || outputIndex < 0 ||
        inputStartIndex >= inSize)
    { //invalid parameters
      return false;
    }

    //determine if the numberOfElementsToCopy needs to be reduced
    if (inSize < (inputStartIndex + numberOfValuesToCopy))
    { //adjust the size
      numberOfValuesToCopy = (inSize - inputStartIndex);
    }

    const vtkm::Id outSize = output.GetNumberOfValues();
    const vtkm::Id copyOutEnd = outputIndex + numberOfValuesToCopy;
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

    auto inputPortal = input.PrepareForInput(DevTag());
    auto outputPortal = output.PrepareForInPlace(DevTag());

    CopyHelper(inputPortal, outputPortal, inputStartIndex, outputIndex, numberOfValuesToCopy);

    return true;
  }

  template <typename T, typename U, class CIn>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input, U initialValue)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    return Reduce(input, initialValue, vtkm::Add());
  }

  template <typename T, typename U, class CIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, CIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    using namespace vtkm::cont::openmp;

    auto portal = input.PrepareForInput(DevTag());
    const OpenMPReductionSupported<typename std::decay<U>::type> fastPath;

    return ReduceHelper::Execute(portal, initialValue, binary_functor, fastPath);
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
                                    BinaryFunctor func)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    openmp::ReduceByKeyHelper(keys, values, keys_output, values_output, func);
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    return ScanInclusive(input, output, vtkm::Add());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    if (input.GetNumberOfValues() <= 0)
    {
      return vtkm::TypeTraits<T>::ZeroInitialization();
    }

    using InPortalT = decltype(input.PrepareForInput(DevTag()));
    using OutPortalT = decltype(output.PrepareForOutput(0, DevTag()));
    using Impl = openmp::ScanInclusiveHelper<InPortalT, OutPortalT, BinaryFunctor>;

    vtkm::Id numVals = input.GetNumberOfValues();
    Impl impl(
      input.PrepareForInput(DevTag()), output.PrepareForOutput(numVals, DevTag()), binaryFunctor);

    return impl.Execute(vtkm::Id2(0, numVals));
  }

  template <typename T, class CIn, class COut>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    return ScanExclusive(input, output, vtkm::Add(), vtkm::TypeTraits<T>::ZeroInitialization());
  }

  template <typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, CIn>& input,
                                   vtkm::cont::ArrayHandle<T, COut>& output,
                                   BinaryFunctor binaryFunctor,
                                   const T& initialValue)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    if (input.GetNumberOfValues() <= 0)
    {
      return initialValue;
    }

    using InPortalT = decltype(input.PrepareForInput(DevTag()));
    using OutPortalT = decltype(output.PrepareForOutput(0, DevTag()));
    using Impl = openmp::ScanExclusiveHelper<InPortalT, OutPortalT, BinaryFunctor>;

    vtkm::Id numVals = input.GetNumberOfValues();
    Impl impl(input.PrepareForInput(DevTag()),
              output.PrepareForOutput(numVals, DevTag()),
              binaryFunctor,
              initialValue);

    return impl.Execute(vtkm::Id2(0, numVals));
  }

  /// \brief Unstable ascending sort of input array.
  ///
  /// Sorts the contents of \c values so that they in ascending value. Doesn't
  /// guarantee stability
  ///
  template <typename T, class Storage>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    Sort(values, vtkm::SortLess());
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    openmp::sort::parallel_sort(values, binary_compare);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    SortByKey(keys, values, std::less<T>());
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    openmp::sort::parallel_sort_bykey(keys, values, binary_compare);
  }

  template <typename T, class Storage>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    Unique(values, std::equal_to<T>());
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    auto portal = values.PrepareForInPlace(DevTag());
    auto iter = vtkm::cont::ArrayPortalToIteratorBegin(portal);

    using IterT = typename std::decay<decltype(iter)>::type;
    using Uniqifier = openmp::UniqueHelper<IterT, BinaryCompare>;

    Uniqifier uniquifier(iter, portal.GetNumberOfValues(), binary_compare);
    vtkm::Id outSize = uniquifier.Execute();
    values.Shrink(outSize);
  }

  VTKM_CONT_EXPORT static void ScheduleTask(vtkm::exec::openmp::internal::TaskTiling1D& functor,
                                            vtkm::Id size);
  VTKM_CONT_EXPORT static void ScheduleTask(vtkm::exec::openmp::internal::TaskTiling3D& functor,
                                            vtkm::Id3 size);

  template <class FunctorType>
  VTKM_CONT static inline void Schedule(FunctorType functor, vtkm::Id numInstances)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::exec::openmp::internal::TaskTiling1D kernel(functor);
    ScheduleTask(kernel, numInstances);
  }

  template <class FunctorType>
  VTKM_CONT static inline void Schedule(FunctorType functor, vtkm::Id3 rangeMax)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::exec::openmp::internal::TaskTiling3D kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

  VTKM_CONT static void Synchronize()
  {
    // Nothing to do. This device schedules all of its operations using a
    // split/join paradigm. This means that the if the control thread is
    // calling this method, then nothing should be running in the execution
    // environment.
  }
};

template <>
class DeviceTaskTypes<vtkm::cont::DeviceAdapterTagOpenMP>
{
public:
  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::openmp::internal::TaskTiling1D MakeTask(const WorkletType& worklet,
                                                             const InvocationType& invocation,
                                                             vtkm::Id,
                                                             vtkm::Id globalIndexOffset = 0)
  {
    return vtkm::exec::openmp::internal::TaskTiling1D(worklet, invocation, globalIndexOffset);
  }

  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::openmp::internal::TaskTiling3D MakeTask(const WorkletType& worklet,
                                                             const InvocationType& invocation,
                                                             vtkm::Id3,
                                                             vtkm::Id globalIndexOffset = 0)
  {
    return vtkm::exec::openmp::internal::TaskTiling3D(worklet, invocation, globalIndexOffset);
  }
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_openmp_internal_DeviceAdapterAlgorithmOpenMP_h
