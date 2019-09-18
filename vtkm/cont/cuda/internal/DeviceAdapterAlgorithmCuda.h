//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h
#define vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h

#include <vtkm/Math.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/UnaryPredicates.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/BitField.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/internal/DeviceAdapterAlgorithmGeneral.h>

#include <vtkm/cont/cuda/ErrorCuda.h>
#include <vtkm/cont/cuda/internal/ArrayManagerExecutionCuda.h>
#include <vtkm/cont/cuda/internal/AtomicInterfaceExecutionCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterRuntimeDetectorCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTimerImplementationCuda.h>
#include <vtkm/cont/cuda/internal/MakeThrustIterator.h>
#include <vtkm/cont/cuda/internal/ThrustExceptionHandler.h>
#include <vtkm/exec/cuda/internal/WrappedOperators.h>

#include <vtkm/exec/cuda/internal/TaskStrided.h>
#include <vtkm/exec/internal/ErrorMessageBuffer.h>

// Disable warnings we check vtkm for but Thrust does not.
#include <vtkm/exec/cuda/internal/ThrustPatches.h>
VTKM_THIRDPARTY_PRE_INCLUDE
//needs to be first
#include <vtkm/exec/cuda/internal/ExecutionPolicy.h>

#include <cooperative_groups.h>
#include <cuda.h>
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

VTKM_THIRDPARTY_POST_INCLUDE

#include <limits>
#include <memory>

namespace vtkm
{
namespace cont
{
namespace cuda
{

/// \brief RAII helper for temporarily changing CUDA stack size in an
/// exception-safe way.
struct ScopedCudaStackSize
{
  ScopedCudaStackSize(std::size_t newStackSize)
  {
    cudaDeviceGetLimit(&this->OldStackSize, cudaLimitStackSize);
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Temporarily changing Cuda stack size from "
                 << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(this->OldStackSize))
                 << " to "
                 << vtkm::cont::GetHumanReadableSize(static_cast<vtkm::UInt64>(newStackSize)));
    cudaDeviceSetLimit(cudaLimitStackSize, newStackSize);
  }

  ~ScopedCudaStackSize()
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Restoring Cuda stack size to " << vtkm::cont::GetHumanReadableSize(
                 static_cast<vtkm::UInt64>(this->OldStackSize)));
    cudaDeviceSetLimit(cudaLimitStackSize, this->OldStackSize);
  }

  // Disable copy
  ScopedCudaStackSize(const ScopedCudaStackSize&) = delete;
  ScopedCudaStackSize& operator=(const ScopedCudaStackSize&) = delete;

private:
  std::size_t OldStackSize;
};

/// \brief Represents how to schedule 1D, 2D, and 3D Cuda kernels
///
/// \c ScheduleParameters represents how VTK-m should schedule different
/// cuda kernel types. By default VTK-m uses a preset table based on the
/// GPU's found at runtime.
///
/// When these defaults are insufficient for certain projects it is possible
/// to override the defaults by using \c InitScheduleParameters.
///
///
struct VTKM_CONT_EXPORT ScheduleParameters
{
  int one_d_blocks;
  int one_d_threads_per_block;

  int two_d_blocks;
  dim3 two_d_threads_per_block;

  int three_d_blocks;
  dim3 three_d_threads_per_block;
};

/// \brief Specify the custom scheduling to use for VTK-m CUDA kernel launches
///
/// By default VTK-m uses a preset table based on the GPU's found at runtime to
/// determine the best scheduling parameters for a worklet.  When these defaults
/// are insufficient for certain projects it is possible to override the defaults
/// by binding a custom function to \c InitScheduleParameters.
///
/// Note: The this function must be called before any invocation of any worklets
/// by VTK-m.
///
/// Note: This function will be called for each GPU on a machine.
///
/// \code{.cpp}
///
///  ScheduleParameters CustomScheduleValues(char const* name,
///                                          int major,
///                                          int minor,
///                                          int multiProcessorCount,
///                                          int maxThreadsPerMultiProcessor,
///                                          int maxThreadsPerBlock)
///  {
///
///    ScheduleParameters params  {
///        64 * multiProcessorCount,  //1d blocks
///        64,                        //1d threads per block
///        64 * multiProcessorCount,  //2d blocks
///        { 8, 8, 1 },               //2d threads per block
///        64 * multiProcessorCount,  //3d blocks
///        { 4, 4, 4 } };             //3d threads per block
///    return params;
///  }
/// \endcode
///
///
VTKM_CONT_EXPORT void InitScheduleParameters(
  vtkm::cont::cuda::ScheduleParameters (*)(char const* name,
                                           int major,
                                           int minor,
                                           int multiProcessorCount,
                                           int maxThreadsPerMultiProcessor,
                                           int maxThreadsPerBlock));

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

  for (vtkm::Id k = start.z; k < size.z; k += inc.z)
  {
    for (vtkm::Id j = start.y; j < size.y; j += inc.y)
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

template <typename PortalType, typename BinaryAndUnaryFunctor>
struct CastPortal
{
  using InputType = typename PortalType::ValueType;
  using ValueType = decltype(std::declval<BinaryAndUnaryFunctor>()(std::declval<InputType>()));

  PortalType Portal;
  BinaryAndUnaryFunctor Functor;

  VTKM_CONT
  CastPortal(const PortalType& portal, const BinaryAndUnaryFunctor& functor)
    : Portal(portal)
    , Functor(functor)
  {
  }

  VTKM_EXEC
  vtkm::Id GetNumberOfValues() const { return this->Portal.GetNumberOfValues(); }

  VTKM_EXEC
  ValueType Get(vtkm::Id index) const { return this->Functor(this->Portal.Get(index)); }
};

struct CudaFreeFunctor
{
  void operator()(void* ptr) const { VTKM_CUDA_CALL(cudaFree(ptr)); }
};

template <typename T>
using CudaUniquePtr = std::unique_ptr<T, CudaFreeFunctor>;

template <typename T>
CudaUniquePtr<T> make_CudaUniquePtr(std::size_t numElements)
{
  T* ptr;
  VTKM_CUDA_CALL(cudaMalloc(&ptr, sizeof(T) * numElements));
  return CudaUniquePtr<T>(ptr);
}
}
} // end namespace cuda::internal

