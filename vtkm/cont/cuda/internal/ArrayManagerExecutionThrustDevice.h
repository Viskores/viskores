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
#include <vtkm/cont/ErrorControlOutOfMemory.h>
#include <vtkm/cont/Storage.h>

// Disable warnings we check vtkm for but Thrust does not.
VTKM_BOOST_PRE_INCLUDE
#include <thrust/system/cuda/vector.h>
#include <thrust/device_malloc_allocator.h>
#include <thrust/copy.h>
VTKM_BOOST_POST_INCLUDE

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
/// for the cuda backend which has separate memory spaces for host and device.
/// This implementation contains a thrust::system::cuda::vector to allocate
/// and manage the array.
///
template<typename T, class StorageTag>
class ArrayManagerExecutionThrustDevice
{
public:
  typedef T ValueType;
  typedef typename thrust::system::cuda::pointer<T>::difference_type difference_type;

  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

  typedef vtkm::exec::cuda::internal::ArrayPortalFromThrust< T > PortalType;
  typedef vtkm::exec::cuda::internal::ConstArrayPortalFromThrust< const T > PortalConstType;

  VTKM_CONT_EXPORT
  ArrayManagerExecutionThrustDevice(StorageType *storage)
    : Storage(storage), Array()
  {

  }

  VTKM_CONT_EXPORT
  ~ArrayManagerExecutionThrustDevice()
  {
    this->ReleaseResources();
  }

  /// Returns the size of the array.
  ///
  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    return static_cast<vtkm::Id>(this->Array.size());
  }

  /// Allocates the appropriate size of the array and copies the given data
  /// into the array.
  ///
  VTKM_CONT_EXPORT
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


    return PortalConstType(this->Array.data(),
                           this->Array.data() + static_cast<difference_type>(this->Array.size()));
  }

  /// Allocates the appropriate size of the array and copies the given data
  /// into the array.
  ///
  VTKM_CONT_EXPORT
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

    return PortalType(this->Array.data(),
                      this->Array.data() + static_cast<difference_type>(this->Array.size()));
  }

  /// Allocates the array to the given size.
  ///
  VTKM_CONT_EXPORT
  PortalType PrepareForOutput(vtkm::Id numberOfValues)
  {
    if (numberOfValues > this->GetNumberOfValues())
    {
      // Resize to 0 first so that you don't have to copy data when resizing
      // to a larger size.
      this->Array.clear();
    }

    try
      {
      this->Array.resize(static_cast<vtkm::UInt64>(numberOfValues));
      }
    catch (std::bad_alloc error)
      {
      throw vtkm::cont::ErrorControlOutOfMemory(error.what());
      }

    return PortalType(this->Array.data(),
                      this->Array.data() + static_cast<difference_type>(this->Array.size()));
  }

  /// Allocates enough space in \c storage and copies the data in the
  /// device vector into it.
  ///
  VTKM_CONT_EXPORT
  void RetrieveOutputData(StorageType *storage) const
  {
    storage->Allocate(static_cast<vtkm::Id>(this->Array.size()));
    ::thrust::copy(
          this->Array.data(),
          this->Array.data() + static_cast<difference_type>(this->Array.size()),
          vtkm::cont::ArrayPortalToIteratorBegin(storage->GetPortal()));
  }

  /// Resizes the device vector.
  ///
  VTKM_CONT_EXPORT void Shrink(vtkm::Id numberOfValues)
  {
    // The operation will succeed even if this assertion fails, but this
    // is still supposed to be a precondition to Shrink.
    VTKM_ASSERT_CONT(numberOfValues <= static_cast<vtkm::Id>(this->Array.size()));

    this->Array.resize(static_cast<vtkm::UInt64>(numberOfValues));
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

  StorageType *Storage;

  ::thrust::system::cuda::vector<ValueType,
                                 UninitializedAllocator<ValueType> > Array;

  VTKM_CONT_EXPORT
  void CopyToExecution()
  {
    try
    {
      this->Array.assign(
            vtkm::cont::ArrayPortalToIteratorBegin(this->Storage->GetPortalConst()),
            vtkm::cont::ArrayPortalToIteratorEnd(this->Storage->GetPortalConst()));
    }
    catch (std::bad_alloc error)
    {
      throw vtkm::cont::ErrorControlOutOfMemory(error.what());
    }
  }
};


}
}
}
} // namespace vtkm::cont::cuda::internal

#endif // vtk_m_cont_cuda_internal_ArrayManagerExecutionThrustDevice_h
