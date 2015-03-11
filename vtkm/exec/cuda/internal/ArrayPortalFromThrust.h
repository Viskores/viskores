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
#ifndef vtk_m_exec_cuda_internal_ArrayPortalFromThrust_h
#define vtk_m_exec_cuda_internal_ArrayPortalFromThrust_h

#include <vtkm/Types.h>

#include <iterator>

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

#if defined(__GNUC__) && !defined(VTKM_CUDA)
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif // gcc version >= 4.6
#endif // gcc && !CUDA

#include <boost/utility/enable_if.hpp>

namespace vtkm {
namespace exec {
namespace cuda {
namespace internal {

template<typename T> struct UseTextureLoad      {typedef  boost::false_type type;};

template<> struct UseTextureLoad<vtkm::Int8*>    {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::UInt8*>   {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::Int16*>   {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::UInt16*>  {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::Int32*>   {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::UInt32*>  {typedef boost::true_type type; };

template<> struct UseTextureLoad<vtkm::Vec<vtkm::Int32,2>* > {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::Vec<vtkm::UInt32,2>* > {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::Vec<vtkm::Int32,4>* > {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::Vec<vtkm::UInt32,4>* > {typedef boost::true_type type; };

template<> struct UseTextureLoad<vtkm::Float32* > {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::Float64* > {typedef boost::true_type type; };

template<> struct UseTextureLoad<vtkm::Vec<vtkm::Float32,2>* > {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::Vec<vtkm::Float32,4>* > {typedef boost::true_type type; };
template<> struct UseTextureLoad<vtkm::Vec<vtkm::Float64,2>* > {typedef boost::true_type type; };

//this T type is not one that is valid to be loaded through texture memory
template<typename T, typename Enable = void>
struct load_through_texture
{
  VTKM_EXEC_EXPORT
  static  T get(const thrust::system::cuda::pointer<T> data)
  {
  return *(data.get());
  }
};

//this T type is valid to be loaded through texture memory
template<typename T>
struct load_through_texture<T, typename ::boost::enable_if< typename UseTextureLoad<T>::type >::type >
{
  VTKM_EXEC_EXPORT
  static T get(const thrust::system::cuda::pointer<T> data)
  {
  //only load through a texture if we have sm 35 support
#if __CUDA_ARCH__ >= 350
  return __ldg(data.get());
#else
  return *(data.get());
#endif
  }
};


class ArrayPortalFromThrustBase {};

/// This templated implementation of an ArrayPortal allows you to adapt a pair
/// of begin/end iterators to an ArrayPortal interface.
///
template<typename T>
class ArrayPortalFromThrust : public ArrayPortalFromThrustBase
{
public:
  typedef T ValueType;
  typedef typename thrust::system::cuda::pointer< T > PointerType;
  typedef T* IteratorType;

  VTKM_EXEC_CONT_EXPORT ArrayPortalFromThrust() {  }

  VTKM_CONT_EXPORT
  ArrayPortalFromThrust(PointerType begin, PointerType end)
    : BeginIterator( begin ),
      EndIterator( end  )
      {  }

  /// Copy constructor for any other ArrayPortalFromThrust with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<typename OtherT>
  VTKM_EXEC_CONT_EXPORT
  ArrayPortalFromThrust(const ArrayPortalFromThrust<OtherT> &src)
    : BeginIterator(src.BeginIterator),
      EndIterator(src.EndIterator)
  {  }

  template<typename OtherT>
  VTKM_EXEC_CONT_EXPORT
  ArrayPortalFromThrust<T> &operator=(
      const ArrayPortalFromThrust<OtherT> &src)
  {
    this->BeginIterator = src.BeginIterator;
    this->EndIterator = src.EndIterator;
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    // Not using std::distance because on CUDA it cannot be used on a device.
    return (this->EndIterator - this->BeginIterator);
  }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const {
    return *this->IteratorAt(index);
  }

  VTKM_EXEC_EXPORT
  void Set(vtkm::Id index, ValueType value) const {
    *this->IteratorAt(index) = value;
  }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorBegin() const { return this->BeginIterator.get(); }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorEnd() const { return this->EndIterator.get(); }

private:
  PointerType BeginIterator;
  PointerType EndIterator;

  VTKM_EXEC_EXPORT
  PointerType IteratorAt(vtkm::Id index) const {
    // Not using std::advance because on CUDA it cannot be used on a device.
    return (this->BeginIterator + index);
  }
};

template<typename T>
class ConstArrayPortalFromThrust : public ArrayPortalFromThrustBase
{
public:

  typedef T ValueType;
  typedef typename thrust::system::cuda::pointer< T > PointerType;
  typedef const T* IteratorType;

  VTKM_EXEC_CONT_EXPORT ConstArrayPortalFromThrust() {  }

  VTKM_CONT_EXPORT
  ConstArrayPortalFromThrust(const PointerType begin, const PointerType end)
    : BeginIterator( begin ),
      EndIterator( end )
      {  }

  /// Copy constructor for any other ConstArrayPortalFromThrust with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  template<typename OtherT>
  VTKM_EXEC_CONT_EXPORT
  ConstArrayPortalFromThrust(const ConstArrayPortalFromThrust<OtherT> &src)
    : BeginIterator(src.BeginIterator),
      EndIterator(src.EndIterator)
  {  }

  template<typename OtherT>
  VTKM_EXEC_CONT_EXPORT
  ConstArrayPortalFromThrust<T> &operator=(
      const ConstArrayPortalFromThrust<OtherT> &src)
  {
    this->BeginIterator = src.BeginIterator;
    this->EndIterator = src.EndIterator;
    return *this;
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    // Not using std::distance because on CUDA it cannot be used on a device.
    return (this->EndIterator - this->BeginIterator);
  }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const {
    return vtkm::exec::cuda::internal::load_through_texture<ValueType>::get( this->IteratorAt(index) );
  }

  VTKM_EXEC_EXPORT
  void Set(vtkm::Id index, ValueType value) const {
    *this->IteratorAt(index) = value;
  }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorBegin() const { return this->BeginIterator.get(); }

  VTKM_CONT_EXPORT
  IteratorType GetIteratorEnd() const { return this->EndIterator.get(); }

private:
  PointerType BeginIterator;
  PointerType EndIterator;

  VTKM_EXEC_EXPORT
  PointerType IteratorAt(vtkm::Id index) const {
    // Not using std::advance because on CUDA it cannot be used on a device.
    return (this->BeginIterator + index);
  }
};

}
}
}
} // namespace vtkm::exec::cuda::internal


#endif //vtk_m_exec_cuda_internal_ArrayPortalFromThrust_h
