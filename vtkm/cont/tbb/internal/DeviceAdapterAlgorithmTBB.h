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
#include <vtkm/exec/internal/ErrorMessageBuffer.h>

VTKM_THIRDPARTY_PRE_INCLUDE

#if  defined(VTKM_MSVC)
// TBB includes windows.h, which clobbers min and max functions so we
// define NOMINMAX to fix that problem. We also include WIN32_LEAN_AND_MEAN
// to reduce the number of macros and objects windows.h imports as those also
// can cause conflicts
// TBB's header include a #pragma comment(lib,"tbb.lib") line to make all
// consuming

#pragma push_macro("WIN32_LEAN_AND_MEAN")
#pragma push_macro("NOMINMAX")
#pragma push_macro("__TBB_NO_IMPLICITLINKAGE")
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define __TBB_NO_IMPLICIT_LINKAGE
#endif

#include <tbb/tbb_stddef.h>
#if (TBB_VERSION_MAJOR == 4) && (TBB_VERSION_MINOR == 2)
//we provide an patched implementation of tbb parallel_sort
//that fixes ADL for std::swap. This patch has been submitted to Intel
//and is fixed in TBB 4.2 update 2.
#include <vtkm/cont/tbb/internal/parallel_sort.h>
#else
#include <tbb/parallel_sort.h>
#endif

#include <tbb/blocked_range.h>
#include <tbb/blocked_range3d.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_scan.h>
#include <tbb/partitioner.h>
#include <tbb/tick_count.h>

#if defined(VTKM_MSVC)
#include <Windows.h>
#pragma pop_macro("WIN32_LEAN_AND_MEAN")
#pragma pop_macro("NOMINMAX")
#pragma pop_macro("__TBB_NO_IMPLICITLINKAGE")
#endif

VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace cont {

template<>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB> :
    vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
        DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>,
        vtkm::cont::DeviceAdapterTagTBB>
{
public:
  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut> &output)
  {
    return tbb::ScanInclusivePortals(
          input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
          output.PrepareForOutput(input.GetNumberOfValues(),
              vtkm::cont::DeviceAdapterTagTBB()), vtkm::internal::Add());
  }

  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut> &output,
      BinaryFunctor binary_functor)
  {
    return tbb::ScanInclusivePortals(
          input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
          output.PrepareForOutput(input.GetNumberOfValues(),
            vtkm::cont::DeviceAdapterTagTBB()), binary_functor);
  }

  template<typename T, class CIn, class COut>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut> &output)
  {
    return tbb::ScanExclusivePortals(
          input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
          output.PrepareForOutput(input.GetNumberOfValues(),
            vtkm::cont::DeviceAdapterTagTBB()),
          vtkm::internal::Add(), vtkm::TypeTraits<T>::ZeroInitialization());
  }

  template<typename T, class CIn, class COut, class BinaryFunctor>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,CIn> &input,
      vtkm::cont::ArrayHandle<T,COut> &output,
      BinaryFunctor binary_functor,
      const T& initialValue)
  {
    return tbb::ScanExclusivePortals(
          input.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
          output.PrepareForOutput(input.GetNumberOfValues(),
            vtkm::cont::DeviceAdapterTagTBB()), binary_functor, initialValue);
  }

  template<class FunctorType>
  VTKM_CONT_EXPORT
  static void Schedule(FunctorType functor, vtkm::Id numInstances)
  {
    const vtkm::Id MESSAGE_SIZE = 1024;
    char errorString[MESSAGE_SIZE];
    errorString[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer
        errorMessage(errorString, MESSAGE_SIZE);

    tbb::ScheduleKernel<FunctorType> kernel(functor);
    kernel.SetErrorMessageBuffer(errorMessage);

    ::tbb::blocked_range<vtkm::Id> range(0, numInstances, tbb::TBB_GRAIN_SIZE);

    ::tbb::parallel_for(range, kernel);

    if (errorMessage.IsErrorRaised())
      {
      throw vtkm::cont::ErrorExecution(errorString);
      }
  }

  template<class FunctorType>
  VTKM_CONT_EXPORT
  static void Schedule(FunctorType functor,
                       vtkm::Id3 rangeMax)
  {
    //we need to extract from the functor that uniform grid information
    const vtkm::Id MESSAGE_SIZE = 1024;
    char errorString[MESSAGE_SIZE];
    errorString[0] = '\0';
    vtkm::exec::internal::ErrorMessageBuffer
        errorMessage(errorString, MESSAGE_SIZE);

    //memory is generally setup in a way that iterating the first range
    //in the tightest loop has the best cache coherence.
    ::tbb::blocked_range3d<vtkm::Id> range(0, rangeMax[2],
                                           0, rangeMax[1],
                                           0, rangeMax[0]);

    tbb::ScheduleKernelId3<FunctorType> kernel(functor,rangeMax);
    kernel.SetErrorMessageBuffer(errorMessage);

    ::tbb::parallel_for(range, kernel);

    if (errorMessage.IsErrorRaised())
      {
      throw vtkm::cont::ErrorExecution(errorString);
      }
  }

  template<typename T, class Container>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Container> &values)
  {
    //this is required to get sort to work with zip handles
    std::less< T > lessOp;
    Sort(values, lessOp );
  }

  template<typename T, class Container, class BinaryCompare>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Container> &values, BinaryCompare binary_compare)
  {
    typedef typename vtkm::cont::ArrayHandle<T,Container>::template
      ExecutionTypes<vtkm::cont::DeviceAdapterTagTBB>::Portal PortalType;
    PortalType arrayPortal = values.PrepareForInPlace(
      vtkm::cont::DeviceAdapterTagTBB());

    typedef vtkm::cont::ArrayPortalToIterators<PortalType> IteratorsType;
    IteratorsType iterators(arrayPortal);

    internal::WrappedBinaryOperator<bool,BinaryCompare> wrappedCompare(binary_compare);
    ::tbb::parallel_sort(iterators.GetBegin(),
                         iterators.GetEnd(),
                         wrappedCompare);
  }

  template<typename T, typename U, class StorageT,  class StorageU>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT> &keys,
      vtkm::cont::ArrayHandle<U,StorageU> &values)
  {
    SortByKey(keys, values, std::less<T>());
  }

  template<typename T, typename U,
           class StorageT, class StorageU,
           class Compare>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT>& keys,
      vtkm::cont::ArrayHandle<U,StorageU>& values,
      Compare comp)
  {
    typedef vtkm::cont::ArrayHandle<T,StorageT> KeyType;
    if (sizeof(U) > sizeof(vtkm::Id))
    {
      /// More efficient sort:
      /// Move value indexes when sorting and reorder the value array at last

      typedef vtkm::cont::ArrayHandle<U,StorageU> ValueType;
      typedef vtkm::cont::ArrayHandle<vtkm::Id> IndexType;
      typedef vtkm::cont::ArrayHandleZip<KeyType,IndexType> ZipHandleType;

      IndexType indexArray;
      ValueType valuesScattered;
      const vtkm::Id size = values.GetNumberOfValues();

      Copy( ArrayHandleIndex(keys.GetNumberOfValues()), indexArray);

      ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys,indexArray);
      Sort(zipHandle,vtkm::cont::internal::KeyCompare<T,vtkm::Id,Compare>(comp));


      tbb::ScatterPortal(values.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
                    indexArray.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
                    valuesScattered.PrepareForOutput(size,vtkm::cont::DeviceAdapterTagTBB()));

      Copy( valuesScattered, values );
    }
    else
    {
      typedef vtkm::cont::ArrayHandle<U,StorageU> ValueType;
      typedef vtkm::cont::ArrayHandleZip<KeyType,ValueType> ZipHandleType;

      ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys,values);
      Sort(zipHandle,vtkm::cont::internal::KeyCompare<T,U,Compare>(comp));
    }
  }

  VTKM_CONT_EXPORT static void Synchronize()
  {
    // Nothing to do. This device schedules all of its operations using a
    // split/join paradigm. This means that the if the control threaad is
    // calling this method, then nothing should be running in the execution
    // environment.
  }

};

/// TBB contains its own high resolution timer.
///
template<>
class DeviceAdapterTimerImplementation<vtkm::cont::DeviceAdapterTagTBB>
{
public:
  VTKM_CONT_EXPORT DeviceAdapterTimerImplementation()
  {
    this->Reset();
  }
  VTKM_CONT_EXPORT void Reset()
  {
    vtkm::cont::DeviceAdapterAlgorithm<
        vtkm::cont::DeviceAdapterTagTBB>::Synchronize();
    this->StartTime = ::tbb::tick_count::now();
  }
  VTKM_CONT_EXPORT vtkm::Float64 GetElapsedTime()
  {
    vtkm::cont::DeviceAdapterAlgorithm<
        vtkm::cont::DeviceAdapterTagTBB>::Synchronize();
    ::tbb::tick_count currentTime = ::tbb::tick_count::now();
    ::tbb::tick_count::interval_t elapsedTime = currentTime - this->StartTime;
    return static_cast<vtkm::Float64>(elapsedTime.seconds());
  }

private:
  ::tbb::tick_count StartTime;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_tbb_internal_DeviceAdapterAlgorithmTBB_h
