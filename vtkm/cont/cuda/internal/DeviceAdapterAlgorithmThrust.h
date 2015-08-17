
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
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterThrust_h
#define vtk_m_cont_cuda_internal_DeviceAdapterThrust_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/Types.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/UnaryPredicates.h>

#include <vtkm/cont/cuda/internal/MakeThrustIterator.h>

#include <vtkm/exec/internal/ErrorMessageBuffer.h>
#include <vtkm/exec/internal/WorkletInvokeFunctor.h>
#include <vtkm/exec/cuda/internal/WrappedOperators.h>

// Disable warnings we check vtkm for but Thrust does not.
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/advance.h>
#include <thrust/binary_search.h>
#include <thrust/copy.h>
#include <thrust/count.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/unique.h>
#include <thrust/system/cuda/vector.h>

#include <thrust/iterator/counting_iterator.h>
#include <thrust/system/cuda/execution_policy.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace cont {
namespace cuda {
namespace internal {

static
__global__
void DetermineProperXGridSize(vtkm::UInt32 desired_size,
                              vtkm::UInt32* actual_size)
{
//used only to see if we can launch kernels with a x grid size that
//matches the max of the graphics card, or are we having to fall back
//to SM_2 grid sizes
 if(blockIdx.x != 0)
  {
  return;
  }
#if __CUDA_ARCH__ <= 200
  const vtkm::UInt32 maxXGridSizeForSM2 = 65535;
  *actual_size = maxXGridSizeForSM2;
#else
  *actual_size = desired_size;
#endif
}

template<class FunctorType>
__global__
void Schedule1DIndexKernel(FunctorType functor,
                           vtkm::Id numberOfKernelsInvoked,
                           vtkm::Id length)
{
  //Note a cuda launch can only handle at most 2B iterations of a kernel
  //because it holds all of the indexes inside UInt32, so for use to
  //handle datasets larger than 2B, we need to execute multiple kernels
  const vtkm::Id index = numberOfKernelsInvoked +
                         static_cast<vtkm::Id>(blockDim.x * blockIdx.x + threadIdx.x);
  if(index < length)
    {
    functor(index);
    }
}

template<class FunctorType>
__global__
void Schedule3DIndexKernel(FunctorType functor, dim3 size)
{
  const vtkm::Id x = blockIdx.x*blockDim.x + threadIdx.x;
  const vtkm::Id y = blockIdx.y*blockDim.y + threadIdx.y;
  const vtkm::Id z = blockIdx.z*blockDim.z + threadIdx.z;

  if (x >= size.x || y >= size.y || z >= size.z)
    {
    return;
    }

  //now convert back to flat memory
  const int idx = x + size.x*(y + size.y*z);
  functor( idx );
}

inline
void compute_block_size(dim3 rangeMax, dim3 blockSize3d, dim3& gridSize3d)
{
  gridSize3d.x = (rangeMax.x % blockSize3d.x != 0) ? (rangeMax.x / blockSize3d.x + 1) : (rangeMax.x / blockSize3d.x);
  gridSize3d.y = (rangeMax.y % blockSize3d.y != 0) ? (rangeMax.y / blockSize3d.y + 1) : (rangeMax.y / blockSize3d.y);
  gridSize3d.z = (rangeMax.z % blockSize3d.z != 0) ? (rangeMax.z / blockSize3d.z + 1) : (rangeMax.z / blockSize3d.z);
}

#ifdef ANALYZE_VTKM_SCHEDULER
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
  const dim3 ranges(static_cast<vtkm::UInt32>(rangeMax[0]),
                    static_cast<vtkm::UInt32>(rangeMax[1]),
                    static_cast<vtkm::UInt32>(rangeMax[2]) );
  std::vector< PerfRecord > results;
  vtkm::UInt32 indexTable[16] = {1, 2, 4, 8, 12, 16, 20, 24, 28, 30, 32, 64, 128, 256, 512, 1024};

