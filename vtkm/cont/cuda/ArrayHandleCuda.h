//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_cont_cuda_ArrayHandleCuda_h
#define vtk_m_cont_cuda_ArrayHandleCuda_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/Storage.h>

#ifdef VTKM_CUDA
#include <vtkm/cont/cuda/internal/ArrayManagerExecutionThrustDevice.h>
#endif

VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/system/cuda/memory.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace cont
{
namespace cuda
{

struct VTKM_ALWAYS_EXPORT StorageTagCuda
{
};

} // namespace cuda
} // namespace cont
} // namespace vtkm

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

/// \brief An array portal for cuda arrays.
///
template <typename T>
class ArrayPortalCuda
{
public:
  typedef T ValueType;
  typedef thrust::system::cuda::pointer<ValueType> DevicePointer;

  VTKM_CONT
  ArrayPortalCuda()
    : Data()
    , NumberOfValues(0)
  {
  }

  VTKM_CONT
  ArrayPortalCuda(ValueType* d, vtkm::Id numberOfValues)
    : Data(d)
    , NumberOfValues(numberOfValues)
  {
  }

  VTKM_CONT
  ArrayPortalCuda(const DevicePointer& ptr, vtkm::Id numberOfValues)
    : Data(ptr)
    , NumberOfValues(numberOfValues)
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return NumberOfValues; }

  VTKM_CONT
  ValueType Get(vtkm::Id index) const
  {
    throw vtkm::cont::ErrorBadType("ArrayHandleCuda only provides access to the device pointer.");
  }

  VTKM_CONT
  void Set(vtkm::Id vtkmNotUsed(index), T vtkmNotUsed(value)) const
  {
    throw vtkm::cont::ErrorBadType("ArrayHandleCuda only provides access to the device pointer.");
  }

  VTKM_CONT
  DevicePointer GetDevicePointer() const { return Data; }
private:
  DevicePointer Data;
  vtkm::Id NumberOfValues;
};

} // namespace internal
} // namespace cuda
} // namespace cont
} // namespace vtkm

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename T>
class Storage<T, vtkm::cont::cuda::StorageTagCuda>
{
public:
  typedef T ValueType;
  typedef thrust::system::cuda::pointer<ValueType> DevicePointer;
  typedef vtkm::cont::cuda::internal::ArrayPortalCuda<ValueType> PortalType;
  typedef vtkm::cont::cuda::internal::ArrayPortalCuda<ValueType> PortalConstType;

  VTKM_CONT
  Storage()
    : Data()
    , NumberOfValues(0)
    , IsOwner(true)
  {
  }
  VTKM_CONT
  Storage(ValueType* d, vtkm::Id numberOfValues)
    : Data(d)
    , NumberOfValues(numberOfValues)
    , IsOwner(false)
  {
  }

  VTKM_CONT
  PortalType GetPortal() { return PortalType(this->Data, this->NumberOfValues); }

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->Data, this->NumberOfValues);
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_CONT
  void Allocate(vtkm::Id size)
  {
    if (!this->OwnsResources())
      throw vtkm::cont::ErrorBadAllocation(
        "ArrayHandleCuda does not own its internal device memory.");

    if (NumberOfValues != 0)
      this->ReleaseResources();
    this->Data = thrust::system::cuda::malloc<ValueType>(size);
    this->NumberOfValues = size;
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues)
  {
    VTKM_ASSERT(numberOfValues <= this->GetNumberOfValues());

    this->NumberOfValues = numberOfValues;
    if (numberOfValues == 0 && this->OwnsResources())
      this->ReleaseResources();
  }

  VTKM_CONT
  void ReleaseResources()
  {
    if (!this->OwnsResources())
      throw vtkm::cont::ErrorBadAllocation(
        "ArrayHandleCuda does not own its internal device memory.");

    if (this->NumberOfValues)
    {
      thrust::system::cuda::free(this->Data);
      this->NumberOfValues = 0;
    }
  }

  VTKM_CONT
  DevicePointer GetDevicePointer() const { return this->Data; }

  VTKM_CONT
  bool OwnsResources() const { return this->IsOwner; }

private:
  DevicePointer Data;
  vtkm::Id NumberOfValues;
  bool IsOwner;
};

} // namespace internal
} // namespace cont
} // namespace vtkm

