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
#ifndef vtk_m_cont_cuda_internal_ArrayManagerExecutionThrustDevice_h
#define vtk_m_cont_cuda_internal_ArrayManagerExecutionThrustDevice_h

#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/Storage.h>

// Disable warnings we check vtkm for but Thrust does not.
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/copy.h>
#include <thrust/device_malloc_allocator.h>
#include <thrust/system/cuda/vector.h>

#include <thrust/system/cuda/execution_policy.h>

VTKM_THIRDPARTY_POST_INCLUDE

#include <vtkm/cont/cuda/ErrorCuda.h>
#include <vtkm/cont/cuda/internal/ThrustExceptionHandler.h>
#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>

#include <limits>

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

/// \c ArrayManagerExecutionThrustDevice provides an implementation for a \c
/// ArrayManagerExecution class for a thrust device adapter that is designed
/// for the cuda backend which has separate memory spaces for host and device.
/// This implementation contains a thrust::system::cuda::pointer to contain the
/// data.
template <typename T, class StorageTag>
class ArrayManagerExecutionThrustDevice
{
public:
  typedef T ValueType;
  typedef typename thrust::system::cuda::pointer<ValueType> PointerType;
  typedef typename PointerType::difference_type difference_type;

  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

  typedef vtkm::exec::cuda::internal::ArrayPortalFromThrust<T> PortalType;
  typedef vtkm::exec::cuda::internal::ConstArrayPortalFromThrust<T> PortalConstType;

  VTKM_CONT
  ArrayManagerExecutionThrustDevice(StorageType* storage)
    : Storage(storage)
    , Begin(static_cast<ValueType*>(nullptr))
    , End(static_cast<ValueType*>(nullptr))
    , Capacity(static_cast<ValueType*>(nullptr))
  {
  }

  VTKM_CONT
  ~ArrayManagerExecutionThrustDevice() { this->ReleaseResources(); }

