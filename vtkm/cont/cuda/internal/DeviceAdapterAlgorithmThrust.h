
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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterThrust_h
#define vtk_m_cont_cuda_internal_DeviceAdapterThrust_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorExecution.h>

#include <vtkm/cont/cuda/internal/MakeThrustIterator.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>
#include <vtkm/exec/internal/WorkletInvokeFunctor.h>

// Disable GCC warnings we check vtkmfor but Thrust does not.
#if defined(__GNUC__) && !defined(VTKM_CUDA)
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif // gcc version >= 4.6
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 2)
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif // gcc version >= 4.2
#endif // gcc && !CUDA

#include <thrust/advance.h>
#include <thrust/binary_search.h>
#include <thrust/copy.h>
#include <thrust/count.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/unique.h>
#include <thrust/system/cuda/vector.h>

#include <thrust/iterator/counting_iterator.h>

#if defined(__GNUC__) && !defined(VTKM_CUDA)
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif // gcc version >= 4.6
#endif // gcc && !CUDA

namespace vtkm {
namespace cont {
namespace cuda {
namespace internal {

template<class FunctorType>
__global__
void Schedule1DIndexKernel(FunctorType functor, vtkm::Id length)
{
  const int idx = blockDim.x * blockIdx.x + threadIdx.x;
  if(idx < length)
    {
    functor(idx);
    }
}

template<class FunctorType>
__global__
void Schedule3DIndexKernel(FunctorType functor, vtkm::Id3 size)
{
  const vtkm::Id x = blockIdx.x*blockDim.x + threadIdx.x;
  const vtkm::Id y = blockIdx.y*blockDim.y + threadIdx.y;
  const vtkm::Id z = blockIdx.z*blockDim.z + threadIdx.z;

  if (x >= size[0] || y >= size[1] || z >= size[2])
    {
    return;
    }

  //now convert back to flat memory
  const int idx = x + size[0]*(y + size[1]*z);
  functor( idx );
}

inline
void compute_block_size(vtkm::Id3 rangeMax, dim3 blockSize3d, dim3& gridSize3d)
{
  gridSize3d.x = (rangeMax[0] % blockSize3d.x != 0) ? (rangeMax[0] / blockSize3d.x + 1) : (rangeMax[0] / blockSize3d.x);
  gridSize3d.y = (rangeMax[1] % blockSize3d.y != 1) ? (rangeMax[1] / blockSize3d.y + 1) : (rangeMax[1] / blockSize3d.y);
  gridSize3d.z = (rangeMax[2] % blockSize3d.z != 2) ? (rangeMax[2] / blockSize3d.z + 1) : (rangeMax[2] / blockSize3d.z);
}

class PerfRecord
{
public:

  PerfRecord(float elapsedT, dim3 block ):
    elapsedTime(elapsedT),
    blockSize(block)
    {

    }

  bool operator<(const PerfRecord& other) const
    { return elapsedTime < other.elapsedTime; }

  float elapsedTime;
  dim3 blockSize;
};

template<class Functor>
static void compare_3d_schedule_patterns(Functor functor, const vtkm::Id3& rangeMax)
{
  std::vector< PerfRecord > results;
  int indexTable[16] = {1, 2, 4, 8, 12, 16, 20, 24, 28, 30, 32, 64, 128, 256, 512, 1024};

  for(int i=0; i < 16; i++)
    {
    for(int j=0; j < 16; j++)
      {
      for(int k=0; k < 16; k++)
        {
        cudaEvent_t start, stop;
        cudaEventCreate(&start);
        cudaEventCreate(&stop);

        dim3 blockSize3d(indexTable[i],indexTable[j],indexTable[k]);
        dim3 gridSize3d;

        if( (blockSize3d.x * blockSize3d.y * blockSize3d.z) >= 1024 ||
            (blockSize3d.x * blockSize3d.y * blockSize3d.z) <=  4 ||
            blockSize3d.z >= 64)
          {
          //cuda can't handle more than 1024 threads per block
          //so don't try if we compute higher than that

          //also don't try stupidly low numbers

          //cuda can't handle more than 64 threads in the z direction
          continue;
          }

        compute_block_size(rangeMax, blockSize3d, gridSize3d);
        cudaEventRecord(start, 0);
        Schedule3DIndexKernel<Functor> <<<gridSize3d, blockSize3d>>> (functor, rangeMax);
        cudaEventRecord(stop, 0);

        cudaEventSynchronize(stop);
        float elapsedTimeMilliseconds;
        cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop);

        cudaEventDestroy(start);
        cudaEventDestroy(stop);

        PerfRecord record(elapsedTimeMilliseconds, blockSize3d);
        results.push_back( record );
      }
    }
  }

