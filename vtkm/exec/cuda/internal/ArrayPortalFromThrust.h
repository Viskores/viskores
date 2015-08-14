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
#ifndef vtk_m_exec_cuda_internal_ArrayPortalFromThrust_h
#define vtk_m_exec_cuda_internal_ArrayPortalFromThrust_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortalToIterators.h>

#include <iterator>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/integral_constant.hpp>
#include <boost/utility/enable_if.hpp>

#include <thrust/system/cuda/memory.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace exec {
namespace cuda {
namespace internal {

template<typename T> struct UseScalarTextureLoad            {typedef  boost::false_type type;};
template<typename T> struct UseVecTextureLoads              {typedef  boost::false_type type;};
template<typename T> struct UseMultipleScalarTextureLoads   {typedef  boost::false_type type;};

//currently CUDA doesn't support texture loading of signed char's so that is why
//you don't see vtkm::Int8 in any of the lists.
template<> struct UseScalarTextureLoad< const vtkm::UInt8 >   {typedef boost::true_type type; };
template<> struct UseScalarTextureLoad< const vtkm::Int16 >   {typedef boost::true_type type; };
template<> struct UseScalarTextureLoad< const vtkm::UInt16 >  {typedef boost::true_type type; };
template<> struct UseScalarTextureLoad< const vtkm::Int32 >   {typedef boost::true_type type; };
template<> struct UseScalarTextureLoad< const vtkm::UInt32 >  {typedef boost::true_type type; };
template<> struct UseScalarTextureLoad< const vtkm::Float32 > {typedef boost::true_type type; };
template<> struct UseScalarTextureLoad< const vtkm::Float64 > {typedef boost::true_type type; };

//CUDA needs vec types converted to CUDA types ( float2, uint2), so we have a special
//case for these vec texture loads.
template<> struct UseVecTextureLoads< const vtkm::Vec<vtkm::Int32,2> > {typedef boost::true_type type; };
template<> struct UseVecTextureLoads< const vtkm::Vec<vtkm::UInt32,2> > {typedef boost::true_type type; };
template<> struct UseVecTextureLoads< const vtkm::Vec<vtkm::Float32,2> > {typedef boost::true_type type; };
template<> struct UseVecTextureLoads< const vtkm::Vec<vtkm::Float64,2> > {typedef boost::true_type type; };

template<> struct UseVecTextureLoads< const vtkm::Vec<vtkm::Int32,4> > {typedef boost::true_type type; };
template<> struct UseVecTextureLoads< const vtkm::Vec<vtkm::UInt32,4> > {typedef boost::true_type type; };
template<> struct UseVecTextureLoads< const vtkm::Vec<vtkm::Float32,4> > {typedef boost::true_type type; };


//CUDA doesn't support loading 3 wide values through a texture unit by default,
//so instead we fetch through texture three times and store the result
//currently CUDA doesn't support texture loading of signed char's so that is why
//you don't see vtkm::Int8 in any of the lists.

template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::UInt8,2> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::Int16,2> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::UInt16,2> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::Int64,2> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::UInt64,2> > {typedef boost::true_type type; };

template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::UInt8,3> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::Int16,3> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::UInt16,3> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::Int32,3> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::UInt32,3> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::Float32,3> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::Float64,3> > {typedef boost::true_type type; };

template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::UInt8,4> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::Int16,4> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::UInt16,4> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::Int64,4> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::UInt64,4> > {typedef boost::true_type type; };
template<> struct UseMultipleScalarTextureLoads< const vtkm::Vec<vtkm::Float64,4> > {typedef boost::true_type type; };


//this T type is not one that is valid to be loaded through texture memory
template<typename T, typename Enable = void>
struct load_through_texture
{
  VTKM_EXEC_EXPORT
  static T get(const thrust::system::cuda::pointer<T>& data)
  {
  return *(data.get());
  }
};

//only load through a texture if we have sm 35 support

// this T type is valid to be loaded through a single texture memory fetch
template<typename T>
struct load_through_texture<T, typename ::boost::enable_if< typename UseScalarTextureLoad<T>::type >::type >
{
  VTKM_EXEC_EXPORT
  static T get(const thrust::system::cuda::pointer<T>& data)
  {
  #if __CUDA_ARCH__ >= 350
    // printf("__CUDA_ARCH__ UseScalarTextureLoad");
    return __ldg(data.get());
  #else
    return *(data.get());
  #endif
  }
};

// this T type is valid to be loaded through a single vec texture memory fetch
template<typename T>
struct load_through_texture<T, typename ::boost::enable_if< typename UseVecTextureLoads<T>::type >::type >
{
  VTKM_EXEC_EXPORT
  static T get(const thrust::system::cuda::pointer<T>& data)
  {
  #if __CUDA_ARCH__ >= 350
    // printf("__CUDA_ARCH__ UseVecTextureLoads");
    return getAs(data);
  #else
    return *(data.get());
  #endif
  }

