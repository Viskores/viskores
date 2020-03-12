//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_Algorithms_h
#define vtk_m_Algorithms_h

#include <vtkm/cont/ArrayPortalToIterators.h>

#include <vtkm/BinaryPredicates.h>

#include <vtkm/internal/Configure.h>

#include <algorithm>
#include <iterator>

namespace vtkm
{

/// Similar to std::lower_bound and std::upper_bound, but returns an iterator
/// to any matching item (rather than a specific one). Returns @a last when
/// @a val is not found.
/// @{
template <typename IterT, typename T, typename Comp>
VTKM_EXEC_CONT IterT BinarySearch(IterT first, IterT last, const T& val, Comp comp)
{
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
    else if (comp(val, *mid))
    {
      len = halfLen;
    }
    else
    {
      return mid; // found element
    }
  }
  return last; // did not find element
}

template <typename IterT, typename T>
VTKM_EXEC_CONT IterT BinarySearch(IterT first, IterT last, const T& val)
{
  return vtkm::BinarySearch(first, last, val, vtkm::SortLess{});
}
/// @}

/// Similar to std::lower_bound and std::upper_bound, but returns the index of
/// any matching item (rather than a specific one). Returns -1 when @a val is not
/// found.
/// @{
template <typename PortalT, typename T, typename Comp>
VTKM_EXEC_CONT vtkm::Id BinarySearch(const PortalT& portal, const T& val, Comp comp)
{
  auto first = vtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = vtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = vtkm::BinarySearch(first, last, val, comp);
  return result == last ? static_cast<vtkm::Id>(-1) : static_cast<vtkm::Id>(result - first);
}

// Return -1 if not found
template <typename PortalT, typename T>
VTKM_EXEC_CONT vtkm::Id BinarySearch(const PortalT& portal, const T& val)
{
  auto first = vtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = vtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = vtkm::BinarySearch(first, last, val, vtkm::SortLess{});
  return result == last ? static_cast<vtkm::Id>(-1) : static_cast<vtkm::Id>(result - first);
}
/// @}

/// Implementation of std::lower_bound or std::upper_bound that is appropriate
/// for both control and execution environments.
/// The overloads that take portals return indices instead of iterators.
/// @{
template <typename IterT, typename T, typename Comp>
VTKM_EXEC_CONT IterT LowerBound(IterT first, IterT last, const T& val, Comp comp)
{
#ifdef VTKM_CUDA
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
#else  // VTKM_CUDA
  return std::lower_bound(first, last, val, std::move(comp));
#endif // VTKM_CUDA
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

template <typename IterT, typename T, typename Comp>
VTKM_EXEC_CONT IterT UpperBound(IterT first, IterT last, const T& val, Comp comp)
{
#ifdef VTKM_CUDA
  auto len = last - first;
  while (len != 0)
  {
    const auto halfLen = len / 2;
    IterT mid = first + halfLen;
    if (!comp(val, *mid))
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
#else  // VTKM_CUDA
  return std::upper_bound(first, last, val, std::move(comp));
#endif // VTKM_CUDA
}

template <typename IterT, typename T>
VTKM_EXEC_CONT IterT UpperBound(IterT first, IterT last, const T& val)
{
  return vtkm::UpperBound(first, last, val, vtkm::SortLess{});
}

template <typename PortalT, typename T, typename Comp>
VTKM_EXEC_CONT vtkm::Id UpperBound(const PortalT& portal, const T& val, Comp comp)
{
  auto first = vtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = vtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = vtkm::UpperBound(first, last, val, comp);
  return static_cast<vtkm::Id>(result - first);
}

template <typename PortalT, typename T>
VTKM_EXEC_CONT vtkm::Id UpperBound(const PortalT& portal, const T& val)
{
  auto first = vtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = vtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = vtkm::UpperBound(first, last, val, vtkm::SortLess{});
  return static_cast<vtkm::Id>(result - first);
}
/// @}

} // end namespace vtkm

#endif // vtk_m_Algorithms_h