  std::sort(results.begin(), results.end());
  for(int i=results.size()-1; i >= 0; --i)
    {
    int x = results[i].blockSize.x;
    int y = results[i].blockSize.y;
    int z = results[i].blockSize.z;
    float t = results[i].elapsedTime;

    std::cout << "BlockSize of: " << x << "," << y << ", " << z << " required: " << t << std::endl;
    }

  std::cout << "flat array performance " << std::endl;
  {
  cudaEvent_t start, stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);

  const vtkm::Id numInstances = rangeMax[0] * rangeMax[1] * rangeMax[2];
  const int blockSize = 128;
  const int blocksPerGrid = (numInstances + blockSize - 1) / blockSize;

  cudaEventRecord(start, 0);
  Schedule1DIndexKernel<Functor> <<<blocksPerGrid, blockSize>>> (functor, numInstances);
  cudaEventRecord(stop, 0);

  cudaEventSynchronize(stop);
  float elapsedTimeMilliseconds;
  cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop);

  cudaEventDestroy(start);
  cudaEventDestroy(stop);

  std::cout << "Flat index required: " << elapsedTimeMilliseconds << std::endl;
  }

  std::cout << "fixed 3d block size performance " << std::endl;
  {
  cudaEvent_t start, stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);

  dim3 blockSize3d(32,4,1);
  dim3 gridSize3d;

  compute_block_size(rangeMax, blockSize3d, gridSize3d);
  cudaEventRecord(start, 0);
  Schedule3DIndexKernel<Functor> <<<gridSize3d, blockSize3d>>> (functor, rangeMax);
  cudaEventRecord(stop, 0);

  cudaEventSynchronize(stop);
  float elapsedTimeMilliseconds;
  cudaEventElapsedTime(&elapsedTimeMilliseconds, start, stop);

  cudaEventDestroy(start);
  cudaEventDestroy(stop);

  std::cout << "BlockSize of: " << blockSize3d.x << "," << blockSize3d.y << "," << blockSize3d.z << " required: " << elapsedTimeMilliseconds << std::endl;
  std::cout << "GridSize of: " << gridSize3d.x << "," << gridSize3d.y << "," << gridSize3d.z << " required: " << elapsedTimeMilliseconds << std::endl;
  }
}


/// This class can be subclassed to implement the DeviceAdapterAlgorithm for a
/// device that uses thrust as its implementation. The subclass should pass in
/// the correct device adapter tag as the template parameter.
///
template<class DeviceAdapterTag>
struct DeviceAdapterAlgorithmThrust
{
  // Because of some funny code conversions in nvcc, kernels for devices have to
  // be public.
  #ifndef VTKM_CUDA
private:
  #endif
  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static void CopyPortal(const InputPortal &input,
                                         const OutputPortal &output)
  {
    ::thrust::copy(IteratorBegin(input),
                   IteratorEnd(input),
                   IteratorBegin(output));
  }

