//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h
#define vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h

#include <vtkm/cont/cuda/ErrorCuda.h>
#include <vtkm/cont/cuda/internal/CudaAllocator.h>
#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/cuda/internal/ThrustExceptionHandler.h>
#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>

#include <vtkm/cont/internal/ArrayExportMacros.h>
#include <vtkm/cont/internal/ArrayManagerExecution.h>

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/Storage.h>

//This is in a separate header so that ArrayHandleBasicImpl can include
//the interface without getting any CUDA headers
#include <vtkm/cont/cuda/internal/ExecutionArrayInterfaceBasicCuda.h>

#include <vtkm/exec/cuda/internal/ThrustPatches.h>
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/copy.h>
#include <thrust/device_ptr.h>
VTKM_THIRDPARTY_POST_INCLUDE

#include <limits>

// These must be placed in the vtkm::cont::internal namespace so that
// the template can be found.

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename T, class StorageTag>
class ArrayManagerExecution<T, StorageTag, vtkm::cont::DeviceAdapterTagCuda>
{
public:
  using ValueType = T;
  using PointerType = T*;
  using PortalType = vtkm::exec::cuda::internal::ArrayPortalFromThrust<T>;
  using PortalConstType = vtkm::exec::cuda::internal::ConstArrayPortalFromThrust<T>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTag>;
  using difference_type = std::ptrdiff_t;

  VTKM_CONT
  ArrayManagerExecution(StorageType* storage)
    : Storage(storage)
    , Begin(nullptr)
    , End(nullptr)
    , Capacity(nullptr)
  {
  }

  VTKM_CONT
  ~ArrayManagerExecution() { this->ReleaseResources(); }

  /// Returns the size of the array.
  ///
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return static_cast<vtkm::Id>(this->End - this->Begin); }

  VTKM_CONT
  PortalConstType PrepareForInput(bool updateData)
  {
    try
    {
      if (updateData)
      {
        this->CopyToExecution();
      }

      return PortalConstType(this->Begin, this->End);
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      // Thrust does not seem to be clearing the CUDA error, so do it here.
      cudaError_t cudaError = cudaPeekAtLastError();
      if (cudaError == cudaErrorMemoryAllocation)
      {
        cudaGetLastError();
      }
      throw error;
    }
  }

  VTKM_CONT
  PortalType PrepareForInPlace(bool updateData)
  {
    try
    {
      if (updateData)
      {
        this->CopyToExecution();
      }

      return PortalType(this->Begin, this->End);
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      // Thrust does not seem to be clearing the CUDA error, so do it here.
      cudaError_t cudaError = cudaPeekAtLastError();
      if (cudaError == cudaErrorMemoryAllocation)
      {
        cudaGetLastError();
      }
      throw error;
    }
  }

  VTKM_CONT
  PortalType PrepareForOutput(vtkm::Id numberOfValues)
  {
    try
    {
      // Can we reuse the existing buffer?
      vtkm::Id curCapacity =
        this->Begin != nullptr ? static_cast<vtkm::Id>(this->Capacity - this->Begin) : 0;

      // Just mark a new end if we don't need to increase the allocation:
      if (curCapacity >= numberOfValues)
      {
        this->End = this->Begin + static_cast<difference_type>(numberOfValues);

        return PortalType(this->Begin, this->End);
      }

      const std::size_t maxNumVals = (std::numeric_limits<std::size_t>::max() / sizeof(ValueType));

      if (static_cast<std::size_t>(numberOfValues) > maxNumVals)
      {
        VTKM_LOG_F(vtkm::cont::LogLevel::MemExec,
                   "Refusing to allocate CUDA memory; number of values (%llu) exceeds "
                   "std::size_t capacity.",
                   static_cast<vtkm::UInt64>(numberOfValues));

        std::ostringstream err;
        err << "Failed to allocate " << numberOfValues << " values on device: "
            << "Number of bytes is not representable by std::size_t.";
        throw vtkm::cont::ErrorBadAllocation(err.str());
      }

      this->ReleaseResources();

      const std::size_t bufferSize = static_cast<std::size_t>(numberOfValues) * sizeof(ValueType);

      // Attempt to allocate:
      try
      {
        this->Begin =
          static_cast<ValueType*>(vtkm::cont::cuda::internal::CudaAllocator::Allocate(bufferSize));
      }
      catch (const std::exception& error)
      {
        std::ostringstream err;
        err << "Failed to allocate " << bufferSize << " bytes on device: " << error.what();
        throw vtkm::cont::ErrorBadAllocation(err.str());
      }

      this->Capacity = this->Begin + static_cast<difference_type>(numberOfValues);
      this->End = this->Capacity;

      return PortalType(this->Begin, this->End);
    }
    catch (vtkm::cont::ErrorBadAllocation& error)
    {
      // Thrust does not seem to be clearing the CUDA error, so do it here.
      cudaError_t cudaError = cudaPeekAtLastError();
      if (cudaError == cudaErrorMemoryAllocation)
      {
        cudaGetLastError();
      }
      throw error;
    }
  }

  /// Allocates enough space in \c storage and copies the data in the
  /// device vector into it.
  ///
  VTKM_CONT
  void RetrieveOutputData(StorageType* storage) const
  {
    storage->Allocate(this->GetNumberOfValues());

    VTKM_LOG_F(vtkm::cont::LogLevel::MemTransfer,
               "Copying CUDA dev --> host: %s",
               vtkm::cont::GetSizeString(this->End - this->Begin).c_str());

    try
    {
      ::thrust::copy(thrust::cuda::pointer<ValueType>(this->Begin),
                     thrust::cuda::pointer<ValueType>(this->End),
                     vtkm::cont::ArrayPortalToIteratorBegin(storage->GetPortal()));
    }
    catch (...)
    {
      vtkm::cont::cuda::internal::throwAsVTKmException();
    }
  }

  /// Resizes the device vector.
  ///
  VTKM_CONT void Shrink(vtkm::Id numberOfValues)
  {
    // The operation will succeed even if this assertion fails, but this
    // is still supposed to be a precondition to Shrink.
    VTKM_ASSERT(this->Begin != nullptr && this->Begin + numberOfValues <= this->End);

    this->End = this->Begin + static_cast<difference_type>(numberOfValues);
  }

  /// Frees all memory.
  ///
  VTKM_CONT void ReleaseResources()
  {
    if (this->Begin != nullptr)
    {
      vtkm::cont::cuda::internal::CudaAllocator::Free(this->Begin);
      this->Begin = nullptr;
      this->End = nullptr;
      this->Capacity = nullptr;
    }
  }