template <>
struct DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>
  : vtkm::cont::internal::DeviceAdapterAlgorithmGeneral<
      vtkm::cont::DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagCuda>,
      vtkm::cont::DeviceAdapterTagCuda>
{
// Because of some funny code conversions in nvcc, kernels for devices have to
// be public.
#ifndef VTKM_CUDA
private:
#endif

  template <typename BitsPortal, typename IndicesPortal, typename GlobalPopCountType>
  struct BitFieldToUnorderedSetFunctor : public vtkm::exec::FunctorBase
  {
    VTKM_STATIC_ASSERT_MSG(VTKM_PASS_COMMAS(std::is_same<GlobalPopCountType, vtkm::Int32>::value ||
                                            std::is_same<GlobalPopCountType, vtkm::UInt32>::value ||
                                            std::is_same<GlobalPopCountType, vtkm::UInt64>::value),
                           "Unsupported GlobalPopCountType. Must support CUDA atomicAdd.");

    //Using typename BitsPortal::WordTypePreferred causes dependent type errors using GCC 4.8.5
    //which is the GCC required compiler for CUDA 9.2 on summit/power9
    using Word = typename vtkm::cont::internal::AtomicInterfaceExecution<
      DeviceAdapterTagCuda>::WordTypePreferred;

    VTKM_STATIC_ASSERT(
      VTKM_PASS_COMMAS(std::is_same<typename IndicesPortal::ValueType, vtkm::Id>::value));

    VTKM_CONT
    BitFieldToUnorderedSetFunctor(const BitsPortal& input,
                                  const IndicesPortal& output,
                                  GlobalPopCountType* globalPopCount)
      : Input{ input }
      , Output{ output }
      , GlobalPopCount{ globalPopCount }
      , FinalWordIndex{ input.GetNumberOfWords() - 1 }
      , FinalWordMask(input.GetFinalWordMask())
    {
    }

    ~BitFieldToUnorderedSetFunctor() {}

    VTKM_CONT void Initialize()
    {
      assert(this->GlobalPopCount != nullptr);
      VTKM_CUDA_CALL(cudaMemset(this->GlobalPopCount, 0, sizeof(GlobalPopCountType)));
    }

    VTKM_SUPPRESS_EXEC_WARNINGS
    __device__ void operator()(vtkm::Id wordIdx) const
    {
      Word word = this->Input.GetWord(wordIdx);

      // The last word may be partial -- mask out trailing bits if needed.
      const Word mask = wordIdx == this->FinalWordIndex ? this->FinalWordMask : ~Word{ 0 };

      word &= mask;

      if (word != 0)
      {
        this->LocalPopCount = vtkm::CountSetBits(word);
        this->ReduceAllocate();

        vtkm::Id firstBitIdx = wordIdx * sizeof(Word) * CHAR_BIT;
        do
        {
          // Find next bit. FindFirstSetBit's result is indexed starting at 1.
          vtkm::Int32 bit = vtkm::FindFirstSetBit(word) - 1;
          vtkm::Id outIdx = this->GetNextOutputIndex();
          // Write index of bit
          this->Output.Set(outIdx, firstBitIdx + bit);
          word ^= (1 << bit); // clear bit
        } while (word != 0);  // have bits
      }
    }

    VTKM_CONT vtkm::Id Finalize() const
    {
      assert(this->GlobalPopCount != nullptr);
      GlobalPopCountType result;
      VTKM_CUDA_CALL(cudaMemcpy(
        &result, this->GlobalPopCount, sizeof(GlobalPopCountType), cudaMemcpyDeviceToHost));
      return static_cast<vtkm::Id>(result);
    }

  private:
    // Every thread with a non-zero local popcount calls this function, which
    // computes the total popcount for the coalesced threads and allocates
    // a contiguous block in the output by atomically increasing the global
    // popcount.
    VTKM_SUPPRESS_EXEC_WARNINGS
    __device__ void ReduceAllocate() const
    {
      const auto activeLanes = cooperative_groups::coalesced_threads();
      const int activeRank = activeLanes.thread_rank();
      const int activeSize = activeLanes.size();

      // Reduction value:
      vtkm::Int32 rVal = this->LocalPopCount;
      for (int delta = 1; delta < activeSize; delta *= 2)
      {
        const vtkm::Int32 shflVal = activeLanes.shfl_down(rVal, delta);
        if (activeRank + delta < activeSize)
        {
          rVal += shflVal;
        }
      }

      if (activeRank == 0)
      {
        this->AllocationHead =
          atomicAdd(this->GlobalPopCount, static_cast<GlobalPopCountType>(rVal));
      }

      this->AllocationHead = activeLanes.shfl(this->AllocationHead, 0);
    }

    // The global output allocation is written to by striding the writes across
    // the warp lanes, allowing the writes to global memory to be coalesced.
    VTKM_SUPPRESS_EXEC_WARNINGS
    __device__ vtkm::Id GetNextOutputIndex() const
    {
      // Only lanes with unwritten output indices left will call this method,
      // so just check the coalesced threads:
      const auto activeLanes = cooperative_groups::coalesced_threads();
      const int activeRank = activeLanes.thread_rank();
      const int activeSize = activeLanes.size();

      vtkm::Id nextIdx = static_cast<vtkm::Id>(this->AllocationHead + activeRank);
      this->AllocationHead += activeSize;

      return nextIdx;
    }

    const BitsPortal Input;
    const IndicesPortal Output;
    GlobalPopCountType* GlobalPopCount;
    mutable vtkm::UInt64 AllocationHead{ 0 };
    mutable vtkm::Int32 LocalPopCount{ 0 };
    // Used to mask trailing bits the in last word.
    vtkm::Id FinalWordIndex{ 0 };
    Word FinalWordMask{ 0 };
  };

  template <class InputPortal, class OutputPortal>
  VTKM_CONT static void CopyPortal(const InputPortal& input, const OutputPortal& output)
  {
    try
    {
      ::thrust::copy(ThrustCudaPolicyPerThread,
                     cuda::internal::IteratorBegin(input),
                     cuda::internal::IteratorEnd(input),
                     cuda::internal::IteratorBegin(output));
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
    }
  }

  template <class ValueIterator, class StencilPortal, class OutputPortal, class UnaryPredicate>
  VTKM_CONT static vtkm::Id CopyIfPortal(ValueIterator valuesBegin,
                                         ValueIterator valuesEnd,
                                         StencilPortal stencil,
                                         OutputPortal output,
                                         UnaryPredicate unary_predicate)
  {
    auto outputBegin = cuda::internal::IteratorBegin(output);

    using ValueType = typename StencilPortal::ValueType;

    vtkm::exec::cuda::internal::WrappedUnaryPredicate<ValueType, UnaryPredicate> up(
      unary_predicate);

    try
    {
      auto newLast = ::thrust::copy_if(ThrustCudaPolicyPerThread,
                                       valuesBegin,
                                       valuesEnd,
                                       cuda::internal::IteratorBegin(stencil),
                                       outputBegin,
                                       up);
      return static_cast<vtkm::Id>(::thrust::distance(outputBegin, newLast));
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
      return vtkm::Id(0);
    }
  }

  template <class ValuePortal, class StencilPortal, class OutputPortal, class UnaryPredicate>
  VTKM_CONT static vtkm::Id CopyIfPortal(ValuePortal values,
                                         StencilPortal stencil,
                                         OutputPortal output,
                                         UnaryPredicate unary_predicate)
  {
    return CopyIfPortal(cuda::internal::IteratorBegin(values),
                        cuda::internal::IteratorEnd(values),
                        stencil,
                        output,
                        unary_predicate);
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
                       cuda::internal::IteratorBegin(input) + inputOffset,
                       static_cast<std::size_t>(size),
                       cuda::internal::IteratorBegin(output) + outputOffset);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
    }
  }


  template <typename BitsPortal, typename GlobalPopCountType>
  struct CountSetBitsFunctor : public vtkm::exec::FunctorBase
  {
    VTKM_STATIC_ASSERT_MSG(VTKM_PASS_COMMAS(std::is_same<GlobalPopCountType, vtkm::Int32>::value ||
                                            std::is_same<GlobalPopCountType, vtkm::UInt32>::value ||
                                            std::is_same<GlobalPopCountType, vtkm::UInt64>::value),
                           "Unsupported GlobalPopCountType. Must support CUDA atomicAdd.");

    //Using typename BitsPortal::WordTypePreferred causes dependent type errors using GCC 4.8.5
    //which is the GCC required compiler for CUDA 9.2 on summit/power9
    using Word = typename vtkm::cont::internal::AtomicInterfaceExecution<
      DeviceAdapterTagCuda>::WordTypePreferred;

    VTKM_CONT
    CountSetBitsFunctor(const BitsPortal& portal, GlobalPopCountType* globalPopCount)
      : Portal{ portal }
      , GlobalPopCount{ globalPopCount }
      , FinalWordIndex{ portal.GetNumberOfWords() - 1 }
      , FinalWordMask{ portal.GetFinalWordMask() }
    {
    }

    ~CountSetBitsFunctor() {}

    VTKM_CONT void Initialize()
    {
      assert(this->GlobalPopCount != nullptr);
      VTKM_CUDA_CALL(cudaMemset(this->GlobalPopCount, 0, sizeof(GlobalPopCountType)));
    }

    VTKM_SUPPRESS_EXEC_WARNINGS
    __device__ void operator()(vtkm::Id wordIdx) const
    {
      Word word = this->Portal.GetWord(wordIdx);

      // The last word may be partial -- mask out trailing bits if needed.
      const Word mask = wordIdx == this->FinalWordIndex ? this->FinalWordMask : ~Word{ 0 };

      word &= mask;

      if (word != 0)
      {
        this->LocalPopCount = vtkm::CountSetBits(word);
        this->Reduce();
      }
    }

    VTKM_CONT vtkm::Id Finalize() const
    {
      assert(this->GlobalPopCount != nullptr);
      GlobalPopCountType result;
      VTKM_CUDA_CALL(cudaMemcpy(
        &result, this->GlobalPopCount, sizeof(GlobalPopCountType), cudaMemcpyDeviceToHost));
      return static_cast<vtkm::Id>(result);
    }

  private:
    // Every thread with a non-zero local popcount calls this function, which
    // computes the total popcount for the coalesced threads and atomically
    // increasing the global popcount.
    VTKM_SUPPRESS_EXEC_WARNINGS
    __device__ void Reduce() const
    {
      const auto activeLanes = cooperative_groups::coalesced_threads();
      const int activeRank = activeLanes.thread_rank();
      const int activeSize = activeLanes.size();

      // Reduction value:
      vtkm::Int32 rVal = this->LocalPopCount;
      for (int delta = 1; delta < activeSize; delta *= 2)
      {
        const vtkm::Int32 shflVal = activeLanes.shfl_down(rVal, delta);
        if (activeRank + delta < activeSize)
        {
          rVal += shflVal;
        }
      }

      if (activeRank == 0)
      {
        atomicAdd(this->GlobalPopCount, static_cast<GlobalPopCountType>(rVal));
      }
    }

    const BitsPortal Portal;
    GlobalPopCountType* GlobalPopCount;
    mutable vtkm::Int32 LocalPopCount{ 0 };
    // Used to mask trailing bits the in last word.
    vtkm::Id FinalWordIndex{ 0 };
    Word FinalWordMask{ 0 };
  };

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
                            cuda::internal::IteratorBegin(input),
                            cuda::internal::IteratorEnd(input),
                            cuda::internal::IteratorBegin(values),
                            cuda::internal::IteratorEnd(values),
                            cuda::internal::IteratorBegin(output),
                            bop);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
      return ::thrust::reduce(ThrustCudaPolicyPerThread,
                              cuda::internal::IteratorBegin(input),
                              cuda::internal::IteratorEnd(input),
                              initialValue,
                              bop);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
    vtkm::cont::cuda::internal::CastPortal<InputPortal, BinaryFunctor> castPortal(input,
                                                                                  binary_functor);

    vtkm::exec::cuda::internal::WrappedBinaryOperator<T, BinaryFunctor> bop(binary_functor);

    try
    {
      return ::thrust::reduce(ThrustCudaPolicyPerThread,
                              cuda::internal::IteratorBegin(castPortal),
                              cuda::internal::IteratorEnd(castPortal),
                              initialValue,
                              bop);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
    auto keys_out_begin = cuda::internal::IteratorBegin(keys_output);
    auto values_out_begin = cuda::internal::IteratorBegin(values_output);

    ::thrust::pair<decltype(keys_out_begin), decltype(values_out_begin)> result_iterators;

    ::thrust::equal_to<typename KeysPortal::ValueType> binaryPredicate;

    using ValueType = typename ValuesPortal::ValueType;
    vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType, BinaryFunctor> bop(binary_functor);

    try
    {
      result_iterators = ::thrust::reduce_by_key(vtkm_cuda_policy(),
                                                 cuda::internal::IteratorBegin(keys),
                                                 cuda::internal::IteratorEnd(keys),
                                                 cuda::internal::IteratorBegin(values),
                                                 keys_out_begin,
                                                 values_out_begin,
                                                 binaryPredicate,
                                                 bop);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
      ::thrust::copy_n(
        ThrustCudaPolicyPerThread, cuda::internal::IteratorEnd(input) - 1, 1, sum.begin());

      vtkm::exec::cuda::internal::WrappedBinaryOperator<ValueType, BinaryFunctor> bop(binaryOp);

      auto end = ::thrust::exclusive_scan(ThrustCudaPolicyPerThread,
                                          cuda::internal::IteratorBegin(input),
                                          cuda::internal::IteratorEnd(input),
                                          cuda::internal::IteratorBegin(output),
                                          initialValue,
                                          bop);

      //Store the new value for the end of the array. This is done because
      //with items such as the transpose array it is unsafe to pass the
      //portal to the SumExclusiveScan
      ::thrust::copy_n(ThrustCudaPolicyPerThread, (end - 1), 1, sum.begin() + 1);

      //execute the binaryOp one last time on the device.
      cuda::internal::SumExclusiveScan<<<1, 1, 0, cudaStreamPerThread>>>(
        sum[0], sum[1], sum[2], bop);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
                                          cuda::internal::IteratorBegin(input),
                                          cuda::internal::IteratorEnd(input),
                                          cuda::internal::IteratorBegin(output),
                                          bop);

      ::thrust::copy_n(ThrustCudaPolicyPerThread, end - 1, 1, result.begin());
      return result[0];
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
                                      cuda::internal::IteratorBegin(keys),
                                      cuda::internal::IteratorEnd(keys),
                                      cuda::internal::IteratorBegin(values),
                                      cuda::internal::IteratorBegin(output),
                                      bpred,
                                      bop);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
                                      cuda::internal::IteratorBegin(keys),
                                      cuda::internal::IteratorEnd(keys),
                                      cuda::internal::IteratorBegin(values),
                                      cuda::internal::IteratorBegin(output),
                                      initValue,
                                      bpred,
                                      bop);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
      ::thrust::sort(vtkm_cuda_policy(),
                     cuda::internal::IteratorBegin(values),
                     cuda::internal::IteratorEnd(values),
                     bop);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
      ::thrust::sort_by_key(vtkm_cuda_policy(),
                            cuda::internal::IteratorBegin(keys),
                            cuda::internal::IteratorEnd(keys),
                            cuda::internal::IteratorBegin(values),
                            bop);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
    }
  }

  template <class ValuesPortal>
  VTKM_CONT static vtkm::Id UniquePortal(const ValuesPortal values)
  {
    try
    {
      auto begin = cuda::internal::IteratorBegin(values);
      auto newLast =
        ::thrust::unique(ThrustCudaPolicyPerThread, begin, cuda::internal::IteratorEnd(values));
      return static_cast<vtkm::Id>(::thrust::distance(begin, newLast));
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
      auto begin = cuda::internal::IteratorBegin(values);
      auto newLast = ::thrust::unique(
        ThrustCudaPolicyPerThread, begin, cuda::internal::IteratorEnd(values), bop);
      return static_cast<vtkm::Id>(::thrust::distance(begin, newLast));
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
                            cuda::internal::IteratorBegin(input),
                            cuda::internal::IteratorEnd(input),
                            cuda::internal::IteratorBegin(values),
                            cuda::internal::IteratorEnd(values),
                            cuda::internal::IteratorBegin(output));
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
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
                            cuda::internal::IteratorBegin(input),
                            cuda::internal::IteratorEnd(input),
                            cuda::internal::IteratorBegin(values),
                            cuda::internal::IteratorEnd(values),
                            cuda::internal::IteratorBegin(output),
                            bop);
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
    }
  }

  template <class InputPortal, class OutputPortal>
  VTKM_CONT static void UpperBoundsPortal(const InputPortal& input,
                                          const OutputPortal& values_output)
  {
    try
    {
      ::thrust::upper_bound(ThrustCudaPolicyPerThread,
                            cuda::internal::IteratorBegin(input),
                            cuda::internal::IteratorEnd(input),
                            cuda::internal::IteratorBegin(values_output),
                            cuda::internal::IteratorEnd(values_output),
                            cuda::internal::IteratorBegin(values_output));
    }
    catch (...)
    {
      cuda::internal::throwAsVTKmException();
    }
  }

  template <typename GlobalPopCountType, typename BitsPortal, typename IndicesPortal>
  VTKM_CONT static vtkm::Id BitFieldToUnorderedSetPortal(const BitsPortal& bits,
                                                         const IndicesPortal& indices)
  {
    using Functor = BitFieldToUnorderedSetFunctor<BitsPortal, IndicesPortal, GlobalPopCountType>;

    // RAII for the global atomic counter.
    auto globalCount = cuda::internal::make_CudaUniquePtr<GlobalPopCountType>(1);
    Functor functor{ bits, indices, globalCount.get() };

    functor.Initialize();
    Schedule(functor, bits.GetNumberOfWords());
    Synchronize(); // Ensure kernel is done before checking final atomic count
    return functor.Finalize();
  }

  template <typename GlobalPopCountType, typename BitsPortal>
  VTKM_CONT static vtkm::Id CountSetBitsPortal(const BitsPortal& bits)
  {
    using Functor = CountSetBitsFunctor<BitsPortal, GlobalPopCountType>;

    // RAII for the global atomic counter.
    auto globalCount = cuda::internal::make_CudaUniquePtr<GlobalPopCountType>(1);
    Functor functor{ bits, globalCount.get() };

    functor.Initialize();
    Schedule(functor, bits.GetNumberOfWords());
    Synchronize(); // Ensure kernel is done before checking final atomic count
    return functor.Finalize();
  }

  //-----------------------------------------------------------------------------