  template<class InputPortal, class ValuesPortal, class OutputPortal>
  VTKM_CONT_EXPORT static void LowerBoundsPortal(const InputPortal &input,
                                                const ValuesPortal &values,
                                                const OutputPortal &output)
  {
    ::thrust::lower_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output));
  }

  template<class InputPortal, class ValuesPortal, class OutputPortal,
           class Compare>
  VTKM_CONT_EXPORT static void LowerBoundsPortal(const InputPortal &input,
                                                const ValuesPortal &values,
                                                const OutputPortal &output,
                                                Compare comp)
  {
    ::thrust::lower_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output),
                          comp);
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  void LowerBoundsPortal(const InputPortal &input,
                         const OutputPortal &values_output)
  {
    ::thrust::lower_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values_output),
                          IteratorEnd(values_output),
                          IteratorBegin(values_output));
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  typename InputPortal::ValueType ScanExclusivePortal(const InputPortal &input,
                                                      const OutputPortal &output)
  {
    // Use iterator to get value so that thrust device_ptr has chance to handle
    // data on device.
    typename InputPortal::ValueType inputEnd = *(IteratorEnd(input) - 1);

    ::thrust::exclusive_scan(IteratorBegin(input),
                             IteratorEnd(input),
                             IteratorBegin(output));

    //return the value at the last index in the array, as that is the sum
    return *(IteratorEnd(output) - 1) + inputEnd;
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  typename InputPortal::ValueType ScanInclusivePortal(const InputPortal &input,
                                                      const OutputPortal &output)
  {
    ::thrust::inclusive_scan(IteratorBegin(input),
                             IteratorEnd(input),
                             IteratorBegin(output));

    //return the value at the last index in the array, as that is the sum
    return *(IteratorEnd(output) - 1);
  }

  template<class ValuesPortal>
  VTKM_CONT_EXPORT static void SortPortal(const ValuesPortal &values)
  {
    ::thrust::sort(IteratorBegin(values),
                   IteratorEnd(values));
  }

  template<class ValuesPortal, class Compare>
  VTKM_CONT_EXPORT static void SortPortal(const ValuesPortal &values,
                                         Compare comp)
  {
    ::thrust::sort(IteratorBegin(values),
                   IteratorEnd(values),
                   comp);
  }


  template<class KeysPortal, class ValuesPortal>
  VTKM_CONT_EXPORT static void SortByKeyPortal(const KeysPortal &keys,
                                              const ValuesPortal &values)
  {
    ::thrust::sort_by_key(IteratorBegin(keys),
                          IteratorEnd(keys),
                          IteratorBegin(values));
  }

  template<class KeysPortal, class ValuesPortal, class Compare>
  VTKM_CONT_EXPORT static void SortByKeyPortal(const KeysPortal &keys,
                                              const ValuesPortal &values,
                                              Compare comp)
  {
    ::thrust::sort_by_key(IteratorBegin(keys),
                          IteratorEnd(keys),
                          IteratorBegin(values),
                          comp);
  }



  template<class StencilPortal>
  VTKM_CONT_EXPORT static vtkm::Id CountIfPortal(const StencilPortal &stencil)
  {
    typedef typename StencilPortal::ValueType ValueType;
    return ::thrust::count_if(IteratorBegin(stencil),
                              IteratorEnd(stencil),
                              ::vtkm::not_default_constructor<ValueType>());
  }

  template<class ValueIterator,
           class StencilPortal,
           class OutputPortal>
  VTKM_CONT_EXPORT static void CopyIfPortal(ValueIterator valuesBegin,
                                           ValueIterator valuesEnd,
                                           const StencilPortal &stencil,
                                           const OutputPortal &output)
  {
    typedef typename StencilPortal::ValueType ValueType;
    ::thrust::copy_if(valuesBegin,
                      valuesEnd,
                      IteratorBegin(stencil),
                      IteratorBegin(output),
                      ::vtkm::not_default_constructor<ValueType>());
  }

  template<class ValueIterator,
           class StencilArrayHandle,
           class OutputArrayHandle>
  VTKM_CONT_EXPORT static void RemoveIf(ValueIterator valuesBegin,
                                       ValueIterator valuesEnd,
                                       const StencilArrayHandle& stencil,
                                       OutputArrayHandle& output)
  {
    vtkm::Id numLeft = CountIfPortal(stencil.PrepareForInput(DeviceAdapterTag()));

    CopyIfPortal(valuesBegin,
                 valuesEnd,
                 stencil.PrepareForInput(DeviceAdapterTag()),
                 output.PrepareForOutput(numLeft, DeviceAdapterTag()));
  }

  template<class InputPortal,
           class StencilArrayHandle,
           class OutputArrayHandle>
  VTKM_CONT_EXPORT static
  void StreamCompactPortal(const InputPortal& inputPortal,
                           const StencilArrayHandle &stencil,
                           OutputArrayHandle& output)
  {
    RemoveIf(IteratorBegin(inputPortal),
             IteratorEnd(inputPortal),
             stencil,
             output);
  }

  template<class ValuesPortal>
  VTKM_CONT_EXPORT static
  vtkm::Id UniquePortal(const ValuesPortal values)
  {
    typedef typename detail::IteratorTraits<ValuesPortal>::IteratorType
                                                            IteratorType;
    IteratorType begin = IteratorBegin(values);
    IteratorType newLast = ::thrust::unique(begin, IteratorEnd(values));
    return ::thrust::distance(begin, newLast);
  }

  template<class ValuesPortal, class Compare>
  VTKM_CONT_EXPORT static
  vtkm::Id UniquePortal(const ValuesPortal values, Compare comp)
  {
    typedef typename detail::IteratorTraits<ValuesPortal>::IteratorType
                                                            IteratorType;
    IteratorType begin = IteratorBegin(values);
    IteratorType newLast = ::thrust::unique(begin, IteratorEnd(values), comp);
    return ::thrust::distance(begin, newLast);
  }

  template<class InputPortal, class ValuesPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  void UpperBoundsPortal(const InputPortal &input,
                         const ValuesPortal &values,
                         const OutputPortal &output)
  {
    ::thrust::upper_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output));
  }


  template<class InputPortal, class ValuesPortal, class OutputPortal,
           class Compare>
  VTKM_CONT_EXPORT static void UpperBoundsPortal(const InputPortal &input,
                                                const ValuesPortal &values,
                                                const OutputPortal &output,
                                                Compare comp)
  {
    ::thrust::upper_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output),
                          comp);
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  void UpperBoundsPortal(const InputPortal &input,
                         const OutputPortal &values_output)
  {
    ::thrust::upper_bound(IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values_output),
                          IteratorEnd(values_output),
                          IteratorBegin(values_output));
  }

