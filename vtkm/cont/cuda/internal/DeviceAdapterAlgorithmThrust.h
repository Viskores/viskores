
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
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmThrust_h
#define vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmThrust_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/UnaryPredicates.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ErrorExecution.h>

#include <vtkm/cont/cuda/ErrorCuda.h>

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/cuda/internal/MakeThrustIterator.h>
#include <vtkm/cont/cuda/internal/ThrustExceptionHandler.h>

#include <vtkm/exec/cuda/internal/TaskStrided.h>
#include <vtkm/exec/cuda/internal/WrappedOperators.h>
#include <vtkm/exec/internal/ErrorMessageBuffer.h>

#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>


// #define PARAMETER_SWEEP_VTKM_SCHEDULER_1D
// #define PARAMETER_SWEEP_VTKM_SCHEDULER_3D
#if defined(PARAMETER_SWEEP_VTKM_SCHEDULER_1D) || defined(PARAMETER_SWEEP_VTKM_SCHEDULER_3D)
#include <vtkm/cont/cuda/internal/TaskTuner.h>
#endif

// Disable warnings we check vtkm for but Thrust does not.
VTKM_THIRDPARTY_PRE_INCLUDE
//our own custom thrust execution policy
#include <thrust/advance.h>
#include <thrust/binary_search.h>
#include <thrust/copy.h>
#include <thrust/count.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/scan.h>
#include <thrust/sort.h>
#include <thrust/system/cpp/memory.h>
#include <thrust/system/cuda/vector.h>
#include <thrust/unique.h>
#include <vtkm/exec/cuda/internal/ExecutionPolicy.h>
VTKM_THIRDPARTY_POST_INCLUDE


