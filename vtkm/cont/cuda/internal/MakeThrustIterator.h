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
#ifndef vtk_m_cont_cuda_internal_MakeThrustIterator_h
#define vtk_m_cont_cuda_internal_MakeThrustIterator_h

#include <vtkm/Types.h>
#include <vtkm/Pair.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>

// Disable warnings we check vtkm for but Thrust does not.
#if defined(__GNUC__) || defined(____clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang

#include <thrust/system/cuda/memory.h>
#include <thrust/functional.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>
#include <thrust/iterator/zip_iterator.h>

#if defined(__GNUC__) || defined(____clang__)
#pragma GCC diagnostic pop
#endif // gcc || clang


//needed forward declares to get zip handle to work properly
namespace vtkm {
namespace exec {
namespace internal {

template<typename ValueType_,
         typename PortalTypeFirst,
         typename PortalTypeSecond>
class ArrayPortalExecZip;

}
}
}

//more forward declares needed to get zip handle to work properly
namespace vtkm {
namespace cont {
namespace cuda {
namespace internal {

namespace detail { template<class PortalType> struct IteratorTraits; }

//forward declare IteratorBegin
template<class PortalType>
VTKM_CONT_EXPORT
typename detail::IteratorTraits<PortalType>::IteratorType
IteratorBegin(PortalType portal);
}
}
}
}


namespace vtkm {
namespace cont {
namespace cuda {
namespace internal {
namespace detail {

// Tags to specify what type of thrust iterator to use.
struct ThrustIteratorTransformTag {  };
struct ThrustIteratorZipTag {  };
struct ThrustIteratorDevicePtrTag {  };

// Traits to help classify what thrust iterators will be used.
template<class PortalType, class IteratorType>
struct ThrustIteratorTag {
  typedef ThrustIteratorTransformTag Type;
};
template<typename PortalType, typename T>
struct ThrustIteratorTag<PortalType, T *> {
  typedef ThrustIteratorDevicePtrTag Type;
};
template<typename PortalType, typename T>
struct ThrustIteratorTag<PortalType, const T*> {
  typedef ThrustIteratorDevicePtrTag Type;
};
template<typename T, typename U, typename V>
struct ThrustIteratorTag< vtkm::exec::internal::ArrayPortalExecZip< T, U, V >,
                          T > {
  //this is a real special case. ExecZip and PortalValue don't combine
  //well together, when used with DeviceAlgorithm that has a custom operator
  //the custom operator is actually passed the PortalValue instead of
  //the real values, and by that point we can't fix anything since we
  //don't know what the original operator is
  typedef ThrustIteratorZipTag Type;
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
  PortalValue()
    : Portal(),
      Index(0) {  }

  VTKM_EXEC_EXPORT
  PortalValue(const PortalType &portal, vtkm::Id index)
    : Portal(portal), Index(index) {  }

  VTKM_EXEC_EXPORT
  PortalValue(const PortalValue<PortalType> &other)
    : Portal(other.Portal), Index(other.Index) {  }

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
    VTKM_EXEC_EXPORT LookupFunctor()
      : Portal() {  }

    VTKM_EXEC_EXPORT LookupFunctor(PortalType portal)
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
struct IteratorChooser<PortalType, detail::ThrustIteratorZipTag> {

  //this is a real special case. ExecZip and PortalValue don't combine
  //well together, when used with DeviceAlgorithm that has a custom operator
  //the custom operator is actually passed the PortalValue instead of
  //the real values, and by that point we can't fix anything since we
  //don't know what the original operator is.

  //So to fix this issue we wrap the original array portals into a thrust
  //zip iterator and let handle everything
  typedef typename PortalType::PortalTypeFirst PortalTypeFirst;
  typedef typename detail::ThrustIteratorTag<
                          PortalTypeFirst,
                          typename PortalTypeFirst::IteratorType>::Type FirstTag;
  typedef typename IteratorChooser<PortalTypeFirst, FirstTag>::Type FirstIterType;

  typedef typename PortalType::PortalTypeSecond PortalTypeSecond;
  typedef typename detail::ThrustIteratorTag<
                          PortalTypeSecond,
                          typename PortalTypeSecond::IteratorType>::Type SecondTag;
  typedef typename IteratorChooser<PortalTypeSecond, SecondTag>::Type SecondIterType;

  //Now that we have deduced the concrete types of the first and second
  //array portals of the zip we can construct a zip iterator for those
  typedef ::thrust::tuple<FirstIterType, SecondIterType> IteratorTuple;
  typedef ::thrust::zip_iterator<IteratorTuple> Type;
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
  typedef typename detail::ThrustIteratorTag<
                          PortalType,
                          typename PortalType::IteratorType>::Type Tag;
  typedef typename IteratorChooser<PortalType, Tag>::Type IteratorType;
};


template<typename T>
VTKM_CONT_EXPORT
::thrust::cuda::pointer<T>
MakeDevicePtr(T *iter)
{
  return::thrust::cuda::pointer<T>(iter);
}
template<typename T>
VTKM_CONT_EXPORT
::thrust::cuda::pointer<const T>
MakeDevicePtr(const T *iter)
{
  return ::thrust::cuda::pointer<const T>(iter);
}

template<typename T, typename U>
VTKM_CONT_EXPORT
::thrust::zip_iterator<
  ::thrust::tuple<typename IteratorTraits<T>::IteratorType,
                  typename IteratorTraits<U>::IteratorType
                  >
  >
MakeZipIterator(const T& t, const U& u)
{
  //todo deduce from T and U the iterator types
  typedef typename IteratorTraits<T>::IteratorType FirstIterType;
  typedef typename IteratorTraits<U>::IteratorType SecondIterType;
  return ::thrust::make_zip_iterator(
          ::thrust::make_tuple( vtkm::cont::cuda::internal::IteratorBegin(t),
                                vtkm::cont::cuda::internal::IteratorBegin(u) )
          );
}

template<class PortalType>
VTKM_CONT_EXPORT
typename IteratorTraits<PortalType>::IteratorType
MakeIteratorBegin(PortalType portal, detail::ThrustIteratorTransformTag)
{
  return ::thrust::make_transform_iterator(
        ::thrust::make_counting_iterator(vtkm::Id(0)),
        LookupFunctor<PortalType>(portal));
}

template<class PortalType>
VTKM_CONT_EXPORT
typename IteratorTraits<PortalType>::IteratorType
MakeIteratorBegin(PortalType portal, detail::ThrustIteratorZipTag)
{
    return MakeZipIterator(portal.GetFirstPortal(),
                           portal.GetSecondPortal()
                           );
}

template<class PortalType>
VTKM_CONT_EXPORT
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

#endif