//-----------------------------------------------------------------------------

public:
  template<typename T, class SIn, class SOut>
  VTKM_CONT_EXPORT static void Copy(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      vtkm::cont::ArrayHandle<T,SOut> &output)
  {
    vtkm::Id numberOfValues = input.GetNumberOfValues();
    CopyPortal(input.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template<typename T, class SIn, class SVal, class SOut>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<T,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template<typename T, class SIn, class SVal, class SOut, class Compare>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<T,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output,
      Compare comp)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                      comp);
  }

  template<class SIn, class SOut>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<vtkm::Id,SIn> &input,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut> &values_output)
  {
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values_output.PrepareForInPlace(DeviceAdapterTag()));
  }

  template<typename T, class SIn, class SOut>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      vtkm::cont::ArrayHandle<T,SOut>& output)
  {
    vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
      {
      output.PrepareForOutput(0, DeviceAdapterTag());
      return 0;
      }

    return ScanExclusivePortal(input.PrepareForInput(DeviceAdapterTag()),
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }
  template<typename T, class SIn, class SOut>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      vtkm::cont::ArrayHandle<T,SOut>& output)
  {
    vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
      {
      output.PrepareForOutput(0, DeviceAdapterTag());
      return 0;
      }

    return ScanInclusivePortal(input.PrepareForInput(DeviceAdapterTag()),
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

// Because of some funny code conversions in nvcc, kernels for devices have to
// be public.
#ifndef VTKM_CUDA
private:
#endif
  // we use cuda pinned memory to reduce the amount of synchronization
  // and mem copies between the host and device.
  VTKM_CONT_EXPORT
  static char* GetPinnedErrorArray(vtkm::Id &arraySize, char** hostPointer)
    {
    const vtkm::Id ERROR_ARRAY_SIZE = 1024;
    static bool errorArrayInit = false;
    static char* hostPtr = NULL;
    static char* devicePtr = NULL;
    if( !errorArrayInit )
      {
      cudaMallocHost( (void**)&hostPtr, ERROR_ARRAY_SIZE, cudaHostAllocMapped );
      cudaHostGetDevicePointer(&devicePtr, hostPtr, 0);
      errorArrayInit = true;
      }
    //set the size of the array
    arraySize = ERROR_ARRAY_SIZE;

    //specify the host pointer to the memory
    *hostPointer = hostPtr;
    (void) hostPointer;
    return devicePtr;
    }

public:
  template<class Functor>
  VTKM_CONT_EXPORT static void Schedule(Functor functor, vtkm::Id numInstances)
  {
    //since the memory is pinned we can access it safely on the host
    //without a memcpy
    vtkm::Id errorArraySize = 0;
    char* hostErrorPtr = NULL;
    char* deviceErrorPtr = GetPinnedErrorArray(errorArraySize, &hostErrorPtr);

    //clear the first character which means that we don't contain an error
    hostErrorPtr[0] = '\0';

    vtkm::exec::internal::ErrorMessageBuffer errorMessage( deviceErrorPtr,
                                                           errorArraySize);

    functor.SetErrorMessageBuffer(errorMessage);

    const int blockSize = 128;
    const int blocksPerGrid = (numInstances + blockSize - 1) / blockSize;

    Schedule1DIndexKernel<Functor> <<<blocksPerGrid, blockSize>>> (functor, numInstances);

    //sync so that we can check the results of the call.
    //In the future I want move this before the for_each call, and throwing
    //an exception if the previous schedule wrote an error. This would help
    //cuda to run longer before we hard sync.
    cudaDeviceSynchronize();

    //check what the value is
    if (hostErrorPtr[0] != '\0')
      {
      throw vtkm::cont::ErrorExecution(hostErrorPtr);
      }
  }

  template<class Functor>
  VTKM_CONT_EXPORT
  static void Schedule(Functor functor, const vtkm::Id3& rangeMax)
  {
    //since the memory is pinned we can access it safely on the host
    //without a memcpy
    vtkm::Id errorArraySize = 0;
    char* hostErrorPtr = NULL;
    char* deviceErrorPtr = GetPinnedErrorArray(errorArraySize, &hostErrorPtr);

    //clear the first character which means that we don't contain an error
    hostErrorPtr[0] = '\0';

    vtkm::exec::internal::ErrorMessageBuffer errorMessage( deviceErrorPtr,
                                                           errorArraySize);

    functor.SetErrorMessageBuffer(errorMessage);

#ifdef ANALYZE_VTKM_SCHEDULER
    //requires the errormessage buffer be set
    compare_3d_schedule_patterns(functor,rangeMax);
#endif

    dim3 blockSize3d(32,4,1);
    dim3 gridSize3d;

    compute_block_size(rangeMax, blockSize3d, gridSize3d);
    Schedule3DIndexKernel<Functor> <<<gridSize3d, blockSize3d>>> (functor, rangeMax);

    //sync so that we can check the results of the call.
    //In the future I want move this before the for_each call, and throwing
    //an exception if the previous schedule wrote an error. This would help
    //cuda to run longer before we hard sync.
    cudaDeviceSynchronize();

    //check what the value is
    if (hostErrorPtr[0] != '\0')
      {
      throw vtkm::cont::ErrorExecution(hostErrorPtr);
      }
  }

  template<typename T, class Storage>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Storage>& values)
  {
    SortPortal(values.PrepareForInPlace(DeviceAdapterTag()));
  }

  template<typename T, class Storage, class Compare>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Storage>& values,
      Compare comp)
  {
    SortPortal(values.PrepareForInPlace(DeviceAdapterTag()),comp);
  }

  template<typename T, typename U,
           class StorageT, class StorageU>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT>& keys,
      vtkm::cont::ArrayHandle<U,StorageU>& values)
  {
    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTag()),
                    values.PrepareForInPlace(DeviceAdapterTag()));
  }

  template<typename T, typename U,
           class StorageT, class StorageU,
           class Compare>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT>& keys,
      vtkm::cont::ArrayHandle<U,StorageU>& values,
      Compare comp)
  {
    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTag()),
                    values.PrepareForInPlace(DeviceAdapterTag()),
                    comp);
  }


  template<typename T, class SStencil, class SOut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,SStencil>& stencil,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output)
  {
    vtkm::Id stencilSize = stencil.GetNumberOfValues();

    RemoveIf(::thrust::make_counting_iterator<vtkm::Id>(0),
             ::thrust::make_counting_iterator<vtkm::Id>(stencilSize),
             stencil,
             output);
  }

  template<typename T,
           typename U,
           class SIn,
           class SStencil,
           class SOut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<U,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SStencil>& stencil,
      vtkm::cont::ArrayHandle<U,SOut>& output)
  {
    StreamCompactPortal(input.PrepareForInput(DeviceAdapterTag()), stencil, output);
  }

  template<typename T, class Storage>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values)
  {
    vtkm::Id newSize = UniquePortal(values.PrepareForInPlace(DeviceAdapterTag()));

    values.Shrink(newSize);
  }

  template<typename T, class Storage, class Compare>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values,
      Compare comp)
  {
    vtkm::Id newSize = UniquePortal(values.PrepareForInPlace(DeviceAdapterTag()),comp);

    values.Shrink(newSize);
  }

  template<typename T, class SIn, class SVal, class SOut>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template<typename T, class SIn, class SVal, class SOut, class Compare>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output,
      Compare comp)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                      comp);
  }

  template<class SIn, class SOut>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<vtkm::Id,SIn> &input,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut> &values_output)
  {
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values_output.PrepareForInPlace(DeviceAdapterTag()));
  }
};

}
}
}
} // namespace vtkm::cont::cuda::internal

#endif //vtk_m_cont_cuda_internal_DeviceAdapterThrust_h