namespace vtkm
{
namespace cont
{

template <class DeviceAdapterTag>
struct DeviceAdapterAlgorithm;

namespace cuda
{
namespace internal
{

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

template <typename TaskType>
__global__ void TaskStrided1DLaunch(TaskType task, vtkm::Id size)
{
  //see https://devblogs.nvidia.com/cuda-pro-tip-write-flexible-kernels-grid-stride-loops/
  //for why our inc is grid-stride
  const vtkm::Id start = blockIdx.x * blockDim.x + threadIdx.x;
  const vtkm::Id inc = blockDim.x * gridDim.x;
  task(start, size, inc);
}

template <typename TaskType>
__global__ void TaskStrided3DLaunch(TaskType task, dim3 size)
{
  //This is the 3D version of executing in a grid-stride manner
  const dim3 start(blockIdx.x * blockDim.x + threadIdx.x,
                   blockIdx.y * blockDim.y + threadIdx.y,
                   blockIdx.z * blockDim.z + threadIdx.z);
  const dim3 inc(blockDim.x * gridDim.x, blockDim.y * gridDim.y, blockDim.z * gridDim.z);

  for (uint k = start.z; k < size.z; k += inc.z)
  {
    for (uint j = start.y; j < size.y; j += inc.y)
    {
      task(start.x, size.x, inc.x, j, k);
    }
  }
}


template <typename T, typename BinaryOperationType>
__global__ void SumExclusiveScan(T a, T b, T result, BinaryOperationType binary_op)
{
  result = binary_op(a, b);
}

#if (defined(VTKM_GCC) || defined(VTKM_CLANG))
#pragma GCC diagnostic pop
#endif

/// This class can be subclassed to implement the DeviceAdapterAlgorithm for a
/// device that uses thrust as its implementation. The subclass should pass in
/// the correct device adapter tag as the template parameter.
///
template <class DeviceAdapterTag>
struct DeviceAdapterAlgorithmThrust
  : vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      vtkm::cont::DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>,
      vtkm::cont::DeviceAdapterTagCuda>
{
// Because of some funny code conversions in nvcc, kernels for devices have to
// be public.
#ifndef VTKM_CUDA
private:
#endif
  template <class InputPortal, class OutputPortal>
  VTKM_CONT static void CopyPortal(const InputPortal& input, const OutputPortal& output)
  {
    try
    {
      ::thrust::copy(
        ThrustCudaPolicyPerThread, IteratorBegin(input), IteratorEnd(input), IteratorBegin(output));
    }
    catch (...)
    {
      throwAsVTKmException();
    }
  }

  template <class ValueIterator, class StencilPortal, class OutputPortal, class UnaryPredicate>
  VTKM_CONT static vtkm::Id CopyIfPortal(ValueIterator valuesBegin,
                                         ValueIterator valuesEnd,
                                         StencilPortal stencil,
                                         OutputPortal output,
                                         UnaryPredicate unary_predicate)
  {
    auto outputBegin = IteratorBegin(output);

    using ValueType = typename StencilPortal::ValueType;

    vtkm::exec::cuda::internal::WrappedUnaryPredicate<ValueType, UnaryPredicate> up(
      unary_predicate);

    try
    {
      auto newLast = ::thrust::copy_if(
        ThrustCudaPolicyPerThread, valuesBegin, valuesEnd, IteratorBegin(stencil), outputBegin, up);
      return static_cast<vtkm::Id>(::thrust::distance(outputBegin, newLast));
    }
    catch (...)
    {
      throwAsVTKmException();
      return vtkm::Id(0);
    }
  }

  template <class ValuePortal, class StencilPortal, class OutputPortal, class UnaryPredicate>
  VTKM_CONT static vtkm::Id CopyIfPortal(ValuePortal values,
                                         StencilPortal stencil,
                                         OutputPortal output,
                                         UnaryPredicate unary_predicate)
  {
    return CopyIfPortal(
      IteratorBegin(values), IteratorEnd(values), stencil, output, unary_predicate);
  }

  template <class InputPortal, class OutputPortal>
  VTKM_CONT static void CopySubRangePortal(const InputPortal& input,
                                           vtkm::Id inputOffset,
                                           vtkm::Id size,
                                           const OutputPortal& output,
                                           vtkm::Id outputOffset)
  {
    try
    {
      ::thrust::copy_n(ThrustCudaPolicyPerThread,
                       IteratorBegin(input) + inputOffset,
                       static_cast<std::size_t>(size),
                       IteratorBegin(output) + outputOffset);
    }
    catch (...)
    {
      throwAsVTKmException();
    }
  }

  template <class InputPortal, class ValuesPortal, class OutputPortal>
  VTKM_CONT static void LowerBoundsPortal(const InputPortal& input,
                                          const ValuesPortal& values,
                                          const OutputPortal& output)
  {
    using ValueType = typename ValuesPortal::ValueType;
    LowerBoundsPortal(input, values, output, ::thrust::less<ValueType>());
  }

  template <class InputPortal, class OutputPortal>
  VTKM_CONT static void LowerBoundsPortal(const InputPortal& input,
                                          const OutputPortal& values_output)
  {
    using ValueType = typename InputPortal::ValueType;
    LowerBoundsPortal(input, values_output, values_output, ::thrust::less<ValueType>());
  }

  template <class InputPortal, class ValuesPortal, class OutputPortal, class BinaryCompare>
  VTKM_CONT static void LowerBoundsPortal(const InputPortal& input,
                                          const ValuesPortal& values,
                                          const OutputPortal& output,
                                          BinaryCompare binary_compare)
  {
    using ValueType = typename InputPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryPredicate<ValueType, BinaryCompare> bop(
      binary_compare);

    try
    {
      ::thrust::lower_bound(ThrustCudaPolicyPerThread,
                            IteratorBegin(input),
                            IteratorEnd(input),
                            IteratorBegin(values),
                            IteratorEnd(values),
                            IteratorBegin(output),
                            bop);
    }
    catch (...)
    {
      throwAsVTKmException();
    }
  }

  template <class InputPortal, typename T>
  VTKM_CONT static T ReducePortal(const InputPortal& input, T initialValue)
  {
    return ReducePortal(input, initialValue, ::thrust::plus<T>());
  }

  template <class InputPortal, typename T, class BinaryFunctor>
  VTKM_CONT static T ReducePortal(const InputPortal& input,
                                  T initialValue,
                                  BinaryFunctor binary_functor)
  {
    using fast_path = std::is_same<typename InputPortal::ValueType, T>;
    return ReducePortalImpl(input, initialValue, binary_functor, fast_path());
  }

  template <class InputPortal, typename T, class BinaryFunctor>
  VTKM_CONT static T ReducePortalImpl(const InputPortal& input,
                                      T initialValue,
                                      BinaryFunctor binary_functor,
                                      std::true_type)
  {
    //The portal type and the initial value are the same so we can use
    //the thrust reduction algorithm
    vtkm::exec::cuda::internal::WrappedBinaryOperator<T, BinaryFunctor> bop(binary_functor);

    try
    {
      return ::thrust::reduce(
        ThrustCudaPolicyPerThread, IteratorBegin(input), IteratorEnd(input), initialValue, bop);
    }
    catch (...)
    {
      throwAsVTKmException();
    }

    return initialValue;
  }

  template <class InputPortal, typename T, class BinaryFunctor>
  VTKM_CONT static T ReducePortalImpl(const InputPortal& input,
                                      T initialValue,
                                      BinaryFunctor binary_functor,
                                      std::false_type)
  {
    //The portal type and the initial value AREN'T the same type so we have
    //to a slower approach, where we wrap the input portal inside a cast
    //portal
    using CastFunctor = vtkm::cont::internal::Cast<typename InputPortal::ValueType, T>;

    vtkm::exec::internal::ArrayPortalTransform<T, InputPortal, CastFunctor> castPortal(input);

    vtkm::exec::cuda::internal::WrappedBinaryOperator<T, BinaryFunctor> bop(binary_functor);

    try
    {
      return ::thrust::reduce(ThrustCudaPolicyPerThread,
                              IteratorBegin(castPortal),
                              IteratorEnd(castPortal),
                              initialValue,
                              bop);
    }
    catch (...)
    {
      throwAsVTKmException();
    }

    return initialValue;
  }

  template <class KeysPortal,
            class ValuesPortal,
            class KeysOutputPortal,
            class ValueOutputPortal,
            class BinaryFunctor>
  VTKM_CONT static vtkm::Id ReduceByKeyPortal(const KeysPortal& keys,
                                              const ValuesPortal& values,
                                              const KeysOutputPortal& keys_output,
                                              const ValueOutputPortal& values_output,
                                              BinaryFunctor binary_functor)
  {
    auto keys_out_begin = IteratorBegin(keys_output);
    auto values_out_begin = IteratorBegin(values_output);

    ::thrust::pair<decltype(keys_out_begin), decltype(values_out_begin)> result_iterators;

    ::thrust::equal_to<typename KeysPortal::ValueType> binaryPredicate;

    using ValueType = typename ValuesPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType, BinaryFunctor> bop(binary_functor);

    try
    {
      result_iterators = ::thrust::reduce_by_key(vtkm_cuda_policy(),
                                                 IteratorBegin(keys),
                                                 IteratorEnd(keys),
                                                 IteratorBegin(values),
                                                 keys_out_begin,
                                                 values_out_begin,
                                                 binaryPredicate,
                                                 bop);
    }
    catch (...)
    {
      throwAsVTKmException();
    }

    return static_cast<vtkm::Id>(::thrust::distance(keys_out_begin, result_iterators.first));
  }

  template <class InputPortal, class OutputPortal>
  VTKM_CONT static typename InputPortal::ValueType ScanExclusivePortal(const InputPortal& input,
                                                                       const OutputPortal& output)
  {
    using ValueType = typename OutputPortal::ValueType;

    return ScanExclusivePortal(input,
                               output,
                               (::thrust::plus<ValueType>()),
                               vtkm::TypeTraits<ValueType>::ZeroInitialization());
  }

  template <class InputPortal, class OutputPortal, class BinaryFunctor>
  VTKM_CONT static typename InputPortal::ValueType ScanExclusivePortal(
    const InputPortal& input,
    const OutputPortal& output,
    BinaryFunctor binaryOp,
    typename InputPortal::ValueType initialValue)
  {
    // Use iterator to get value so that thrust device_ptr has chance to handle
    // data on device.
    using ValueType = typename OutputPortal::ValueType;

    //we have size three so that we can store the origin end value, the
    //new end value, and the sum of those two
    ::thrust::system::cuda::vector<ValueType> sum(3);
    try
    {

      //store the current value of the last position array in a separate cuda
      //memory location since the exclusive_scan will overwrite that value
      //once run
      ::thrust::copy_n(ThrustCudaPolicyPerThread, IteratorEnd(input) - 1, 1, sum.begin());

      vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType, BinaryFunctor> bop(binaryOp);

      auto end = ::thrust::exclusive_scan(ThrustCudaPolicyPerThread,
                                          IteratorBegin(input),
                                          IteratorEnd(input),
                                          IteratorBegin(output),
                                          initialValue,
                                          bop);

      //Store the new value for the end of the array. This is done because
      //with items such as the transpose array it is unsafe to pass the
      //portal to the SumExclusiveScan
      ::thrust::copy_n(ThrustCudaPolicyPerThread, (end - 1), 1, sum.begin() + 1);

      //execute the binaryOp one last time on the device.
      SumExclusiveScan<<<1, 1, 0, cudaStreamPerThread>>>(sum[0], sum[1], sum[2], bop);
    }
    catch (...)
    {
      throwAsVTKmException();
    }
    return sum[2];
  }

  template <class InputPortal, class OutputPortal>
  VTKM_CONT static typename InputPortal::ValueType ScanInclusivePortal(const InputPortal& input,
                                                                       const OutputPortal& output)
  {
    using ValueType = typename OutputPortal::ValueType;
    return ScanInclusivePortal(input, output, ::thrust::plus<ValueType>());
  }

  template <class InputPortal, class OutputPortal, class BinaryFunctor>
  VTKM_CONT static typename InputPortal::ValueType ScanInclusivePortal(const InputPortal& input,
                                                                       const OutputPortal& output,
                                                                       BinaryFunctor binary_functor)
  {
    using ValueType = typename OutputPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType, BinaryFunctor> bop(binary_functor);

    try
    {
      ::thrust::system::cuda::vector<ValueType> result(1);
      auto end = ::thrust::inclusive_scan(ThrustCudaPolicyPerThread,
                                          IteratorBegin(input),
                                          IteratorEnd(input),
                                          IteratorBegin(output),
                                          bop);

      ::thrust::copy_n(ThrustCudaPolicyPerThread, end - 1, 1, result.begin());
      return result[0];
    }
    catch (...)
    {
      throwAsVTKmException();
      return typename InputPortal::ValueType();
    }

    //return the value at the last index in the array, as that is the sum
  }

  template <typename KeysPortal, typename ValuesPortal, typename OutputPortal>
  VTKM_CONT static void ScanInclusiveByKeyPortal(const KeysPortal& keys,
                                                 const ValuesPortal& values,
                                                 const OutputPortal& output)
  {
    using KeyType = typename KeysPortal::ValueType;
    using ValueType = typename OutputPortal::ValueType;
    ScanInclusiveByKeyPortal(
      keys, values, output, ::thrust::equal_to<KeyType>(), ::thrust::plus<ValueType>());
  }

  template <typename KeysPortal,
            typename ValuesPortal,
            typename OutputPortal,
            typename BinaryPredicate,
            typename AssociativeOperator>
  VTKM_CONT static void ScanInclusiveByKeyPortal(const KeysPortal& keys,
                                                 const ValuesPortal& values,
                                                 const OutputPortal& output,
                                                 BinaryPredicate binary_predicate,
                                                 AssociativeOperator binary_operator)
  {
    using KeyType = typename KeysPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryOperator<KeyType, BinaryPredicate> bpred(
      binary_predicate);
    using ValueType = typename OutputPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType, AssociativeOperator> bop(
      binary_operator);

    try
    {
      ::thrust::inclusive_scan_by_key(ThrustCudaPolicyPerThread,
                                      IteratorBegin(keys),
                                      IteratorEnd(keys),
                                      IteratorBegin(values),
                                      IteratorBegin(output),
                                      bpred,
                                      bop);
    }
    catch (...)
    {
      throwAsVTKmException();
    }
  }

  template <typename KeysPortal, typename ValuesPortal, typename OutputPortal>
  VTKM_CONT static void ScanExclusiveByKeyPortal(const KeysPortal& keys,
                                                 const ValuesPortal& values,
                                                 const OutputPortal& output)
  {
    using KeyType = typename KeysPortal::ValueType;
    using ValueType = typename OutputPortal::ValueType;
    ScanExclusiveByKeyPortal(keys,
                             values,
                             output,
                             vtkm::TypeTraits<ValueType>::ZeroInitialization(),
                             ::thrust::equal_to<KeyType>(),
                             ::thrust::plus<ValueType>());
  }

  template <typename KeysPortal,
            typename ValuesPortal,
            typename OutputPortal,
            typename T,
            typename BinaryPredicate,
            typename AssociativeOperator>
  VTKM_CONT static void ScanExclusiveByKeyPortal(const KeysPortal& keys,
                                                 const ValuesPortal& values,
                                                 const OutputPortal& output,
                                                 T initValue,
                                                 BinaryPredicate binary_predicate,
                                                 AssociativeOperator binary_operator)
  {
    using KeyType = typename KeysPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryOperator<KeyType, BinaryPredicate> bpred(
      binary_predicate);
    using ValueType = typename OutputPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType, AssociativeOperator> bop(
      binary_operator);
    try
    {
      ::thrust::exclusive_scan_by_key(ThrustCudaPolicyPerThread,
                                      IteratorBegin(keys),
                                      IteratorEnd(keys),
                                      IteratorBegin(values),
                                      IteratorBegin(output),
                                      initValue,
                                      bpred,
                                      bop);
    }
    catch (...)
    {
      throwAsVTKmException();
    }
  }

  template <class ValuesPortal>
  VTKM_CONT static void SortPortal(const ValuesPortal& values)
  {
    using ValueType = typename ValuesPortal::ValueType;
    SortPortal(values, ::thrust::less<ValueType>());
  }

  template <class ValuesPortal, class BinaryCompare>
  VTKM_CONT static void SortPortal(const ValuesPortal& values, BinaryCompare binary_compare)
  {
    using ValueType = typename ValuesPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryPredicate<ValueType, BinaryCompare> bop(
      binary_compare);
    try
    {
      ::thrust::sort(vtkm_cuda_policy(), IteratorBegin(values), IteratorEnd(values), bop);
    }
    catch (...)
    {
      throwAsVTKmException();
    }
  }

  template <class KeysPortal, class ValuesPortal>
  VTKM_CONT static void SortByKeyPortal(const KeysPortal& keys, const ValuesPortal& values)
  {
    using ValueType = typename KeysPortal::ValueType;
    SortByKeyPortal(keys, values, ::thrust::less<ValueType>());
  }

  template <class KeysPortal, class ValuesPortal, class BinaryCompare>
  VTKM_CONT static void SortByKeyPortal(const KeysPortal& keys,
                                        const ValuesPortal& values,
                                        BinaryCompare binary_compare)
  {
    using ValueType = typename KeysPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryPredicate<ValueType, BinaryCompare> bop(
      binary_compare);
    try
    {
      ::thrust::sort_by_key(
        vtkm_cuda_policy(), IteratorBegin(keys), IteratorEnd(keys), IteratorBegin(values), bop);
    }
    catch (...)
    {
      throwAsVTKmException();
    }
  }

  template <class ValuesPortal>
  VTKM_CONT static vtkm::Id UniquePortal(const ValuesPortal values)
  {
    try
    {
      auto begin = IteratorBegin(values);
      auto newLast = ::thrust::unique(ThrustCudaPolicyPerThread, begin, IteratorEnd(values));
      return static_cast<vtkm::Id>(::thrust::distance(begin, newLast));
    }
    catch (...)
    {
      throwAsVTKmException();
      return vtkm::Id(0);
    }
  }

  template <class ValuesPortal, class BinaryCompare>
  VTKM_CONT static vtkm::Id UniquePortal(const ValuesPortal values, BinaryCompare binary_compare)
  {
    using ValueType = typename ValuesPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryPredicate<ValueType, BinaryCompare> bop(
      binary_compare);
    try
    {
      auto begin = IteratorBegin(values);
      auto newLast = ::thrust::unique(ThrustCudaPolicyPerThread, begin, IteratorEnd(values), bop);
      return static_cast<vtkm::Id>(::thrust::distance(begin, newLast));
    }
    catch (...)
    {
      throwAsVTKmException();
      return vtkm::Id(0);
    }
  }

  template <class InputPortal, class ValuesPortal, class OutputPortal>
  VTKM_CONT static void UpperBoundsPortal(const InputPortal& input,
                                          const ValuesPortal& values,
                                          const OutputPortal& output)
  {
    try
    {
      ::thrust::upper_bound(ThrustCudaPolicyPerThread,
                            IteratorBegin(input),
                            IteratorEnd(input),
                            IteratorBegin(values),
                            IteratorEnd(values),
                            IteratorBegin(output));
    }
    catch (...)
    {
      throwAsVTKmException();
    }
  }

  template <class InputPortal, class ValuesPortal, class OutputPortal, class BinaryCompare>
  VTKM_CONT static void UpperBoundsPortal(const InputPortal& input,
                                          const ValuesPortal& values,
                                          const OutputPortal& output,
                                          BinaryCompare binary_compare)
  {
    using ValueType = typename OutputPortal::ValueType;

    vtkm::exec::cuda::internal::WrappedBinaryPredicate<ValueType, BinaryCompare> bop(
      binary_compare);
    try
    {
      ::thrust::upper_bound(ThrustCudaPolicyPerThread,
                            IteratorBegin(input),
                            IteratorEnd(input),
                            IteratorBegin(values),
                            IteratorEnd(values),
                            IteratorBegin(output),
                            bop);
    }
    catch (...)
    {
      throwAsVTKmException();
    }
  }

  template <class InputPortal, class OutputPortal>
  VTKM_CONT static void UpperBoundsPortal(const InputPortal& input,
                                          const OutputPortal& values_output)
  {
    try
    {
      ::thrust::upper_bound(ThrustCudaPolicyPerThread,
                            IteratorBegin(input),
                            IteratorEnd(input),
                            IteratorBegin(values_output),
                            IteratorEnd(values_output),
                            IteratorBegin(values_output));
    }
    catch (...)
    {
      throwAsVTKmException();
    }
  }

  //-----------------------------------------------------------------------------

public:
  template <typename T, typename U, class SIn, class SOut>
  VTKM_CONT static void Copy(const vtkm::cont::ArrayHandle<T, SIn>& input,
                             vtkm::cont::ArrayHandle<U, SOut>& output)
  {
    const vtkm::Id inSize = input.GetNumberOfValues();
    CopyPortal(input.PrepareForInput(DeviceAdapterTag()),
               output.PrepareForOutput(inSize, DeviceAdapterTag()));
  }

  template <typename T, typename U, class SIn, class SStencil, class SOut>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<U, SIn>& input,
                               const vtkm::cont::ArrayHandle<T, SStencil>& stencil,
                               vtkm::cont::ArrayHandle<U, SOut>& output)
  {
    vtkm::Id size = stencil.GetNumberOfValues();
    vtkm::Id newSize = CopyIfPortal(input.PrepareForInput(DeviceAdapterTag()),
                                    stencil.PrepareForInput(DeviceAdapterTag()),
                                    output.PrepareForOutput(size, DeviceAdapterTag()),
                                    ::vtkm::NotZeroInitialized()); //yes on the stencil
    output.Shrink(newSize);
  }