public:
  template <typename IndicesStorage>
  VTKM_CONT static vtkm::Id BitFieldToUnorderedSet(
    const vtkm::cont::BitField& bits,
    vtkm::cont::ArrayHandle<Id, IndicesStorage>& indices)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id numBits = bits.GetNumberOfBits();
    auto bitsPortal = bits.PrepareForInput(DeviceAdapterTagCuda{});
    auto indicesPortal = indices.PrepareForOutput(numBits, DeviceAdapterTagCuda{});

    // Use a uint64 for accumulator, as atomicAdd does not support signed int64.
    numBits = BitFieldToUnorderedSetPortal<vtkm::UInt64>(bitsPortal, indicesPortal);

    indices.Shrink(numBits);
    return numBits;
  }

  template <typename T, typename U, class SIn, class SOut>
  VTKM_CONT static void Copy(const vtkm::cont::ArrayHandle<T, SIn>& input,
                             vtkm::cont::ArrayHandle<U, SOut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id inSize = input.GetNumberOfValues();
    if (inSize <= 0)
    {
      output.Shrink(inSize);
      return;
    }
    CopyPortal(input.PrepareForInput(DeviceAdapterTagCuda()),
               output.PrepareForOutput(inSize, DeviceAdapterTagCuda()));
  }

  template <typename T, typename U, class SIn, class SStencil, class SOut>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<U, SIn>& input,
                               const vtkm::cont::ArrayHandle<T, SStencil>& stencil,
                               vtkm::cont::ArrayHandle<U, SOut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id size = stencil.GetNumberOfValues();
    if (size <= 0)
    {
      output.Shrink(size);
      return;
    }

    vtkm::Id newSize = CopyIfPortal(input.PrepareForInput(DeviceAdapterTagCuda()),
                                    stencil.PrepareForInput(DeviceAdapterTagCuda()),
                                    output.PrepareForOutput(size, DeviceAdapterTagCuda()),
                                    ::vtkm::NotZeroInitialized()); //yes on the stencil
    output.Shrink(newSize);
  }

  template <typename T, typename U, class SIn, class SStencil, class SOut, class UnaryPredicate>
  VTKM_CONT static void CopyIf(const vtkm::cont::ArrayHandle<U, SIn>& input,
                               const vtkm::cont::ArrayHandle<T, SStencil>& stencil,
                               vtkm::cont::ArrayHandle<U, SOut>& output,
                               UnaryPredicate unary_predicate)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id size = stencil.GetNumberOfValues();
    if (size <= 0)
    {
      output.Shrink(size);
      return;
    }
    vtkm::Id newSize = CopyIfPortal(input.PrepareForInput(DeviceAdapterTagCuda()),
                                    stencil.PrepareForInput(DeviceAdapterTagCuda()),
                                    output.PrepareForOutput(size, DeviceAdapterTagCuda()),
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
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

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
    CopySubRangePortal(input.PrepareForInput(DeviceAdapterTagCuda()),
                       inputStartIndex,
                       numberOfElementsToCopy,
                       output.PrepareForInPlace(DeviceAdapterTagCuda()),
                       outputIndex);
    return true;
  }

  VTKM_CONT static vtkm::Id CountSetBits(const vtkm::cont::BitField& bits)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);
    auto bitsPortal = bits.PrepareForInput(DeviceAdapterTagCuda{});
    // Use a uint64 for accumulator, as atomicAdd does not support signed int64.
    return CountSetBitsPortal<vtkm::UInt64>(bitsPortal);
  }

  template <typename T, class SIn, class SVal, class SOut>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                    const vtkm::cont::ArrayHandle<T, SVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id numberOfValues = values.GetNumberOfValues();
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda()),
                      values.PrepareForInput(DeviceAdapterTagCuda()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()));
  }

  template <typename T, class SIn, class SVal, class SOut, class BinaryCompare>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                    const vtkm::cont::ArrayHandle<T, SVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& output,
                                    BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id numberOfValues = values.GetNumberOfValues();
    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda()),
                      values.PrepareForInput(DeviceAdapterTagCuda()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()),
                      binary_compare);
  }

  template <class SIn, class SOut>
  VTKM_CONT static void LowerBounds(const vtkm::cont::ArrayHandle<vtkm::Id, SIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& values_output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    LowerBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda()),
                      values_output.PrepareForInPlace(DeviceAdapterTagCuda()));
  }

  template <typename T, typename U, class SIn>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, SIn>& input, U initialValue)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      return initialValue;
    }
    return ReducePortal(input.PrepareForInput(DeviceAdapterTagCuda()), initialValue);
  }

  template <typename T, typename U, class SIn, class BinaryFunctor>
  VTKM_CONT static U Reduce(const vtkm::cont::ArrayHandle<T, SIn>& input,
                            U initialValue,
                            BinaryFunctor binary_functor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      return initialValue;
    }
    return ReducePortal(
      input.PrepareForInput(DeviceAdapterTagCuda()), initialValue, binary_functor);
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
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    //there is a concern that by default we will allocate too much
    //space for the keys/values output. 1 option is to
    const vtkm::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      return;
    }
    vtkm::Id reduced_size =
      ReduceByKeyPortal(keys.PrepareForInput(DeviceAdapterTagCuda()),
                        values.PrepareForInput(DeviceAdapterTagCuda()),
                        keys_output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()),
                        values_output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()),
                        binary_functor);

    keys_output.Shrink(reduced_size);
    values_output.Shrink(reduced_size);
  }

  template <typename T, class SIn, class SOut>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                   vtkm::cont::ArrayHandle<T, SOut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTagCuda());
      return vtkm::TypeTraits<T>::ZeroInitialization();
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagCuda());
    return ScanExclusivePortal(inputPortal,
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()));
  }

  template <typename T, class SIn, class SOut, class BinaryFunctor>
  VTKM_CONT static T ScanExclusive(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                   vtkm::cont::ArrayHandle<T, SOut>& output,
                                   BinaryFunctor binary_functor,
                                   const T& initialValue)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTagCuda());
      return vtkm::TypeTraits<T>::ZeroInitialization();
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagCuda());
    return ScanExclusivePortal(inputPortal,
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()),
                               binary_functor,
                               initialValue);
  }

  template <typename T, class SIn, class SOut>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                   vtkm::cont::ArrayHandle<T, SOut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTagCuda());
      return vtkm::TypeTraits<T>::ZeroInitialization();
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagCuda());
    return ScanInclusivePortal(inputPortal,
                               output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()));
  }

  template <typename T, class SIn, class SOut, class BinaryFunctor>
  VTKM_CONT static T ScanInclusive(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                   vtkm::cont::ArrayHandle<T, SOut>& output,
                                   BinaryFunctor binary_functor)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfValues = input.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTagCuda());
      return vtkm::TypeTraits<T>::ZeroInitialization();
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto inputPortal = input.PrepareForInput(DeviceAdapterTagCuda());
    return ScanInclusivePortal(
      inputPortal, output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()), binary_functor);
  }

  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanInclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTagCuda());
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTagCuda());
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTagCuda());
    ScanInclusiveByKeyPortal(
      keysPortal, valuesPortal, output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()));
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
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTagCuda());
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTagCuda());
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTagCuda());
    ScanInclusiveByKeyPortal(keysPortal,
                             valuesPortal,
                             output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()),
                             ::thrust::equal_to<T>(),
                             binary_functor);
  }

  template <typename T, typename U, typename KIn, typename VIn, typename VOut>
  VTKM_CONT static void ScanExclusiveByKey(const vtkm::cont::ArrayHandle<T, KIn>& keys,
                                           const vtkm::cont::ArrayHandle<U, VIn>& values,
                                           vtkm::cont::ArrayHandle<U, VOut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTagCuda());
      return;
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTagCuda());
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTagCuda());
    ScanExclusiveByKeyPortal(keysPortal,
                             valuesPortal,
                             output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()),
                             vtkm::TypeTraits<T>::ZeroInitialization(),
                             ::thrust::equal_to<T>(),
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
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    const vtkm::Id numberOfValues = keys.GetNumberOfValues();
    if (numberOfValues <= 0)
    {
      output.PrepareForOutput(0, DeviceAdapterTagCuda());
      return;
    }

    //We need call PrepareForInput on the input argument before invoking a
    //function. The order of execution of parameters of a function is undefined
    //so we need to make sure input is called before output, or else in-place
    //use case breaks.
    auto keysPortal = keys.PrepareForInput(DeviceAdapterTagCuda());
    auto valuesPortal = values.PrepareForInput(DeviceAdapterTagCuda());
    ScanExclusiveByKeyPortal(keysPortal,
                             valuesPortal,
                             output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()),
                             initialValue,
                             ::thrust::equal_to<T>(),
                             binary_functor);
  }

  // we use cuda pinned memory to reduce the amount of synchronization
  // and mem copies between the host and device.
  struct VTKM_CONT_EXPORT PinnedErrorArray
  {
    char* HostPtr = nullptr;
    char* DevicePtr = nullptr;
    vtkm::Id Size = 0;
  };

  VTKM_CONT_EXPORT
  static const PinnedErrorArray& GetPinnedErrorArray();

  VTKM_CONT_EXPORT
  static void CheckForErrors(); // throws vtkm::cont::ErrorExecution

  VTKM_CONT_EXPORT
  static void SetupErrorBuffer(vtkm::exec::cuda::internal::TaskStrided& functor);

  VTKM_CONT_EXPORT
  static void GetBlocksAndThreads(vtkm::UInt32& blocks,
                                  vtkm::UInt32& threadsPerBlock,
                                  vtkm::Id size);

  VTKM_CONT_EXPORT
  static void GetBlocksAndThreads(vtkm::UInt32& blocks, dim3& threadsPerBlock, const dim3& size);

  VTKM_CONT_EXPORT
  static void LogKernelLaunch(const cudaFuncAttributes& func_attrs,
                              const std::type_info& worklet_info,
                              vtkm::UInt32 blocks,
                              vtkm::UInt32 threadsPerBlock,
                              vtkm::Id size);

  VTKM_CONT_EXPORT
  static void LogKernelLaunch(const cudaFuncAttributes& func_attrs,
                              const std::type_info& worklet_info,
                              vtkm::UInt32 blocks,
                              dim3 threadsPerBlock,
                              const dim3& size);

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

    CheckForErrors();
    SetupErrorBuffer(functor);

    vtkm::UInt32 blocks, threadsPerBlock;
    GetBlocksAndThreads(blocks, threadsPerBlock, numInstances);

