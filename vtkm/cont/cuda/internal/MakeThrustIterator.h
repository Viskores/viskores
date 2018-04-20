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

#include <vtkm/exec/cuda/internal/ArrayPortalFromThrust.h>
#include <vtkm/exec/cuda/internal/IteratorFromArrayPortal.h>

namespace vtkm
{
namespace cont
{
namespace cuda
{
namespace internal
{
template <typename PortalType>
inline vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType> IteratorBegin(
  const PortalType& portal)
{
  vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType> iterator(portal);
  return iterator;
}

template <typename PortalType>
inline vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType> IteratorEnd(
  const PortalType& portal)
{
  vtkm::exec::cuda::internal::IteratorFromArrayPortal<PortalType> iterator(portal);
  iterator += static_cast<std::ptrdiff_t>(portal.GetNumberOfValues());
  return iterator;
}

template <typename T>
inline T* IteratorBegin(const vtkm::exec::cuda::internal::ArrayPortalFromThrust<T>& portal)
{
  return portal.GetIteratorBegin();
}

template <typename T>
inline T* IteratorEnd(const vtkm::exec::cuda::internal::ArrayPortalFromThrust<T>& portal)
{
  return portal.GetIteratorEnd();
}

template <typename T>
inline const T* IteratorBegin(
  const vtkm::exec::cuda::internal::ConstArrayPortalFromThrust<T>& portal)
{
  return portal.GetIteratorBegin();
}

template <typename T>
inline const T* IteratorEnd(const vtkm::exec::cuda::internal::ConstArrayPortalFromThrust<T>& portal)
{
  return portal.GetIteratorEnd();
}
}
}
}

} //namespace vtkm::cont::cuda::internal

#endif