  VTKM_EXEC_EXPORT
  static vtkm::Vec<vtkm::Int32,2> getAs(const thrust::system::cuda::pointer<const vtkm::Vec<vtkm::Int32,2> >& data)
  {
  const int2 temp = __ldg((const int2*)data.get());
  return vtkm::Vec<vtkm::Int32,2>(temp.x, temp.y);
  }

  VTKM_EXEC_EXPORT
  static vtkm::Vec<vtkm::UInt32,2> getAs(const thrust::system::cuda::pointer<const vtkm::Vec<vtkm::UInt32,2> >& data)
  {
  const uint2 temp = __ldg((const uint2*)data.get());
  return vtkm::Vec<vtkm::UInt32,2>(temp.x, temp.y);
  }

  VTKM_EXEC_EXPORT
  static vtkm::Vec<vtkm::Int32,4> getAs(const thrust::system::cuda::pointer<const vtkm::Vec<vtkm::Int32,4> >& data)
  {
  const int4 temp = __ldg((const int4*)data.get());
  return vtkm::Vec<vtkm::Int32,4>(temp.x, temp.y, temp.z, temp.w);
  }

  VTKM_EXEC_EXPORT
  static vtkm::Vec<vtkm::UInt32,4> getAs(const thrust::system::cuda::pointer<const vtkm::Vec<vtkm::UInt32,4> >& data)
  {
  const uint4 temp = __ldg((const uint4*)data.get());
  return vtkm::Vec<vtkm::UInt32,4>(temp.x, temp.y, temp.z, temp.w);
  }

  VTKM_EXEC_EXPORT
  static vtkm::Vec<vtkm::Float32,2> getAs(const thrust::system::cuda::pointer<const vtkm::Vec<vtkm::Float32,2> >& data)
  {
  const float2 temp = __ldg((const float2*)data.get());
  return vtkm::Vec<vtkm::Float32,2>(temp.x, temp.y);
  }

  VTKM_EXEC_EXPORT
  static vtkm::Vec<vtkm::Float32,4> getAs(const thrust::system::cuda::pointer<const vtkm::Vec<vtkm::Float32,4> >& data)
  {
  const float4 temp = __ldg((const float4*)data.get());
  return vtkm::Vec<vtkm::Float32,4>(temp.x, temp.y, temp.z, temp.w);
  }

  VTKM_EXEC_EXPORT
  static vtkm::Vec<vtkm::Float64,2> getAs(const thrust::system::cuda::pointer<const vtkm::Vec<vtkm::Float64,2> >& data)
  {
  const double2 temp = __ldg((const double2*)data.get());
  return vtkm::Vec<vtkm::Float64,2>(temp.x, temp.y);
  }
};


//this T type is valid to be loaded through multiple texture memory fetches
template<typename T>
struct load_through_texture<T, typename ::boost::enable_if< typename UseMultipleScalarTextureLoads<T>::type >::type >
{
  typedef typename boost::remove_const<T>::type NonConstT;

  VTKM_EXEC_EXPORT
  static T get(const thrust::system::cuda::pointer<T>& data)
  {
  #if __CUDA_ARCH__ >= 350
     // printf("__CUDA_ARCH__ UseMultipleScalarTextureLoads");
    return getAs(data);
  #else
    return *(data.get());
  #endif
  }

  VTKM_EXEC_EXPORT
  static T getAs(const thrust::system::cuda::pointer<T>& data)
  {
  //we need to fetch each component individually
  const vtkm::IdComponent NUM_COMPONENTS= T::NUM_COMPONENTS;
  typedef typename T::ComponentType ComponentType;
  const ComponentType* recasted_data = (const ComponentType*)(data.get());
  NonConstT result;
  #pragma unroll
  for(vtkm::IdComponent i=0; i < NUM_COMPONENTS; ++i)
  {
    result[i] = __ldg(recasted_data+i);
  }
  return result;
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
  typedef thrust::system::cuda::pointer< T > IteratorType;

  VTKM_EXEC_CONT_EXPORT ArrayPortalFromThrust() {  }

  VTKM_CONT_EXPORT
  ArrayPortalFromThrust(thrust::system::cuda::pointer< T > begin,
                        thrust::system::cuda::pointer< T > end)
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
    : BeginIterator(src.GetIteratorBegin()),
      EndIterator(src.GetIteratorEnd())
  {  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    // Not using std::distance because on CUDA it cannot be used on a device.
    return static_cast<vtkm::Id>( (this->EndIterator - this->BeginIterator) );
  }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const {
    return *(this->BeginIterator + index);
  }

  VTKM_EXEC_EXPORT
  void Set(vtkm::Id index, ValueType value) const {
    *(this->BeginIterator + index) = value;
  }

  VTKM_EXEC_CONT_EXPORT
  IteratorType GetIteratorBegin() const { return this->BeginIterator; }

  VTKM_EXEC_CONT_EXPORT
  IteratorType GetIteratorEnd() const { return this->EndIterator; }

private:
  IteratorType BeginIterator;
  IteratorType EndIterator;
};

template<typename T>
class ConstArrayPortalFromThrust : public ArrayPortalFromThrustBase
{
public:

