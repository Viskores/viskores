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
#ifndef vtk_m_cont_cuda_internal_ArrayManagerExecutionThrustDevice_h
#define vtk_m_cont_cuda_internal_ArrayManagerExecutionThrustDevice_h

#include <vtkm/cont/Storage.h>
#include <vtkm/cont/ErrorControlOutOfMemory.h>

#include <iostream>

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

#include <thrust/system/cuda/vector.h>
#include <thrust/device_malloc_allocator.h>
#include <thrust/copy.h>

#if defined(__GNUC__) && !defined(VTKM_CUDA)
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif // gcc version >= 4.6
#endif // gcc && !CUDA

#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>

namespace vtkm {
namespace cont {
namespace cuda {
namespace internal {

// UninitializedAllocator is an allocator which
// derives from device_allocator and which has a
// no-op construct member function
template<typename T>
  struct UninitializedAllocator
    : ::thrust::device_malloc_allocator<T>
{
  // note that construct is annotated as
  // a __host__ __device__ function
  __host__ __device__
  void construct(T *p)
  {
    // no-op
  }
};

/// \c ArrayManagerExecutionThrustDevice provides an implementation for a \c
/// ArrayManagerExecution class for a thrust device adapter that is designed
/// for the cuda backend which has separate memory spaces for host and device. This
/// implementation contains a ::thrust::system::cuda::vector to allocate and manage
/// the array.
///
/// This array manager should only be used with the cuda device adapter,
/// since in the future it will take advantage of texture memory and
/// the unique memory access patterns of cuda systems.
template<typename T, class StorageTag>
class ArrayManagerExecutionThrustDevice
{
public:
  typedef T ValueType;

  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> ContainerType;

  typedef vtkm::exec::cuda::internal::ArrayPortalFromThrust< T > PortalType;
  typedef vtkm::exec::cuda::internal::ConstArrayPortalFromThrust< const T > PortalConstType;

  VTKM_CONT_EXPORT ArrayManagerExecutionThrustDevice():
    Array()
  {

  }

  ~ArrayManagerExecutionThrustDevice()
  {
    this->ReleaseResources();
  }

  /// Returns the size of the array.
  ///
  VTKM_CONT_EXPORT vtkm::Id GetNumberOfValues() const {
    return this->Array.size();
  }

  /// Allocates the appropriate size of the array and copies the given data
  /// into the array.
  ///
  template<class PortalControl>
  VTKM_CONT_EXPORT void LoadDataForInput(PortalControl arrayPortal)
  {
    //don't bind to the texture yet, as we could have allocate the array
    //on a previous call with AllocateArrayForOutput and now are directly
    //calling get portal const
    try
      {
      this->Array.assign(arrayPortal.GetRawIterator(),
                         arrayPortal.GetRawIterator() + arrayPortal.GetNumberOfValues());
      }
    catch (std::bad_alloc error)
      {
      throw vtkm::cont::ErrorControlOutOfMemory(error.what());
      }
  }

  /// Allocates the appropriate size of the array and copies the given data
  /// into the array.
  ///
  template<class PortalControl>
  VTKM_CONT_EXPORT void LoadDataForInPlace(PortalControl arrayPortal)
  {
    this->LoadDataForInput(arrayPortal);
  }

  /// Allocates the array to the given size.
  ///
  VTKM_CONT_EXPORT void AllocateArrayForOutput(
      ContainerType &vtkmNotUsed(container),
      vtkm::Id numberOfValues)
  {
    try
      {
      this->Array.resize(numberOfValues);
      }
    catch (std::bad_alloc error)
      {
      throw vtkm::cont::ErrorControlOutOfMemory(error.what());
      }


  }

  /// Allocates enough space in \c controlArray and copies the data in the
  /// device vector into it.
  ///
  VTKM_CONT_EXPORT void RetrieveOutputData(ContainerType &controlArray) const
  {
    controlArray.Allocate(this->Array.size());
    ::thrust::copy( this->Array.data(),
                    this->Array.data() + this->Array.size(),
                   controlArray.GetPortal().GetRawIterator());
  }

  /// Resizes the device vector.
  ///
  VTKM_CONT_EXPORT void Shrink(vtkm::Id numberOfValues)
  {
    // The operation will succeed even if this assertion fails, but this
    // is still supposed to be a precondition to Shrink.
    VTKM_ASSERT_CONT(numberOfValues <= this->Array.size());

    this->Array.resize(numberOfValues);
  }

  VTKM_CONT_EXPORT PortalType GetPortal()
  {
    return PortalType( this->Array.data(),
                       this->Array.data() + this->Array.size());
  }

  VTKM_CONT_EXPORT PortalConstType GetPortalConst() const
  {
    return PortalConstType( this->Array.data(),
                            this->Array.data() + this->Array.size());
  }


  /// Frees all memory.
  ///
  VTKM_CONT_EXPORT void ReleaseResources()
  {
    this->Array.clear();
    this->Array.shrink_to_fit();
  }

private:
  // Not implemented
  ArrayManagerExecutionThrustDevice(
      ArrayManagerExecutionThrustDevice<T, StorageTag> &);
  void operator=(
      ArrayManagerExecutionThrustDevice<T, StorageTag> &);

  ::thrust::system::cuda::vector<ValueType,
                                 UninitializedAllocator<ValueType> > Array;
};


}
}
}
} // namespace vtkm::cont::cuda::internal

#endif // vtk_m_cont_cuda_internal_ArrayManagerExecutionThrustDevice_h
