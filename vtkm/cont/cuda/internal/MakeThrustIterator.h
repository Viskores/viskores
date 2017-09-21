//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_cuda_internal_MakeThrustIterator_h
#define vtk_m_cont_cuda_internal_MakeThrustIterator_h

#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/internal/ExportMacros.h>

#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>
#include <vtkm/exec/cuda/internal/WrappedOperators.h>

// Disable warnings we check vtkm for but Thrust does not.
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/functional.h>
#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>
#include <thrust/system/cuda/memory.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{
namespace detail
{

// Tags to specify what type of thrust iterator to use.
struct ThrustIteratorFromArrayPortalTag
{
};
struct ThrustIteratorDevicePtrTag
{
};

// Traits to help classify what thrust iterators will be used.
template <typename IteratorType>
struct ThrustIteratorTag
{
  using Type = ThrustIteratorFromArrayPortalTag;
};
template <typename T>
struct ThrustIteratorTag<thrust::system::cuda::pointer<T>>
{
  using Type = ThrustIteratorDevicePtrTag;
};
template <typename T>
struct ThrustIteratorTag<thrust::system::cuda::pointer<const T>>
{
  using Type = ThrustIteratorDevicePtrTag;
};

template <typename PortalType, typename Tag>
struct IteratorChooser;
template <typename PortalType>
struct IteratorChooser<PortalType, detail::ThrustIteratorFromArrayPortalTag>
{
  using Type = vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType>;
};
template <typename PortalType>
struct IteratorChooser<PortalType, detail::ThrustIteratorDevicePtrTag>
{
  using PortalToIteratorType = vtkm::cont::ArrayPortalToIterators<PortalType>;

  using Type = typename PortalToIteratorType::IteratorType;
};

template <typename PortalType>
struct IteratorTraits
{
  using PortalToIteratorType = vtkm::cont::ArrayPortalToIterators<PortalType>;
  using Tag = typename detail::ThrustIteratorTag<typename PortalToIteratorType::IteratorType>::Type;
  using IteratorType = typename IteratorChooser<PortalType, Tag>::Type;
};

template <typename PortalType>
VTKM_CONT typename IteratorTraits<PortalType>::IteratorType MakeIteratorBegin(
  PortalType portal,
  detail::ThrustIteratorFromArrayPortalTag)
{
  return vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType>(portal);
}

template <typename PortalType>
VTKM_CONT typename IteratorTraits<PortalType>::IteratorType MakeIteratorBegin(
  PortalType portal,
  detail::ThrustIteratorDevicePtrTag)
{
  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
  return iterators.GetBegin();
}

template <typename PortalType>
VTKM_CONT typename IteratorTraits<PortalType>::IteratorType MakeIteratorEnd(
  PortalType portal,
  detail::ThrustIteratorFromArrayPortalTag)
{
  vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType> iterator(portal);
  ::thrust::advance(iterator, static_cast<std::size_t>(portal.GetNumberOfValues()));
  return iterator;
}

template <typename PortalType>
VTKM_CONT typename IteratorTraits<PortalType>::IteratorType MakeIteratorEnd(
  PortalType portal,
  detail::ThrustIteratorDevicePtrTag)
{
  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
  return iterators.GetEnd();
}

} // namespace detail

template <typename PortalType>
VTKM_CONT typename detail::IteratorTraits<PortalType>::IteratorType IteratorBegin(PortalType portal)
{
  using IteratorTag = typename detail::IteratorTraits<PortalType>::Tag;
  return detail::MakeIteratorBegin(portal, IteratorTag());
}

template <typename PortalType>
VTKM_CONT typename detail::IteratorTraits<PortalType>::IteratorType IteratorEnd(PortalType portal)
{
  using IteratorTag = typename detail::IteratorTraits<PortalType>::Tag;
  return detail::MakeIteratorEnd(portal, IteratorTag());
}
}
}
}

} //namespace vtkm::cont::cuda::internal

#endif