  for(vtkm::UInt32 i=0; i < 16; i++)
    {
    for(vtkm::UInt32 j=0; j < 16; j++)
      {
      for(vtkm::UInt32 k=0; k < 16; k++)
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

        compute_block_size(ranges, blockSize3d, gridSize3d);
        cudaEventRecord(start, 0);
        Schedule3DIndexKernel<Functor> <<<gridSize3d, blockSize3d>>> (functor, ranges);
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
  const vtkm::Int64 size = static_cast<vtkm::Int64>(results.size());
  for(vtkm::Int64 i=1; i <= size; i++)
    {
    vtkm::UInt64 index = static_cast<vtkm::UInt64>(size-i);
    vtkm::UInt32 x = results[index].blockSize.x;
    vtkm::UInt32 y = results[index].blockSize.y;
    vtkm::UInt32 z = results[index].blockSize.z;
    float t = results[index].elapsedTime;

    std::cout << "BlockSize of: " << x << "," << y << "," << z << " required: " << t << std::endl;
    }

  std::cout << "flat array performance " << std::endl;
  {
  cudaEvent_t start, stop;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);

  cudaEventRecord(start, 0);
  typedef
    vtkm::cont::cuda::internal::DeviceAdapterAlgorithmThrust<
          vtkm::cont::DeviceAdapterTagCuda > Algorithm;
  Algorithm::Schedule(functor, numInstances);
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

  dim3 blockSize3d(64,2,1);
  dim3 gridSize3d;

  compute_block_size(ranges, blockSize3d, gridSize3d);
  cudaEventRecord(start, 0);
  Schedule3DIndexKernel<Functor> <<<gridSize3d, blockSize3d>>> (functor, ranges);
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

#endif


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
    ::thrust::copy(thrust::cuda::par,
                   IteratorBegin(input),
                   IteratorEnd(input),
                   IteratorBegin(output));
  }

  template<class InputPortal, class ValuesPortal, class OutputPortal>
  VTKM_CONT_EXPORT static void LowerBoundsPortal(const InputPortal &input,
                                                 const ValuesPortal &values,
                                                 const OutputPortal &output)
  {
    typedef typename ValuesPortal::ValueType ValueType;
    LowerBoundsPortal(input, values, output, ::thrust::less<ValueType>() );
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  void LowerBoundsPortal(const InputPortal &input,
                         const OutputPortal &values_output)
  {
    typedef typename InputPortal::ValueType ValueType;
    LowerBoundsPortal(input, values_output, values_output,
                      ::thrust::less<ValueType>() );
  }

  template<class InputPortal, class ValuesPortal, class OutputPortal,
           class BinaryCompare>
  VTKM_CONT_EXPORT static void LowerBoundsPortal(const InputPortal &input,
                                                 const ValuesPortal &values,
                                                 const OutputPortal &output,
                                                 BinaryCompare binary_compare)
  {
    typedef typename InputPortal::ValueType ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryPredicate<ValueType,
                                            BinaryCompare> bop(binary_compare);
    ::thrust::lower_bound(thrust::cuda::par,
                          IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output),
                          bop);
  }

  template<class InputPortal>
  VTKM_CONT_EXPORT static
  typename InputPortal::ValueType ReducePortal(const InputPortal &input,
                            typename InputPortal::ValueType initialValue)
  {
    typedef typename InputPortal::ValueType ValueType;
    return ReducePortal(input,
                        initialValue,
                        ::thrust::plus<ValueType>());
  }

  template<class InputPortal, class BinaryFunctor>
  VTKM_CONT_EXPORT static
  typename InputPortal::ValueType ReducePortal(const InputPortal &input,
                            typename InputPortal::ValueType initialValue,
                            BinaryFunctor binary_functor)
  {
    typedef typename InputPortal::ValueType ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType,
                                                      BinaryFunctor> bop(binary_functor);
    return ::thrust::reduce(thrust::cuda::par,
                            IteratorBegin(input),
                            IteratorEnd(input),
                            initialValue,
                            bop);
  }

  template<class KeysPortal, class ValuesPortal,
           class KeysOutputPortal, class ValueOutputPortal,
           class BinaryFunctor>
  VTKM_CONT_EXPORT static
  vtkm::Id ReduceByKeyPortal(const KeysPortal &keys,
                             const ValuesPortal& values,
                             const KeysOutputPortal &keys_output,
                             const ValueOutputPortal &values_output,
                             BinaryFunctor binary_functor)
  {
    typedef typename detail::IteratorTraits<KeysOutputPortal>::IteratorType
                                                             KeysIteratorType;
    typedef typename detail::IteratorTraits<ValueOutputPortal>::IteratorType
                                                             ValuesIteratorType;

    KeysIteratorType keys_out_begin = IteratorBegin(keys_output);
    ValuesIteratorType values_out_begin = IteratorBegin(values_output);

    ::thrust::pair< KeysIteratorType, ValuesIteratorType > result_iterators;

    ::thrust::equal_to<typename KeysPortal::ValueType> binaryPredicate;

    typedef typename ValuesPortal::ValueType ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType,
                                                      BinaryFunctor> bop(binary_functor);
    result_iterators = ::thrust::reduce_by_key(thrust::cuda::par,
                                               IteratorBegin(keys),
                                               IteratorEnd(keys),
                                               IteratorBegin(values),
                                               keys_out_begin,
                                               values_out_begin,
                                               binaryPredicate,
                                               bop);

    return static_cast<vtkm::Id>( ::thrust::distance(keys_out_begin,
                                                     result_iterators.first) );
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  typename InputPortal::ValueType ScanExclusivePortal(const InputPortal &input,
                                                      const OutputPortal &output)
  {
    typedef typename OutputPortal::ValueType ValueType;

    return ScanExclusivePortal(input,
                               output,
                               (::thrust::plus<ValueType>()) );

  }

    template<class InputPortal, class OutputPortal, class BinaryOperation>
  VTKM_CONT_EXPORT static
  typename InputPortal::ValueType ScanExclusivePortal(const InputPortal &input,
                                                      const OutputPortal &output,
                                                      BinaryOperation binaryOp)
  {
    // Use iterator to get value so that thrust device_ptr has chance to handle
    // data on device.
    typedef typename OutputPortal::ValueType ValueType;
    ValueType inputEnd = *(IteratorEnd(input) - 1);

    vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType,
                                                      BinaryOperation> bop(binaryOp);

    typedef typename detail::IteratorTraits<OutputPortal>::IteratorType
                                                            IteratorType;
    IteratorType end = ::thrust::exclusive_scan(thrust::cuda::par,
                                                IteratorBegin(input),
                                                IteratorEnd(input),
                                                IteratorBegin(output),
                                                vtkm::TypeTraits<ValueType>::ZeroInitialization(),
                                                bop);

    //return the value at the last index in the array, as that is the sum
    return binaryOp( *(end-1), inputEnd);
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  typename InputPortal::ValueType ScanInclusivePortal(const InputPortal &input,
                                                      const OutputPortal &output)
  {
    typedef typename OutputPortal::ValueType ValueType;
    return ScanInclusivePortal(input, output, ::thrust::plus<ValueType>() );
  }

  template<class InputPortal, class OutputPortal, class BinaryFunctor>
  VTKM_CONT_EXPORT static
  typename InputPortal::ValueType ScanInclusivePortal(const InputPortal &input,
                                                      const OutputPortal &output,
                                                      BinaryFunctor binary_functor)
  {
    typedef typename OutputPortal::ValueType ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType,
                                                      BinaryFunctor> bop(binary_functor);

    typedef typename detail::IteratorTraits<OutputPortal>::IteratorType
                                                            IteratorType;

    IteratorType end = ::thrust::inclusive_scan(thrust::cuda::par,
                                                IteratorBegin(input),
                                                IteratorEnd(input),
                                                IteratorBegin(output),
                                                bop);

    //return the value at the last index in the array, as that is the sum
    return *(end-1);
  }

  template<class ValuesPortal>
  VTKM_CONT_EXPORT static void SortPortal(const ValuesPortal &values)
  {
    typedef typename ValuesPortal::ValueType ValueType;
    SortPortal(values, ::thrust::less<ValueType>());
  }

  template<class ValuesPortal, class BinaryCompare>
  VTKM_CONT_EXPORT static void SortPortal(const ValuesPortal &values,
                                         BinaryCompare binary_compare)
  {
    typedef typename ValuesPortal::ValueType ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryPredicate<ValueType,
                                                       BinaryCompare> bop(binary_compare);
    ::thrust::sort(thrust::cuda::par,
                   IteratorBegin(values),
                   IteratorEnd(values),
                   bop);
  }


  template<class KeysPortal, class ValuesPortal>
  VTKM_CONT_EXPORT static void SortByKeyPortal(const KeysPortal &keys,
                                               const ValuesPortal &values)
  {
    typedef typename KeysPortal::ValueType ValueType;
    SortByKeyPortal(keys,values,::thrust::less<ValueType>());
  }

  template<class KeysPortal, class ValuesPortal, class BinaryCompare>
  VTKM_CONT_EXPORT static void SortByKeyPortal(const KeysPortal &keys,
                                               const ValuesPortal &values,
                                               BinaryCompare binary_compare)
  {
    typedef typename KeysPortal::ValueType ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryPredicate<ValueType,
                                                       BinaryCompare> bop(binary_compare);
    ::thrust::sort_by_key(thrust::cuda::par,
                          IteratorBegin(keys),
                          IteratorEnd(keys),
                          IteratorBegin(values),
                          bop);
  }

  template<class ValueIterator,
           class StencilPortal,
           class OutputPortal,
           class UnaryPredicate>
  VTKM_CONT_EXPORT static
  vtkm::Id CopyIfPortal(ValueIterator valuesBegin,
                        ValueIterator valuesEnd,
                        StencilPortal stencil,
                        OutputPortal output,
                        UnaryPredicate unary_predicate)
  {
    typedef typename detail::IteratorTraits<OutputPortal>::IteratorType
                                                            IteratorType;

    IteratorType outputBegin = IteratorBegin(output);
    IteratorType newLast = ::thrust::copy_if(thrust::cuda::par,
                                             valuesBegin,
                                             valuesEnd,
                                             IteratorBegin(stencil),
                                             outputBegin,
                                             unary_predicate);

    return static_cast<vtkm::Id>( ::thrust::distance(outputBegin, newLast) );
  }

    template<class ValuePortal,
             class StencilPortal,
             class OutputPortal,
             class UnaryPredicate>
  VTKM_CONT_EXPORT static
  vtkm::Id CopyIfPortal(ValuePortal values,
                        StencilPortal stencil,
                        OutputPortal output,
                        UnaryPredicate unary_predicate)
  {
    return CopyIfPortal(IteratorBegin(values),
                        IteratorEnd(values),
                        stencil,
                        output,
                        unary_predicate);
  }

  template<class ValuesPortal>
  VTKM_CONT_EXPORT static
  vtkm::Id UniquePortal(const ValuesPortal values)
  {
    typedef typename detail::IteratorTraits<ValuesPortal>::IteratorType
                                                            IteratorType;
    IteratorType begin = IteratorBegin(values);
    IteratorType newLast = ::thrust::unique(thrust::cuda::par,
                                            begin,
                                            IteratorEnd(values));
    return static_cast<vtkm::Id>( ::thrust::distance(begin, newLast) );
  }

  template<class ValuesPortal, class BinaryCompare>
  VTKM_CONT_EXPORT static
  vtkm::Id UniquePortal(const ValuesPortal values, BinaryCompare binary_compare)
  {
    typedef typename detail::IteratorTraits<ValuesPortal>::IteratorType
                                                            IteratorType;
    typedef typename ValuesPortal::ValueType ValueType;

    vtkm::exec::cuda::internal::WrappedBinaryPredicate<ValueType,
                                                       BinaryCompare> bop(binary_compare);
    IteratorType begin = IteratorBegin(values);
    IteratorType newLast = ::thrust::unique(thrust::cuda::par,
                                            begin,
                                            IteratorEnd(values),
                                            bop);
    return static_cast<vtkm::Id>( ::thrust::distance(begin, newLast) );
  }

  template<class InputPortal, class ValuesPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  void UpperBoundsPortal(const InputPortal &input,
                         const ValuesPortal &values,
                         const OutputPortal &output)
  {
    ::thrust::upper_bound(thrust::cuda::par,
                          IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output));
  }


  template<class InputPortal, class ValuesPortal, class OutputPortal,
           class BinaryCompare>
  VTKM_CONT_EXPORT static void UpperBoundsPortal(const InputPortal &input,
                                                const ValuesPortal &values,
                                                const OutputPortal &output,
                                                BinaryCompare binary_compare)
  {
    typedef typename OutputPortal::ValueType ValueType;

    vtkm::exec::cuda::internal::WrappedBinaryPredicate<ValueType,
                                                       BinaryCompare> bop(binary_compare);
    ::thrust::upper_bound(thrust::cuda::par,
                          IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values),
                          IteratorEnd(values),
                          IteratorBegin(output),
                          bop);
  }

  template<class InputPortal, class OutputPortal>
  VTKM_CONT_EXPORT static
  void UpperBoundsPortal(const InputPortal &input,
                         const OutputPortal &values_output)
  {
    ::thrust::upper_bound(thrust::cuda::par,
                          IteratorBegin(input),
                          IteratorEnd(input),
                          IteratorBegin(values_output),
                          IteratorEnd(values_output),
                          IteratorBegin(values_output));
  }

//-----------------------------------------------------------------------------

public:
  template<typename T, typename U, class SIn, class SOut>
  VTKM_CONT_EXPORT static void Copy(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      vtkm::cont::ArrayHandle<U,SOut> &output)
  {
    const vtkm::Id numberOfValues = input.GetNumberOfValues();

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    input.PrepareForInput(DeviceAdapterTag());
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

  template<typename T, class SIn, class SVal, class SOut, class BinaryCompare>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<T,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output,
      BinaryCompare binary_compare)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                      binary_compare);
  }