#ifdef VTKM_ENABLE_LOGGING
    if (GetStderrLogLevel() >= vtkm::cont::LogLevel::KernelLaunches)
    {
      using FunctorType = vtkm::exec::cuda::internal::TaskStrided1D<WType, IType>;
      cudaFuncAttributes empty_kernel_attrs;
      VTKM_CUDA_CALL(cudaFuncGetAttributes(&empty_kernel_attrs,
                                           cuda::internal::TaskStrided1DLaunch<FunctorType>));
      LogKernelLaunch(empty_kernel_attrs, typeid(WType), blocks, threadsPerBlock, numInstances);
    }
#endif

    cuda::internal::TaskStrided1DLaunch<<<blocks, threadsPerBlock, 0, cudaStreamPerThread>>>(
      functor, numInstances);
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

    CheckForErrors();
    SetupErrorBuffer(functor);

    const dim3 ranges(static_cast<vtkm::UInt32>(rangeMax[0]),
                      static_cast<vtkm::UInt32>(rangeMax[1]),
                      static_cast<vtkm::UInt32>(rangeMax[2]));

    vtkm::UInt32 blocks;
    dim3 threadsPerBlock;
    GetBlocksAndThreads(blocks, threadsPerBlock, ranges);

#ifdef VTKM_ENABLE_LOGGING
    if (GetStderrLogLevel() >= vtkm::cont::LogLevel::KernelLaunches)
    {
      using FunctorType = vtkm::exec::cuda::internal::TaskStrided3D<WType, IType>;
      cudaFuncAttributes empty_kernel_attrs;
      VTKM_CUDA_CALL(cudaFuncGetAttributes(&empty_kernel_attrs,
                                           cuda::internal::TaskStrided3DLaunch<FunctorType>));
      LogKernelLaunch(empty_kernel_attrs, typeid(WType), blocks, threadsPerBlock, ranges);
    }
