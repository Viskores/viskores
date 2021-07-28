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

#include <vtkm/Deprecated.h>
#include <vtkm/LowerBound.h>
#include <vtkm/UpperBound.h>

namespace vtkm
{

VTKM_DEPRECATED(1.7, "Use LowerBound.h, or UpperBound.h instead of Algorithms.h.")
inline void Algorithms_h_deprecated() {}

inline void ActivateAlgorithms_h_deprecated_warning()
{
  Algorithms_h_deprecated();
}

template <typename IterT, typename T, typename Comp>
VTKM_DEPRECATED(1.7, "Use LowerBound or UpperBound instead of BinarySearch.")
VTKM_EXEC_CONT IterT BinarySearch(IterT first, IterT last, const T& val, Comp comp)
{
  IterT found = vtkm::LowerBound(first, last, val, comp);
  if ((found == last) || comp(val, *found) || comp(*found, val))
  {
    // Element is not actually in the array
    return last;
  }
  else
  {
    return found;
  }
}

VTKM_DEPRECATED_SUPPRESS_BEGIN

template <typename IterT, typename T>
VTKM_DEPRECATED(1.7, "Use LowerBound or UpperBound instead of BinarySearch.")
VTKM_EXEC_CONT IterT BinarySearch(IterT first, IterT last, const T& val)
{
  return vtkm::BinarySearch(first, last, val, vtkm::SortLess{});
}

template <typename PortalT, typename T, typename Comp>
VTKM_DEPRECATED(1.7, "Use LowerBound or UpperBound instead of BinarySearch.")
VTKM_EXEC_CONT vtkm::Id BinarySearch(const PortalT& portal, const T& val, Comp comp)
{
  auto first = vtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = vtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = vtkm::BinarySearch(first, last, val, comp);
  return result == last ? static_cast<vtkm::Id>(-1) : static_cast<vtkm::Id>(result - first);
}

// Return -1 if not found
template <typename PortalT, typename T>
VTKM_DEPRECATED(1.7, "Use LowerBound or UpperBound instead of BinarySearch.")
VTKM_EXEC_CONT vtkm::Id BinarySearch(const PortalT& portal, const T& val)
{
  auto first = vtkm::cont::ArrayPortalToIteratorBegin(portal);
  auto last = vtkm::cont::ArrayPortalToIteratorEnd(portal);
  auto result = vtkm::BinarySearch(first, last, val, vtkm::SortLess{});
  return result == last ? static_cast<vtkm::Id>(-1) : static_cast<vtkm::Id>(result - first);
}

VTKM_DEPRECATED_SUPPRESS_END

} // end namespace vtkm

#endif // vtk_m_Algorithms_h
