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
#include <vtkm/exec/cuda/internal/WrappedOperators.h>

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

template<typename T> struct ThrustStripPointer;
template<typename T> struct ThrustStripPointer<T *> {
  typedef T Type;
};
template<typename T> struct ThrustStripPointer<const T *> {
  typedef const T Type;
};

template<class PortalType, class Tag> struct IteratorChooser;
template<class PortalType>
struct IteratorChooser<PortalType, detail::ThrustIteratorTransformTag> {
  typedef vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType> Type;
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

template<class PortalType>
VTKM_CONT_EXPORT
typename IteratorTraits<PortalType>::IteratorType
MakeIteratorBegin(PortalType portal, detail::ThrustIteratorTransformTag)
{
  return vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType>(portal,0);
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