#endif

    cuda::internal::TaskStrided3DLaunch<<<blocks, threadsPerBlock, 0, cudaStreamPerThread>>>(
      functor, ranges);
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, vtkm::Id numInstances)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::exec::cuda::internal::TaskStrided1D<Functor, vtkm::internal::NullType> kernel(functor);

    ScheduleTask(kernel, numInstances);
  }

  template <class Functor>
  VTKM_CONT static void Schedule(Functor functor, const vtkm::Id3& rangeMax)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::exec::cuda::internal::TaskStrided3D<Functor, vtkm::internal::NullType> kernel(functor);
    ScheduleTask(kernel, rangeMax);
  }

  template <typename T, class Storage>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    SortPortal(values.PrepareForInPlace(DeviceAdapterTagCuda()));
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Sort(vtkm::cont::ArrayHandle<T, Storage>& values,
                             BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    SortPortal(values.PrepareForInPlace(DeviceAdapterTagCuda()), binary_compare);
  }

  template <typename T, typename U, class StorageT, class StorageU>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTagCuda()),
                    values.PrepareForInPlace(DeviceAdapterTagCuda()));
  }

  template <typename T, typename U, class StorageT, class StorageU, class BinaryCompare>
  VTKM_CONT static void SortByKey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                                  vtkm::cont::ArrayHandle<U, StorageU>& values,
                                  BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    SortByKeyPortal(keys.PrepareForInPlace(DeviceAdapterTagCuda()),
                    values.PrepareForInPlace(DeviceAdapterTagCuda()),
                    binary_compare);
  }

  template <typename T, class Storage>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id newSize = UniquePortal(values.PrepareForInPlace(DeviceAdapterTagCuda()));

    values.Shrink(newSize);
  }

  template <typename T, class Storage, class BinaryCompare>
  VTKM_CONT static void Unique(vtkm::cont::ArrayHandle<T, Storage>& values,
                               BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id newSize =
      UniquePortal(values.PrepareForInPlace(DeviceAdapterTagCuda()), binary_compare);

    values.Shrink(newSize);
  }

  template <typename T, class SIn, class SVal, class SOut>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                    const vtkm::cont::ArrayHandle<T, SVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id numberOfValues = values.GetNumberOfValues();
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda()),
                      values.PrepareForInput(DeviceAdapterTagCuda()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()));
  }

  template <typename T, class SIn, class SVal, class SOut, class BinaryCompare>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<T, SIn>& input,
                                    const vtkm::cont::ArrayHandle<T, SVal>& values,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& output,
                                    BinaryCompare binary_compare)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    vtkm::Id numberOfValues = values.GetNumberOfValues();
    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda()),
                      values.PrepareForInput(DeviceAdapterTagCuda()),
                      output.PrepareForOutput(numberOfValues, DeviceAdapterTagCuda()),
                      binary_compare);
  }

  template <class SIn, class SOut>
  VTKM_CONT static void UpperBounds(const vtkm::cont::ArrayHandle<vtkm::Id, SIn>& input,
                                    vtkm::cont::ArrayHandle<vtkm::Id, SOut>& values_output)
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    UpperBoundsPortal(input.PrepareForInput(DeviceAdapterTagCuda()),
                      values_output.PrepareForInPlace(DeviceAdapterTagCuda()));
  }

  VTKM_CONT static void Synchronize()
  {
    VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

    VTKM_CUDA_CALL(cudaStreamSynchronize(cudaStreamPerThread));
    CheckForErrors();
  }
};

template <>
class DeviceTaskTypes<vtkm::cont::DeviceAdapterTagCuda>
{
public:
  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::cuda::internal::TaskStrided1D<WorkletType, InvocationType> MakeTask(
    WorkletType& worklet,
    InvocationType& invocation,
    vtkm::Id,
    vtkm::Id globalIndexOffset = 0)
  {
    using Task = vtkm::exec::cuda::internal::TaskStrided1D<WorkletType, InvocationType>;
    return Task(worklet, invocation, globalIndexOffset);
  }

  template <typename WorkletType, typename InvocationType>
  static vtkm::exec::cuda::internal::TaskStrided3D<WorkletType, InvocationType> MakeTask(
    WorkletType& worklet,
    InvocationType& invocation,
    vtkm::Id3,
    vtkm::Id globalIndexOffset = 0)
  {
    using Task = vtkm::exec::cuda::internal::TaskStrided3D<WorkletType, InvocationType>;
    return Task(worklet, invocation, globalIndexOffset);
  }
};
}
} // namespace vtkm::cont

#endif //vtk_m_cont_cuda_internal_DeviceAdapterAlgorithmCuda_h