#ifdef VTKM_CUDA
namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{

template <typename T, typename U>
class ArrayManagerExecutionThrustDevice;

template <typename T>
class ArrayManagerExecutionThrustDevice<T, vtkm::cont::cuda::StorageTagCuda>
{
public:
  typedef T ValueType;
  typedef vtkm::cont::cuda::StorageTagCuda StorageTag;
  typedef typename thrust::system::cuda::pointer<T>::difference_type difference_type;

  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

  typedef vtkm::exec::cuda::internal::ArrayPortalFromThrust<T> PortalType;
  typedef vtkm::exec::cuda::internal::ConstArrayPortalFromThrust<const T> PortalConstType;

  VTKM_CONT
  ArrayManagerExecutionThrustDevice(StorageType* storage)
    : Storage(storage)
  {
  }

  VTKM_CONT
  ~ArrayManagerExecutionThrustDevice() {}

  /// Returns the size of the array.
  ///
  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  /// Since memory is already on the device, there is nothing to prepare
  ///
  VTKM_CONT
  PortalConstType PrepareForInput(bool)
  {
    return PortalConstType(this->Storage->GetDevicePointer(), this->Storage->GetDevicePointer() +
                             static_cast<difference_type>(Storage->GetNumberOfValues()));
  }

  /// Since memory is already on the device, there is nothing to prepare
  ///
  VTKM_CONT
  PortalType PrepareForInPlace(bool)
  {
    return PortalType(this->Storage->GetDevicePointer(), this->Storage->GetDevicePointer() +
                        static_cast<difference_type>(Storage->GetNumberOfValues()));
  }

  /// Allocates the array to the given size.
  ///
  VTKM_CONT
  PortalType PrepareForOutput(vtkm::Id numberOfValues)
  {
    if (this->Storage->GetNumberOfValues())
      this->Storage->ReleaseResources();
    this->Storage->Allocate(numberOfValues);

    return PortalType(this->Storage->GetDevicePointer(), this->Storage->GetDevicePointer() +
                        static_cast<difference_type>(Storage->GetNumberOfValues()));
  }

  /// Since output data stays on the device, there is nothing to retrieve
  ///
  VTKM_CONT
  void RetrieveOutputData(StorageType*) const {}

  /// Resizes the device vector.
  ///
  VTKM_CONT void Shrink(vtkm::Id numberOfValues)
  {
    this->Storage->Shrink(static_cast<vtkm::UInt64>(numberOfValues));
  }

  /// Releases storage resources, if the storage owns them
  VTKM_CONT void ReleaseResources()
  {
    if (this->Storage->OwnsResources())
      this->Storage->ReleaseResources();
  }

private:
  ArrayManagerExecutionThrustDevice(ArrayManagerExecutionThrustDevice<T, StorageTag>&) = delete;
  void operator=(ArrayManagerExecutionThrustDevice<T, StorageTag>&) = delete;

  StorageType* Storage;
};

} //namespace internal
} //namespace cuda
} //namespace cont
} //namespace vtkm
#endif

namespace vtkm
{
namespace cont
{

/// ArrayHandleCuda is a specialization of ArrayHandle, which stores an
/// Array that has already been allocated inside CUDA
template <typename T>
class ArrayHandleCuda : public vtkm::cont::ArrayHandle<T, vtkm::cont::cuda::StorageTagCuda>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleCuda, (ArrayHandleCuda<T>),
                             (vtkm::cont::ArrayHandle<T, vtkm::cont::cuda::StorageTagCuda>));

  VTKM_CONT
  ArrayHandleCuda(T* start, vtkm::Id length)
    : Superclass(vtkm::cont::internal::Storage<T, vtkm::cont::cuda::StorageTagCuda>(start, length))
  {
  }
};

/// A convenience function for creating an ArrayHandle from a Cuda pointer.
///
template <typename T>
VTKM_CONT vtkm::cont::ArrayHandle<T, vtkm::cont::cuda::StorageTagCuda> make_ArrayHandleCuda(
  T* array, vtkm::Id length)
{
  typedef vtkm::cont::cuda::StorageTagCuda StorageTag;
  typedef vtkm::cont::ArrayHandle<T, StorageTag> ArrayHandleType;
  return ArrayHandleType(array, length);
}

template <typename T>
VTKM_CONT void printSummary_ArrayHandle(
  const vtkm::cont::ArrayHandle<T, vtkm::cont::cuda::StorageTagCuda>& array, std::ostream& out)
{
  vtkm::Id sz = array.GetNumberOfValues();
  out << "sz= " << sz << " [(on device)]";
}

} //namespace cont
} //namespace vtkm

namespace vtkm
{
namespace cont
{
namespace cuda
{

template <typename T>
class ArrayHandle : public vtkm::cont::ArrayHandleCuda<T>
{
public:
  VTKM_CONT
  ArrayHandle(T* start, vtkm::Id length)
    : vtkm::cont::ArrayHandleCuda<T>(start, length)
  {
  }
};

} //namespace cuda
} //namespace cont
} //namespace vtkm

#endif //vtk_m_cont_cuda_ArrayHandleCuda_h
