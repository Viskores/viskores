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

#include <thrust/system/cuda/memory.h>
#include <thrust/copy.h>

#if defined(__GNUC__) && !defined(VTKM_CUDA)
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif // gcc version >= 4.6
#endif // gcc && !CUDA

#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>
#include <vtkm/exec/cuda/internal/ArrayPortalFromTexture.h>

#include <boost/utility/enable_if.hpp>

namespace vtkm {
namespace cont {
namespace cuda {
namespace internal {

template<typename T> struct UseTexturePortal      {typedef  boost::false_type type;};

template<> struct UseTexturePortal<vtkm::Int8>    {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::UInt8>   {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::Int16>   {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::UInt16>  {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::Int32>   {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::UInt32>  {typedef boost::true_type type; };

template<> struct UseTexturePortal<vtkm::Vec<vtkm::Int32,2> > {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::Vec<vtkm::UInt32,2> > {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::Vec<vtkm::Int32,4> > {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::Vec<vtkm::UInt32,4> > {typedef boost::true_type type; };

template<> struct UseTexturePortal<vtkm::Float32> {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::Float64> {typedef boost::true_type type; };

template<> struct UseTexturePortal<vtkm::Vec<vtkm::Float32,2> > {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::Vec<vtkm::Float32,4> > {typedef boost::true_type type; };
template<> struct UseTexturePortal<vtkm::Vec<vtkm::Float64,2> > {typedef boost::true_type type; };


/// \c ArrayManagerExecutionThrustDevice provides an implementation for a \c
/// ArrayManagerExecution class for a thrust device adapter that is designed
/// for the cuda backend which has separate memory spaces for host and device. This
/// implementation contains a ::thrust::system::cuda::vector to allocate and manage
/// the array.
///
/// This array manager should only be used with the cuda device adapter,
/// since in the future it will take advantage of texture memory and
/// the unique memory access patterns of cuda systems.
template<typename T, class StorageTag, typename Enable= void>
class ArrayManagerExecutionThrustDevice
{
  //we need a way to detect that we are using FERMI or lower and disable
  //the usage of texture iterator. The __CUDA_ARCH__ define is only around
  //for device code so that can't be used. I expect that we will have to devise
  //some form of Try/Compile with CUDA or just offer this as an advanced CMake
  //option. We could also try and see if a runtime switch is possible.

public:
  typedef T ValueType;

  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> ContainerType;

  typedef vtkm::exec::cuda::internal::ArrayPortalFromThrust< T > PortalType;
  typedef vtkm::exec::cuda::internal::ConstArrayPortalFromThrust< T > PortalConstType;

  VTKM_CONT_EXPORT ArrayManagerExecutionThrustDevice():
    NumberOfValues(0),
    ArrayBegin(),
    ArrayEnd()
  {

  }

  ~ArrayManagerExecutionThrustDevice()
  {
    this->ReleaseResources();
  }

  /// Returns the size of the array.
  ///
  VTKM_CONT_EXPORT vtkm::Id GetNumberOfValues() const {
    return this->NumberOfValues;
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
      this->NumberOfValues = arrayPortal.GetNumberOfValues();
      this->ArrayBegin = ::thrust::system::cuda::malloc<T>( static_cast<std::size_t>(this->NumberOfValues)  );
      this->ArrayEnd = this->ArrayBegin + this->NumberOfValues;

      ::thrust::copy(arrayPortal.GetRawIterator(),
                     arrayPortal.GetRawIterator() + this->NumberOfValues,
                     this->ArrayBegin);
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
    if(this->NumberOfValues > 0)
      {
      ::thrust::system::cuda::free( this->ArrayBegin  );
      }
    this->NumberOfValues = numberOfValues;
    this->ArrayBegin = ::thrust::system::cuda::malloc<T>( this->NumberOfValues  );
    this->ArrayEnd = this->ArrayBegin + numberOfValues;
  }

  /// Allocates enough space in \c controlArray and copies the data in the
  /// device vector into it.
  ///
  VTKM_CONT_EXPORT void RetrieveOutputData(ContainerType &controlArray) const
  {
    controlArray.Allocate(this->NumberOfValues);
    ::thrust::copy(this->ArrayBegin,
                   this->ArrayEnd,
                   controlArray.GetPortal().GetRawIterator());
  }

  /// Resizes the device vector.
  ///
  VTKM_CONT_EXPORT void Shrink(vtkm::Id numberOfValues)
  {
    // The operation will succeed even if this assertion fails, but this
    // is still supposed to be a precondition to Shrink.
    VTKM_ASSERT_CONT(numberOfValues <= this->NumberOfValues);
    this->NumberOfValues = numberOfValues;
    this->ArrayEnd = this->ArrayBegin + this->NumberOfValues;
  }

  VTKM_CONT_EXPORT PortalType GetPortal()
  {
    return PortalType(this->ArrayBegin, this->ArrayEnd);
  }

  VTKM_CONT_EXPORT PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->ArrayBegin, this->ArrayEnd);
  }


  /// Frees all memory.
  ///
  VTKM_CONT_EXPORT void ReleaseResources()
  {
    ::thrust::system::cuda::free( this->ArrayBegin  );
    this->ArrayBegin = ::thrust::system::cuda::pointer<ValueType>();
    this->ArrayEnd = ::thrust::system::cuda::pointer<ValueType>();
  }

private:
  // Not implemented
  ArrayManagerExecutionThrustDevice(
      ArrayManagerExecutionThrustDevice<T, StorageTag> &);
  void operator=(
      ArrayManagerExecutionThrustDevice<T, StorageTag> &);

  vtkm::Id NumberOfValues;
  ::thrust::system::cuda::pointer<ValueType> ArrayBegin;
  ::thrust::system::cuda::pointer<ValueType> ArrayEnd;
};


/// This is a specialization that is used to enable texture memory iterators
template<typename T, class StorageTag>
class ArrayManagerExecutionThrustDevice<T, StorageTag,
    typename ::boost::enable_if< typename UseTexturePortal<T>::type >::type >
{
public:
  typedef T ValueType;

  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> ContainerType;

  typedef vtkm::exec::cuda::internal::ArrayPortalFromThrust< T > PortalType;
  typedef vtkm::exec::cuda::internal::ConstArrayPortalFromTexture< T > PortalConstType;


  VTKM_CONT_EXPORT ArrayManagerExecutionThrustDevice():
    NumberOfValues(0),
    ArrayBegin(),
    ArrayEnd()
  {

  }

  ~ArrayManagerExecutionThrustDevice()
  {
    this->ReleaseResources();
  }

  /// Returns the size of the array.
  ///
  VTKM_CONT_EXPORT vtkm::Id GetNumberOfValues() const {
    return this->NumberOfValues;
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
      this->NumberOfValues = arrayPortal.GetNumberOfValues();
      this->ArrayBegin = ::thrust::system::cuda::malloc<T>( static_cast<std::size_t>(this->NumberOfValues)  );
      this->ArrayEnd = this->ArrayBegin + this->NumberOfValues;

      ::thrust::copy(arrayPortal.GetRawIterator(),
                     arrayPortal.GetRawIterator() + this->NumberOfValues,
                     this->ArrayBegin);
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
    if(this->NumberOfValues > 0)
      {
      ::thrust::system::cuda::free( this->ArrayBegin  );
      }
    this->NumberOfValues = numberOfValues;
    this->ArrayBegin = ::thrust::system::cuda::malloc<T>( this->NumberOfValues  );
    this->ArrayEnd = this->ArrayBegin + numberOfValues;
  }

  /// Allocates enough space in \c controlArray and copies the data in the
  /// device vector into it.
  ///
  VTKM_CONT_EXPORT void RetrieveOutputData(ContainerType &controlArray) const
  {
    controlArray.Allocate(this->NumberOfValues);
    ::thrust::copy(this->ArrayBegin,
                   this->ArrayEnd,
                   controlArray.GetPortal().GetRawIterator());
  }

  /// Resizes the device vector.
  ///
  VTKM_CONT_EXPORT void Shrink(vtkm::Id numberOfValues)
  {
    // The operation will succeed even if this assertion fails, but this
    // is still supposed to be a precondition to Shrink.
    VTKM_ASSERT_CONT(numberOfValues <= this->NumberOfValues);
    this->NumberOfValues = numberOfValues;
    this->ArrayEnd = this->ArrayBegin + this->NumberOfValues;
  }

  VTKM_CONT_EXPORT PortalType GetPortal()
  {
    return PortalType(this->ArrayBegin, this->ArrayEnd);
  }

  VTKM_CONT_EXPORT PortalConstType GetPortalConst() const
  {
    return PortalConstType(this->ArrayBegin, this->ArrayEnd);
  }


  /// Frees all memory.
  ///
  VTKM_CONT_EXPORT void ReleaseResources()
  {
    ::thrust::system::cuda::free( this->ArrayBegin  );
    this->ArrayBegin = ::thrust::system::cuda::pointer<ValueType>();
    this->ArrayEnd = ::thrust::system::cuda::pointer<ValueType>();
  }

private:
  // Not implemented
  ArrayManagerExecutionThrustDevice(
      ArrayManagerExecutionThrustDevice<T, StorageTag> &);
  void operator=(
      ArrayManagerExecutionThrustDevice<T, StorageTag> &);

  vtkm::Id NumberOfValues;
  ::thrust::system::cuda::pointer<ValueType> ArrayBegin;
  ::thrust::system::cuda::pointer<ValueType> ArrayEnd;
};


}
}
}
} // namespace vtkm::cont::cuda::internal

#endif // vtk_m_cont_cuda_internal_ArrayManagerExecutionThrustDevice_h