  template<class SIn, class SOut>
  VTKM_CONT_EXPORT static void LowerBounds(
      const vtkm::cont::ArrayHandle<vtkm::Id,SIn> &input,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut> &values_output)
  {
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values_output.PrepareForInPlace(DeviceAdapterTag()));
  }

 template<typename T, class SIn>
  VTKM_CONT_EXPORT static T Reduce(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      T initialValue)
  {
    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
      {
      return initialValue;
      }
    return ReducePortal(input.PrepareForInput( DeviceAdapterTag() ),
                        initialValue);
  }

 template<typename T, class SIn, class BinaryFunctor>
  VTKM_CONT_EXPORT static T Reduce(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      T initialValue,
      BinaryFunctor binary_functor)
  {
    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
      {
      return initialValue;
      }
    return ReducePortal(input.PrepareForInput( DeviceAdapterTag() ),
                        initialValue,
                        binary_functor);
  }

 template<typename T, typename U, class KIn, class VIn, class KOut, class VOut,
          class BinaryFunctor>
  VTKM_CONT_EXPORT static void ReduceByKey(
      const vtkm::cont::ArrayHandle<T,KIn> &keys,
      const vtkm::cont::ArrayHandle<U,VIn> &values,
      vtkm::cont::ArrayHandle<T,KOut> &keys_output,
      vtkm::cont::ArrayHandle<U,VOut> &values_output,
      BinaryFunctor binary_functor)
  {
    //there is a concern that by default we will allocate too much
    //space for the keys/values output. 1 option is to
    const vtkm::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
      {
      return;
      }
    vtkm::Id reduced_size =
            ReduceByKeyPortal(keys.PrepareForInput( DeviceAdapterTag() ),
                              values.PrepareForInput( DeviceAdapterTag() ),
                              keys_output.PrepareForOutput( numberOfValues, DeviceAdapterTag() ),
                              values_output.PrepareForOutput( numberOfValues, DeviceAdapterTag() ),
                              binary_functor);

    keys_output.Shrink( reduced_size );
    values_output.Shrink( reduced_size );
  }

  template<typename T, class SIn, class SOut>
  VTKM_CONT_EXPORT static T ScanExclusive(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      vtkm::cont::ArrayHandle<T,SOut>& output)
  {
    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
      {
      output.PrepareForOutput(0, DeviceAdapterTag());
      return vtkm::TypeTraits<T>::ZeroInitialization();
      }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    input.PrepareForInput(DeviceAdapterTag());
    return ScanExclusivePortal(input.PrepareForInput(DeviceAdapterTag()),
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template<typename T, class SIn, class SOut>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      vtkm::cont::ArrayHandle<T,SOut>& output)
  {
    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
      {
      output.PrepareForOutput(0, DeviceAdapterTag());
      return vtkm::TypeTraits<T>::ZeroInitialization();
      }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    input.PrepareForInput(DeviceAdapterTag());
    return ScanInclusivePortal(input.PrepareForInput(DeviceAdapterTag()),
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template<typename T, class SIn, class SOut, class BinaryFunctor>
  VTKM_CONT_EXPORT static T ScanInclusive(
      const vtkm::cont::ArrayHandle<T,SIn> &input,
      vtkm::cont::ArrayHandle<T,SOut>& output,
      BinaryFunctor binary_functor)
  {
    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
      {
      output.PrepareForOutput(0, DeviceAdapterTag());
      return vtkm::TypeTraits<T>::ZeroInitialization();
      }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    input.PrepareForInput(DeviceAdapterTag());
    return ScanInclusivePortal(input.PrepareForInput(DeviceAdapterTag()),
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                               binary_functor);
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

  // we query cuda for the max blocks per grid for 1D scheduling
  // and cache the values in static variables
  VTKM_CONT_EXPORT
  static vtkm::Vec<vtkm::UInt32,3> GetMaxGridOfThreadBlocks()
    {
    static bool gridQueryInit = false;
    static vtkm::Vec< vtkm::UInt32, 3> maxGridSize;
    if( !gridQueryInit )
      {
      gridQueryInit = true;
      int currDevice; cudaGetDevice(&currDevice); //get deviceid from cuda

      cudaDeviceProp properties;
      cudaGetDeviceProperties(&properties, currDevice);
      maxGridSize[0] = static_cast<vtkm::UInt32>(properties.maxGridSize[0]);
      maxGridSize[1] = static_cast<vtkm::UInt32>(properties.maxGridSize[1]);
      maxGridSize[2] = static_cast<vtkm::UInt32>(properties.maxGridSize[2]);

      //Note: While in practice SM_3+ devices can schedule up to (2^31-1) grids
      //in the X direction, it is dependent on the code being compiled for SM3+.
      //If not, it falls back to SM_2 limitation of 65535 being the largest grid
      //size.
      //Now since SM architecture is only available inside kernels we have to
      //invoke one to see what the actual limit is for our device.  So that is
      //what we are going to do next, and than we will store that result

      vtkm::UInt32 *dev_actual_size;
      cudaMalloc( (void**)&dev_actual_size, sizeof(vtkm::UInt32) );
      DetermineProperXGridSize <<<1,1>>> (maxGridSize[0], dev_actual_size);
      cudaDeviceSynchronize();
      cudaMemcpy( &maxGridSize[0],
                  dev_actual_size,
                  sizeof(vtkm::UInt32),
                  cudaMemcpyDeviceToHost );
      cudaFree(dev_actual_size);
      }
    return maxGridSize;
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

    const vtkm::UInt32 blockSize = 128;
    const vtkm::UInt32 maxblocksPerLaunch = GetMaxGridOfThreadBlocks()[0];
    const vtkm::Id totalBlocks = (numInstances + blockSize - 1) / blockSize;

    //Note a cuda launch can only handle at most 2B iterations of a kernel
    //because it holds all of the indexes inside UInt32, so for use to
    //handle datasets larger than 2B, we need to execute multiple kernels
    if(totalBlocks < maxblocksPerLaunch)
      {
      Schedule1DIndexKernel<Functor> <<<totalBlocks, blockSize>>> (functor,
                                                                   vtkm::Id(0),
                                                                   numInstances);
      }
    else
      {
      const vtkm::Id numberOfKernelsToRun = blockSize * maxblocksPerLaunch;
      for(vtkm::Id numberOfKernelsInvoked = 0;
          numberOfKernelsInvoked < numInstances;
          numberOfKernelsInvoked += numberOfKernelsToRun)
        {
        Schedule1DIndexKernel<Functor> <<<maxblocksPerLaunch, blockSize>>> (functor,
                                                                            numberOfKernelsInvoked,
                                                                            numInstances);
        }
      }

    //sync so that we can check the results of the call.
    //In the future I want move this before the schedule call, and throwing
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
    const dim3 ranges(static_cast<vtkm::UInt32>(rangeMax[0]),
                      static_cast<vtkm::UInt32>(rangeMax[1]),
                      static_cast<vtkm::UInt32>(rangeMax[2]) );


    //currently we presume that 3d scheduling access patterns prefer accessing
    //memory in the X direction. Also should be good for thin in the Z axis
    //algorithms.
    dim3 blockSize3d(64,2,1);

    //handle the simple use case of 'bad' datasets which are thin in X
    //but larger in the other directions, allowing us decent performance with
    //that use case.
    if(rangeMax[0] <= 128 &&
       (rangeMax[0] < rangeMax[1] || rangeMax[0] < rangeMax[2]) )
      {
      blockSize3d = dim3(16,4,4);
      }

    dim3 gridSize3d;
    compute_block_size(ranges, blockSize3d, gridSize3d);

    Schedule3DIndexKernel<Functor> <<<gridSize3d, blockSize3d>>> (functor, ranges);

    //sync so that we can check the results of the call.
    //In the future I want move this before the schedule call, and throwing
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

  template<typename T, class Storage, class BinaryCompare>
  VTKM_CONT_EXPORT static void Sort(
      vtkm::cont::ArrayHandle<T,Storage>& values,
      BinaryCompare binary_compare)
  {
    SortPortal(values.PrepareForInPlace(DeviceAdapterTag()),binary_compare);
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
           class BinaryCompare>
  VTKM_CONT_EXPORT static void SortByKey(
      vtkm::cont::ArrayHandle<T,StorageT>& keys,
      vtkm::cont::ArrayHandle<U,StorageU>& values,
      BinaryCompare binary_compare)
  {
    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTag()),
                    values.PrepareForInPlace(DeviceAdapterTag()),
                    binary_compare);
  }


  template<typename T, class SStencil, class SOut>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<T,SStencil>& stencil,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output)
  {
    vtkm::Id size = stencil.GetNumberOfValues();
    vtkm::Id newSize = CopyIfPortal(::thrust::make_counting_iterator<vtkm::Id>(0),
                                    ::thrust::make_counting_iterator<vtkm::Id>(size),
                                    stencil.PrepareForInput(DeviceAdapterTag()),
                                    output.PrepareForOutput(size, DeviceAdapterTag()),
                                    ::vtkm::NotZeroInitialized());
    output.Shrink(newSize);
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
    vtkm::Id size = stencil.GetNumberOfValues();
    vtkm::Id newSize = CopyIfPortal(input.PrepareForInput(DeviceAdapterTag()),
                                    stencil.PrepareForInput(DeviceAdapterTag()),
                                    output.PrepareForOutput(size, DeviceAdapterTag()),
                                    ::vtkm::NotZeroInitialized()); //yes on the stencil
    output.Shrink(newSize);
  }

  template<typename T,
           typename U,
           class SIn,
           class SStencil,
           class SOut,
           class UnaryPredicate>
  VTKM_CONT_EXPORT static void StreamCompact(
      const vtkm::cont::ArrayHandle<U,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SStencil>& stencil,
      vtkm::cont::ArrayHandle<U,SOut>& output,
      UnaryPredicate unary_predicate)
  {
    vtkm::Id size = stencil.GetNumberOfValues();
    vtkm::Id newSize = CopyIfPortal(input.PrepareForInput(DeviceAdapterTag()),
                                    stencil.PrepareForInput(DeviceAdapterTag()),
                                    output.PrepareForOutput(size, DeviceAdapterTag()),
                                    unary_predicate);
    output.Shrink(newSize);
  }

  template<typename T, class Storage>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values)
  {
    vtkm::Id newSize = UniquePortal(values.PrepareForInPlace(DeviceAdapterTag()));

    values.Shrink(newSize);
  }

  template<typename T, class Storage, class BinaryCompare>
  VTKM_CONT_EXPORT static void Unique(
      vtkm::cont::ArrayHandle<T,Storage> &values,
      BinaryCompare binary_compare)
  {
    vtkm::Id newSize = UniquePortal(values.PrepareForInPlace(DeviceAdapterTag()),binary_compare);

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

  template<typename T, class SIn, class SVal, class SOut, class BinaryCompare>
  VTKM_CONT_EXPORT static void UpperBounds(
      const vtkm::cont::ArrayHandle<T,SIn>& input,
      const vtkm::cont::ArrayHandle<T,SVal>& values,
      vtkm::cont::ArrayHandle<vtkm::Id,SOut>& output,
      BinaryCompare binary_compare)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                      binary_compare);
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