  typedef T ValueType;
  typedef thrust::system::cuda::pointer< const T > IteratorType;

  VTKM_EXEC_CONT_EXPORT ConstArrayPortalFromThrust() {  }

  VTKM_CONT_EXPORT
  ConstArrayPortalFromThrust(const thrust::system::cuda::pointer< T > begin,
                             const thrust::system::cuda::pointer< T > end)
    : BeginIterator( begin ),
      EndIterator( end )
  {
    // printf("ConstArrayPortalFromThrust() %s \n", __PRETTY_FUNCTION__ );
  }

  /// Copy constructor for any other ConstArrayPortalFromThrust with an iterator
  /// type that can be copied to this iterator type. This allows us to do any
  /// type casting that the iterators do (like the non-const to const cast).
  ///
  // template<typename OtherT>
  VTKM_EXEC_CONT_EXPORT
  ConstArrayPortalFromThrust(const ArrayPortalFromThrust<T> &src)
    : BeginIterator(src.GetIteratorBegin()),
      EndIterator(src.GetIteratorEnd())
  {
  }

  VTKM_EXEC_CONT_EXPORT
  vtkm::Id GetNumberOfValues() const {
    // Not using std::distance because on CUDA it cannot be used on a device.
    return static_cast<vtkm::Id>( (this->EndIterator - this->BeginIterator) );
  }

  VTKM_EXEC_EXPORT
  ValueType Get(vtkm::Id index) const {
    return vtkm::exec::cuda::internal::load_through_texture<ValueType>::get( this->BeginIterator + index );
  }

  VTKM_EXEC_EXPORT
  void Set(vtkm::Id index, ValueType value) const {
    *(this->BeginIterator + index) = value;
  }

  VTKM_EXEC_CONT_EXPORT
  IteratorType GetIteratorBegin() const { return this->BeginIterator; }

  VTKM_EXEC_CONT_EXPORT
  IteratorType GetIteratorEnd() const { return this->EndIterator; }

private:
  IteratorType BeginIterator;
  IteratorType EndIterator;
};

}
}
}
} // namespace vtkm::exec::cuda::internal


namespace vtkm {
namespace cont {

/// Partial specialization of \c ArrayPortalToIterators for \c
/// ArrayPortalFromThrust. Returns the original array rather than
/// the portal wrapped in an \c IteratorFromArrayPortal.
///
template<typename T>
class ArrayPortalToIterators<
    vtkm::exec::cuda::internal::ArrayPortalFromThrust<T> >
{
  typedef vtkm::exec::cuda::internal::ArrayPortalFromThrust<T>
      PortalType;
public:

  typedef typename PortalType::IteratorType IteratorType;

  VTKM_CONT_EXPORT
  ArrayPortalToIterators(const PortalType &portal)
    : BIterator(portal.GetIteratorBegin()),
      EIterator(portal.GetIteratorEnd())
  {  }

  VTKM_CONT_EXPORT
  IteratorType GetBegin() const { return this->BIterator; }

  VTKM_CONT_EXPORT
  IteratorType GetEnd() const { return this->EIterator; }

private:
  IteratorType BIterator;
  IteratorType EIterator;
  vtkm::Id NumberOfValues;
};

/// Partial specialization of \c ArrayPortalToIterators for \c
/// ConstArrayPortalFromThrust. Returns the original array rather than
/// the portal wrapped in an \c IteratorFromArrayPortal.
///
template<typename T>
class ArrayPortalToIterators<
    vtkm::exec::cuda::internal::ConstArrayPortalFromThrust<T> >
{
  typedef vtkm::exec::cuda::internal::ConstArrayPortalFromThrust<T>
      PortalType;
public:

  typedef typename PortalType::IteratorType IteratorType;

  VTKM_CONT_EXPORT
  ArrayPortalToIterators(const PortalType &portal)
    : BIterator(portal.GetIteratorBegin()),
      EIterator(portal.GetIteratorEnd())
  {  }

  VTKM_CONT_EXPORT
  IteratorType GetBegin() const { return this->BIterator; }

  VTKM_CONT_EXPORT
  IteratorType GetEnd() const { return this->EIterator; }

private:
  IteratorType BIterator;
  IteratorType EIterator;
  vtkm::Id NumberOfValues;
};

}
} // namespace vtkm::cont


#endif //vtk_m_exec_cuda_internal_ArrayPortalFromThrust_h
