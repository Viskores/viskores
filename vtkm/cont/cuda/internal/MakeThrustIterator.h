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
#ifndef vtk_m_cont_cuda_internal_MakeThrustIterator_h
#define vtk_m_cont_cuda_internal_MakeThrustIterator_h

#include <vtkm/Types.h>
#include <vtkm/internal/ExportMacros.h>

// Disable GCC warnings we check Dax for but Thrust does not.
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
#include <thrust/functional.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>

#if defined(__GNUC__) && !defined(VTKM_CUDA)
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif // gcc version >= 4.6
#endif // gcc && !CUDA


namespace vtkm {
namespace cont {
namespace cuda {
namespace internal {

namespace detail {

// Tags to specify what type of thrust iterator to use.
struct ThrustIteratorTransformTag {  };
struct ThrustIteratorDevicePtrTag {  };

// Traits to help classify what thrust iterators will be used.
template<class IteratorType>
struct ThrustIteratorTag {
  typedef ThrustIteratorTransformTag Type;
};
template<typename T>
struct ThrustIteratorTag<T *> {
  typedef ThrustIteratorDevicePtrTag Type;
};
template<typename T>
struct ThrustIteratorTag<const T*> {
  typedef ThrustIteratorDevicePtrTag Type;
};

template<typename T> struct ThrustStripPointer;
template<typename T> struct ThrustStripPointer<T *> {
  typedef T Type;
};
template<typename T> struct ThrustStripPointer<const T *> {
  typedef const T Type;
};


template<class PortalType>
struct PortalValue {
  typedef typename PortalType::ValueType ValueType;

  VTKM_EXEC_EXPORT
  PortalValue(const PortalType &portal, vtkm::Id index)
    : Portal(portal), Index(index) {  }

  VTKM_EXEC_EXPORT
  ValueType operator=(ValueType value) {
    this->Portal.Set(this->Index, value);
    return value;
  }

  VTKM_EXEC_EXPORT
  operator ValueType(void) const {
    return this->Portal.Get(this->Index);
  }

  const PortalType Portal;
  const vtkm::Id Index;
};

template<class PortalType>
class LookupFunctor
      : public ::thrust::unary_function<vtkm::Id,
                                        PortalValue<PortalType>  >
{
  public:
    VTKM_CONT_EXPORT LookupFunctor(PortalType portal)
      : Portal(portal) {  }

    VTKM_EXEC_EXPORT
    PortalValue<PortalType>
    operator()(vtkm::Id index)
    {
      return PortalValue<PortalType>(this->Portal, index);
    }

  private:
    PortalType Portal;
};

template<class PortalType, class Tag> struct IteratorChooser;
template<class PortalType>
struct IteratorChooser<PortalType, detail::ThrustIteratorTransformTag> {
  typedef ::thrust::transform_iterator<
      LookupFunctor<PortalType>,
      ::thrust::counting_iterator<vtkm::Id> > Type;
};
template<class PortalType>
struct IteratorChooser<PortalType, detail::ThrustIteratorDevicePtrTag> {
  typedef ::thrust::cuda::pointer<
      typename detail::ThrustStripPointer<
          typename PortalType::IteratorType>::Type> Type;
};

template<class PortalType>
struct IteratorTraits
{
  typedef typename PortalType::IteratorType BaseIteratorType;
  typedef typename detail::ThrustIteratorTag<BaseIteratorType>::Type Tag;
  typedef typename IteratorChooser<PortalType, Tag>::Type IteratorType;
};

template<typename T>
VTKM_CONT_EXPORT static
::thrust::cuda::pointer<T>
MakeDevicePtr(T *iter)
{
  return::thrust::cuda::pointer<T>(iter);
}
template<typename T>
VTKM_CONT_EXPORT static
::thrust::cuda::pointer<const T>
MakeDevicePtr(const T *iter)
{
  return ::thrust::cuda::pointer<const T>(iter);
}

template<class PortalType>
VTKM_CONT_EXPORT static
typename IteratorTraits<PortalType>::IteratorType
MakeIteratorBegin(PortalType portal, detail::ThrustIteratorTransformTag)
{
  return ::thrust::make_transform_iterator(
        ::thrust::make_counting_iterator(vtkm::Id(0)),
        LookupFunctor<PortalType>(portal));
}

template<class PortalType>
VTKM_CONT_EXPORT static
typename IteratorTraits<PortalType>::IteratorType
MakeIteratorBegin(PortalType portal, detail::ThrustIteratorDevicePtrTag)
{
  return MakeDevicePtr(portal.GetIteratorBegin());
}

} // namespace detail



template<class PortalType>
VTKM_CONT_EXPORT
typename detail::IteratorTraits<PortalType>::IteratorType
IteratorBegin(PortalType portal)
{
  typedef typename detail::IteratorTraits<PortalType>::Tag IteratorTag;
  return detail::MakeIteratorBegin(portal, IteratorTag());
}

template<class PortalType>
VTKM_CONT_EXPORT
typename detail::IteratorTraits<PortalType>::IteratorType
IteratorEnd(PortalType portal)
{
  return IteratorBegin(portal) + portal.GetNumberOfValues();
}

}
}
}
} //namespace vtkm::cont::cuda::internal

namespace thrust {

template< typename PortalType >
struct less< vtkm::cont::cuda::internal::detail::PortalValue< PortalType > > :
        public binary_function<
          vtkm::cont::cuda::internal::detail::PortalValue< PortalType >,
          vtkm::cont::cuda::internal::detail::PortalValue< PortalType >,
          bool>
{
  typedef vtkm::cont::cuda::internal::detail::PortalValue< PortalType > T;
  typedef typename vtkm::cont::cuda::internal::detail::PortalValue<
                        PortalType >::ValueType ValueType;


  /*! Function call operator. The return value is <tt>lhs < rhs</tt>.
   */
  __host__ __device__ bool operator()(const T &lhs, const T &rhs) const
  {return (ValueType)lhs < (ValueType)rhs;}

  /*! Function call operator. The return value is <tt>lhs < rhs</tt>.
      specially designed to work with vtkm portal values, which can
      be compared to their underline type
   */
  __host__ __device__ bool operator()(const T &lhs,
                                      const ValueType &rhs) const
  {return (ValueType)lhs < rhs;}
}; // end less

}
#endif