private:
  ArrayManagerExecution(ArrayManagerExecution&) = delete;
  void operator=(ArrayManagerExecution&) = delete;

  StorageType* Storage;

  PointerType Begin;
  PointerType End;
  PointerType Capacity;

  VTKM_CONT
  void CopyToExecution()
  {
    try
    {
      this->PrepareForOutput(this->Storage->GetNumberOfValues());

      VTKM_LOG_F(vtkm::cont::LogLevel::MemTransfer,
                 "Copying host --> CUDA dev: %s.",
                 vtkm::cont::GetSizeString(this->End - this->Begin).c_str());

      ::thrust::copy(vtkm::cont::ArrayPortalToIteratorBegin(this->Storage->GetPortalConst()),
                     vtkm::cont::ArrayPortalToIteratorEnd(this->Storage->GetPortalConst()),
                     thrust::cuda::pointer<ValueType>(this->Begin));
    }
    catch (...)
    {
      vtkm::cont::cuda::internal::throwAsVTKmException();
    }
  }
};

template <typename T>
struct ExecutionPortalFactoryBasic<T, DeviceAdapterTagCuda>
{
  using ValueType = T;
  using PortalType = vtkm::exec::cuda::internal::ArrayPortalFromThrust<ValueType>;
  using PortalConstType = vtkm::exec::cuda::internal::ConstArrayPortalFromThrust<ValueType>;

  VTKM_CONT
  static PortalType CreatePortal(ValueType* start, ValueType* end)
  {
    return PortalType(start, end);
  }

  VTKM_CONT
  static PortalConstType CreatePortalConst(const ValueType* start, const ValueType* end)
  {
    return PortalConstType(start, end);
  }
};

} // namespace internal

#ifndef vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_cu
VTKM_EXPORT_ARRAYHANDLES_FOR_DEVICE_ADAPTER(DeviceAdapterTagCuda)
#endif // !vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_cu
}
} // namespace vtkm::cont

#endif //vtk_m_cont_cuda_internal_ArrayManagerExecutionCuda_h