  template <typename T, typename U, class SIn, class SStencil, class SOut, class UnaryPredicate>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<U, SIn>& input,
                               const vtkm::cont::ArrayHandle<T, SStencil>& stencil,
                               vtkm::cont::ArrayHandle<U, SOut>& output,
                               UnaryPredicate unary_predicate)
  {
    vtkm::Id size = stencil.GetNumberOfValues();
    vtkm::Id newSize = CopyIfPortal(input.PrepareForInput(DeviceAdapterTag()),
                                    stencil.PrepareForInput(DeviceAdapterTag()),
                                    output.PrepareForOutput(size, DeviceAdapterTag()),
                                    unary_predicate);
    output.Shrink(newSize);
  }

  template <typename T, typename U, class SIn, class SOut>
  VTKM_CONT static bool CopySubRange(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                     vtkm::Id inputStartIndex,
                                     vtkm::Id numberOfElementsToCopy,
                                     vtkm::cont::ArrayHandle<U, SOut>& output,
                                     vtkm::Id outputIndex = 0)
  {
    const vtkm::Id inSize = input.GetNumberOfValues();

    // Check if the ranges overlap and fail if they do.
    if (input == output && ((outputIndex >= inputStartIndex &&
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
        vtkm::cont::ArrayHandle<U, SOut> temp;
        temp.Allocate(copyOutEnd);
        CopySubRange(output, 0, outSize, temp);
        output = temp;
      }
    }
    CopySubRangePortal(input.PrepareForInput(DeviceAdapterTag()),
                       inputStartIndex,
                       numberOfElementsToCopy,
                       output.PrepareForInPlace(DeviceAdapterTag()),
                       outputIndex);
    return true;
  }

  template <typename T, class SIn, class SVal, class SOut>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                    const vtkm::cont::ArrayHandle<T, SVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& output)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template <typename T, class SIn, class SVal, class SOut, class BinaryCompare>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                    const vtkm::cont::ArrayHandle<T, SVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& output,
                                    BinaryCompare binary_compare)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                      binary_compare);
  }

  template <class SIn, class SOut>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<vtkm::Id, SIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& values_output)
  {
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values_output.PrepareForInPlace(DeviceAdapterTag()));
  }

  template <typename T, typename U, class SIn>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, SIn>& input, U initialValue)
  {
    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      return initialValue;
    }
    return ReducePortal(input.PrepareForInput(DeviceAdapterTag()), initialValue);
  }

  template <typename T, typename U, class SIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, SIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      return initialValue;
    }
    return ReducePortal(input.PrepareForInput(DeviceAdapterTag()), initialValue, binary_functor);
  }

  template <typename T,
            typename U,
            class KIn,
            class VIn,
            class KOut,
            class VOut,
            class BinaryFunctor>
  VTKM_CONT static void ReduceByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                    const vtkm::cont::ArrayHandle<U, VIn>& values,
                                    vtkm::cont::ArrayHandle<T, KOut>& keys_output,
                                    vtkm::cont::ArrayHandle<U, VOut>& values_output,
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
      ReduceByKeyPortal(keys.PrepareForInput(DeviceAdapterTag()),
                        values.PrepareForInput(DeviceAdapterTag()),
                        keys_output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                        values_output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                        binary_functor);

    keys_output.Shrink(reduced_size);
    values_output.Shrink(reduced_size);
  }

  template <typename T, class SIn, class SOut>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                   vtkm::cont::ArrayHandle<T, SOut>& output)
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
    auto inputPortal = input.PrepareForInput(DeviceAdapterTag());
    return ScanExclusivePortal(inputPortal,
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template <typename T, class SIn, class SOut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                   vtkm::cont::ArrayHandle<T, SOut>& output,
                                   BinaryFunctor binary_functor,
                                   const T& initialValue)
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
    auto inputPortal = input.PrepareForInput(DeviceAdapterTag());
    return ScanExclusivePortal(inputPortal,
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                               binary_functor,
                               initialValue);
  }

  template <typename T, class SIn, class SOut>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                   vtkm::cont::ArrayHandle<T, SOut>& output)
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
    auto inputPortal = input.PrepareForInput(DeviceAdapterTag());
    return ScanInclusivePortal(inputPortal,
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template <typename T, class SIn, class SOut, class BinaryFunctor>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                   vtkm::cont::ArrayHandle<T, SOut>& output,
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
    auto inputPortal = input.PrepareForInput(DeviceAdapterTag());
    return ScanInclusivePortal(
      inputPortal, output.PrepareForOutput(numberOfValues, DeviceAdapterTag()), binary_functor);
  }

  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanInclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output)
  {
    const vtkm::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTag());
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTag());
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTag());
    ScanInclusiveByKeyPortal(
      keysPortal, valuesPortal, output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template <typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  VTKM_CONT static void ScanInclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output,
                                           BinaryFunctor binary_functor)
  {
    const vtkm::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTag());
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTag());
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTag());
    ScanInclusiveByKeyPortal(keysPortal,
                             valuesPortal,
                             output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                             ::thrust::equal_to<T>(),
                             binary_functor);
  }

  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanExclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output)
  {
    const vtkm::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTag());
      return vtkm::TypeTraits<T>::ZeroInitialization();
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTag());
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTag());
    ScanExnclusiveByKeyPortal(keysPortal,
                              valuesPortal,
                              output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                              vtkm::TypeTraits<T>::ZeroInitialization(),
                              vtkm::Add());
  }

  template <typename T,
            typename U,
            typename KIn,
            typename VIn,
            typename VOut,
            typename BinaryFunctor>
  VTKM_CONT static void ScanExclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output,
                                           const U& initialValue,
                                           BinaryFunctor binary_functor)
  {
    const vtkm::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTag());
      return;
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTag());
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTag());
    ScanExclusiveByKeyPortal(keysPortal,
                             valuesPortal,
                             output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                             initialValue,
                             ::thrust::equal_to<T>(),
                             binary_functor);
  }
  // we use cuda pinned memory to reduce the amount of synchronization
  // and mem copies between the host and device.
  VTKM_CONT_EXPORT
  static char* GetPinnedErrorArray(vtkm::Id& arraySize, char** hostPointer);

  VTKM_CONT_EXPORT
  static char* SetupErrorBuffer(vtkm::exec::cuda::internal::TaskStrided& functor);

  VTKM_CONT_EXPORT
  static void GetGridsAndBlocks(int& grid, int& blocks, vtkm::Id size);

  VTKM_CONT_EXPORT
  static void GetGridsAndBlocks(int& grid, dim3& blocks, const dim3& size);

