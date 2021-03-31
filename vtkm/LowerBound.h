//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_LowerBound_h
#define vtk_m_LowerBound_h

#include <vtkm/BinaryPredicates.h>
#include <vtkm/cont/ArrayPortalToIterators.h>

#include <vtkm/internal/Configure.h>

#include <algorithm>
#include <iterator>

namespace vtkm
{

/// Implementation of std::lower_bound that is appropriate
/// for both control and execution environments.
/// The overloads that take portals return indices instead of iterators.
/// @{
template <typename IterT, typename T, typename Comp>
VTKM_EXEC_CONT IterT LowerBound(IterT first, IterT last, const T& val, Comp comp)
{
#if defined(VTKM_CUDA) || defined(VTKM_HIP)
  auto len = last - first;
  while (len != 0)
  {
    const auto halfLen = len / 2;
    IterT mid = first + halfLen;
    if (comp(*mid, val))
    {
      first = mid + 1;
      len -= halfLen + 1;
    }
    else
    {
      len = halfLen;
    }
  }
  return first;
#else  // VTKM_CUDA || VTKM_HIP
  return std::lower_bound(first, last, val, std::move(comp));
#endif // VTKM_CUDA || VTKM_HIP
}

template <typename IterT, typename T>
VTKM_EXEC_CONT IterT LowerBound(IterT first, IterT last, const T& val)
{
  return vtkm::LowerBound(first, last, val, vtkm::SortLess{});
}

template <typename PortalT, typename T, typename Comp>
VTKM_EXEC_CONT vtkm::Id LowerBound(const PortalT& portal, const T& val, Comp comp)
{
  auto first = vtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = vtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = vtkm::LowerBound(first, last, val, comp);
  return static_cast<vtkm::Id>(result - first);
}

template <typename PortalT, typename T>
VTKM_EXEC_CONT vtkm::Id LowerBound(const PortalT& portal, const T& val)
{
  auto first = vtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = vtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = vtkm::LowerBound(first, last, val, vtkm::SortLess{});
  return static_cast<vtkm::Id>(result - first);
}
/// @}

} // end namespace vtkm

#endif // vtk_m_LowerBound_h