  /// Returns the size of the array.
  ///
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    return static_cast<vtkm::Id>(this->End.get() - this->Begin.get());
  }

  /// Allocates the appropriate size of the array and copies the given data
  /// into the array.
  ///
  VTKM_CONT
  PortalConstType PrepareForInput(bool updateData)
  {
    if (updateData)
    {
      this->CopyToExecution();
    }
    else // !updateData
    {
      // The data in this->Array should already be valid.
    }

    return PortalConstType(this->Begin, this->End);
  }

  /// Workaround for nvcc 7.5 compiler warning bug.
  template <typename DummyType>
  VTKM_CONT PortalConstType _PrepareForInput(bool updateData)
  {
    return this->PrepareForInput(updateData);
  }

  /// Allocates the appropriate size of the array and copies the given data
  /// into the array.
  ///
  VTKM_CONT
  PortalType PrepareForInPlace(bool updateData)
  {
    if (updateData)
    {
      this->CopyToExecution();
    }
    else // !updateData
    {
      // The data in this->Array should already be valid.
    }

    return PortalType(this->Begin, this->End);
  }

  /// Workaround for nvcc 7.5 compiler warning bug.
  template <typename DummyType>
  VTKM_CONT PortalType _PrepareForInPlace(bool updateData)
  {
    return this->PrepareForInPlace(updateData);
  }

  /// Allocates the array to the given size.
  ///
  VTKM_CONT
  PortalType PrepareForOutput(vtkm::Id numberOfValues)
  {
    // Can we reuse the existing buffer?
    vtkm::Id curCapacity = this->Begin.get() != nullptr
      ? static_cast<vtkm::Id>(this->Capacity.get() - this->Begin.get())
      : 0;

    // Just mark a new end if we don't need to increase the allocation:
    if (curCapacity >= numberOfValues)
    {
      this->End = PointerType(this->Begin.get() + static_cast<difference_type>(numberOfValues));

      return PortalType(this->Begin, this->End);
    }

    const std::size_t maxNumVals = (std::numeric_limits<std::size_t>::max() / sizeof(ValueType));

    if (static_cast<std::size_t>(numberOfValues) > maxNumVals)
    {
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
      ValueType* tmp;
#ifdef VTKM_USE_UNIFIED_MEMORY
      int dev;
      VTKM_CUDA_CALL(cudaGetDevice(&dev));
      VTKM_CUDA_CALL(cudaMallocManaged(&tmp, bufferSize));
      VTKM_CUDA_CALL(cudaMemAdvise(tmp, bufferSize, cudaMemAdviseSetPreferredLocation, dev));
      VTKM_CUDA_CALL(cudaMemPrefetchAsync(tmp, bufferSize, dev, 0));
      VTKM_CUDA_CALL(cudaStreamSynchronize(0));
#else
      VTKM_CUDA_CALL(cudaMalloc(&tmp, bufferSize));
#endif
      this->Begin = PointerType(tmp);
    }
    catch (const std::exception& error)
    {
      std::ostringstream err;
      err << "Failed to allocate " << bufferSize << " bytes on device: " << error.what();
      throw vtkm::cont::ErrorBadAllocation(err.str());
    }

    this->Capacity = PointerType(this->Begin.get() + static_cast<difference_type>(numberOfValues));
    this->End = this->Capacity;

    return PortalType(this->Begin, this->End);
  }

  /// Workaround for nvcc 7.5 compiler warning bug.
  template <typename DummyType>
  VTKM_CONT PortalType _PrepareForOutput(vtkm::Id numberOfValues)
  {
    return this->PrepareForOutput(numberOfValues);
  }

  /// Allocates enough space in \c storage and copies the data in the
  /// device vector into it.
  ///
  VTKM_CONT
  void RetrieveOutputData(StorageType* storage) const
  {
    storage->Allocate(this->GetNumberOfValues());
    try
    {
#ifdef VTKM_USE_UNIFIED_MEMORY
      cudaDeviceSynchronize();
#endif
      ::thrust::copy(
        this->Begin, this->End, vtkm::cont::ArrayPortalToIteratorBegin(storage->GetPortal()));
    }
    catch (...)
    {
      vtkm::cont::cuda::internal::throwAsVTKmException();
    }
  }

  /// Copies the data currently in the device array into the given iterators.
  /// Although the iterator is supposed to be from the control environment,
  /// thrust can generally handle iterators for a device as well.
  ///
  template <class IteratorTypeControl>
  VTKM_CONT void CopyInto(IteratorTypeControl dest) const
  {
    ::thrust::copy(this->Begin, this->End, dest);
  }

  /// Resizes the device vector.
  ///
  VTKM_CONT void Shrink(vtkm::Id numberOfValues)
  {
    // The operation will succeed even if this assertion fails, but this
    // is still supposed to be a precondition to Shrink.
    VTKM_ASSERT(this->Begin.get() != nullptr &&
                this->Begin.get() + numberOfValues <= this->End.get());

    this->End = PointerType(this->Begin.get() + static_cast<difference_type>(numberOfValues));
  }

  /// Frees all memory.
  ///
  VTKM_CONT void ReleaseResources()
  {
    if (this->Begin.get() != nullptr)
    {
      VTKM_CUDA_CALL(cudaFree(this->Begin.get()));
      this->Begin = PointerType(static_cast<ValueType*>(nullptr));
      this->End = PointerType(static_cast<ValueType*>(nullptr));
      this->Capacity = PointerType(static_cast<ValueType*>(nullptr));
    }
  }

private:
  ArrayManagerExecutionThrustDevice(ArrayManagerExecutionThrustDevice<T, StorageTag>&) = delete;
  void operator=(ArrayManagerExecutionThrustDevice<T, StorageTag>&) = delete;

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
      ::thrust::copy(vtkm::cont::ArrayPortalToIteratorBegin(this->Storage->GetPortalConst()),
                     vtkm::cont::ArrayPortalToIteratorEnd(this->Storage->GetPortalConst()),
                     this->Begin);
    }
    catch (...)
    {
      vtkm::cont::cuda::internal::throwAsVTKmException();
    }
  }
};
}
}
}
} // namespace vtkm::cont::cuda::internal

#endif // vtk_m_cont_cuda_internal_ArrayManagerExecutionThrustDevice_h