public:
  template <typename WType, typename IType>
  static void ScheduleTask(vtkm::exec::cuda::internal::TaskStrided1D<WType, IType>& functor,
                           vtkm::Id numInstances)
  {
    VTKM_ASSERT(numInstances >= 0);
    if (numInstances < 1)
    {
      // No instances means nothing to run. Just return.
      return;
    }
    char* hostErrorPtr = SetupErrorBuffer(functor);

    int grids, blocks;
    GetGridsAndBlocks(grids, blocks, numInstances);

    TaskStrided1DLaunch<<<grids, blocks, 0, cudaStreamPerThread>>>(functor, numInstances);

    //sync so that we can check the results of the call.
    //In the future I want move this before the schedule call, and throwing
    //an exception if the previous schedule wrote an error. This would help
    //cuda to run longer before we hard sync.
    VTKM_CUDA_CALL(cudaStreamSynchronize(cudaStreamPerThread));

    //check what the value is
    if (hostErrorPtr[0] != '\0')
    {
      throw vtkm::cont::ErrorExecution(hostErrorPtr);
    }

#ifdef PARAMETER_SWEEP_VTKM_SCHEDULER_1D
    parameter_sweep_1d_schedule(functor, numInstances);
#endif
  }

  template <typename WType, typename IType>
  static void ScheduleTask(vtkm::exec::cuda::internal::TaskStrided3D<WType, IType>& functor,
                           vtkm::Id3 rangeMax)
  {
    VTKM_ASSERT((rangeMax[0] >= 0) && (rangeMax[1] >= 0) && (rangeMax[2] >= 0));
    if ((rangeMax[0] < 1) || (rangeMax[1] < 1) || (rangeMax[2] < 1))
    {
      // No instances means nothing to run. Just return.
      return;
    }
    char* hostErrorPtr = SetupErrorBuffer(functor);

    const dim3 ranges(static_cast<vtkm::UInt32>(rangeMax[0]),
                      static_cast<vtkm::UInt32>(rangeMax[1]),
                      static_cast<vtkm::UInt32>(rangeMax[2]));

    int grids;
    dim3 blocks;
    GetGridsAndBlocks(grids, blocks, ranges);

    TaskStrided3DLaunch<<<grids, blocks, 0, cudaStreamPerThread>>>(functor, ranges);

    //sync so that we can check the results of the call.
    //In the future I want move this before the schedule call, and throwing
    //an exception if the previous schedule wrote an error. This would help
    //cuda to run longer before we hard sync.
    VTKM_CUDA_CALL(cudaStreamSynchronize(cudaStreamPerThread));

    //check what the value is
    if (hostErrorPtr[0] != '\0')
    {
      throw vtkm::cont::ErrorExecution(hostErrorPtr);
    }

#ifdef PARAMETER_SWEEP_VTKM_SCHEDULER_3D
    parameter_sweep_3d_schedule(functor, rangeMax);
#endif
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id numInstances)
  {
    vtkm::exec::cuda::internal::TaskStrided1D<Functor, vtkm::internal::NullType> kernel(functor);
    ScheduleTask(kernel, numInstances);
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, const vtkm::Id3& rangeMax)
  {
    vtkm::exec::cuda::internal::TaskStrided3D<Functor, vtkm::internal::NullType> kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

  template <typename T, class Storage>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    SortPortal(values.PrepareForInPlace(DeviceAdapterTag()));
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    SortPortal(values.PrepareForInPlace(DeviceAdapterTag()), binary_compare);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTag()),
                    values.PrepareForInPlace(DeviceAdapterTag()));
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTag()),
                    values.PrepareForInPlace(DeviceAdapterTag()),
                    binary_compare);
  }

  template <typename T, class Storage>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    vtkm::Id newSize = UniquePortal(values.PrepareForInPlace(DeviceAdapterTag()));

    values.Shrink(newSize);
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    vtkm::Id newSize = UniquePortal(values.PrepareForInPlace(DeviceAdapterTag()), binary_compare);

    values.Shrink(newSize);
  }

  template <typename T, class SIn, class SVal, class SOut>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                    const vtkm::cont::ArrayHandle<T, SVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& output)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()));
  }

  template <typename T, class SIn, class SVal, class SOut, class BinaryCompare>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                    const vtkm::cont::ArrayHandle<T, SVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& output,
                                    BinaryCompare binary_compare)
  {
    vtkm::Id numberOfValues = values.GetNumberOfValues();
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values.PrepareForInput(DeviceAdapterTag()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTag()),
                      binary_compare);
  }

  template <class SIn, class SOut>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<vtkm::Id, SIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& values_output)
  {
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTag()),
                      values_output.PrepareForInPlace(DeviceAdapterTag()));
  }
};
}
}
}
} // namespace vtkm::cont::cuda::internal

#endif //vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmThrust_h
