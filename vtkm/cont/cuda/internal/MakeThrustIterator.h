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
#include <vtkm/cont/ArrayPortalToIterators.h>

#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>
#include <vtkm/exec/cuda/internal/WrappedOperators.h>

// Disable warnings we check vtkm for but Thrust does not.
#if defined(VTKM_GCC) || defined(VTKM_CLANG)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc || clang

#include <thrust/system/cuda/memory.h>
#include <thrust/functional.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>

#if defined(VTKM_GCC) || defined(VTKM_CLANG)
#pragma GCC diagnostic pop
#endif // gcc || clang

namespace vtkm {
namespace cont {
namespace cuda {
namespace internal {
namespace detail {

// Tags to specify what type of thrust iterator to use.
struct ThrustIteratorFromArrayPortalTag {  };
struct ThrustIteratorDevicePtrTag {  };

// Traits to help classify what thrust iterators will be used.
template<typename IteratorType>
struct ThrustIteratorTag {
  typedef ThrustIteratorFromArrayPortalTag Type;
};
template<typename T>
struct ThrustIteratorTag< thrust::system::cuda::pointer<T> > {
  typedef ThrustIteratorDevicePtrTag Type;
};
template<typename T>
struct ThrustIteratorTag< thrust::system::cuda::pointer<const T> > {
  typedef ThrustIteratorDevicePtrTag Type;
};

template<typename PortalType, typename Tag> struct IteratorChooser;
template<typename PortalType>
struct IteratorChooser<PortalType, detail::ThrustIteratorFromArrayPortalTag> {
  typedef vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType> Type;
};
template<typename PortalType>
struct IteratorChooser<PortalType, detail::ThrustIteratorDevicePtrTag> {
  typedef vtkm::cont::ArrayPortalToIterators<PortalType> PortalToIteratorType;

  typedef typename PortalToIteratorType::IteratorType Type;

};

template<typename PortalType>
struct IteratorTraits
{
  typedef vtkm::cont::ArrayPortalToIterators<PortalType> PortalToIteratorType;
  typedef typename detail::ThrustIteratorTag<
                  typename PortalToIteratorType::IteratorType>::Type Tag;
  typedef typename IteratorChooser<
                                PortalType,
                                Tag
                                >::Type IteratorType;
};

template<typename PortalType>
VTKM_CONT_EXPORT
typename IteratorTraits<PortalType>::IteratorType
MakeIteratorBegin(PortalType portal, detail::ThrustIteratorFromArrayPortalTag)
{
  return vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType>(portal);
}

template<typename PortalType>
VTKM_CONT_EXPORT
typename IteratorTraits<PortalType>::IteratorType
MakeIteratorBegin(PortalType portal, detail::ThrustIteratorDevicePtrTag)
{
  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
  return iterators.GetBegin();
}

} // namespace detail



template<typename PortalType>
VTKM_CONT_EXPORT
typename detail::IteratorTraits<PortalType>::IteratorType
IteratorBegin(PortalType portal)
{
  typedef typename detail::IteratorTraits<PortalType>::Tag IteratorTag;
  return detail::MakeIteratorBegin(portal, IteratorTag());
}

template<typename PortalType>
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
