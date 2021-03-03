//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_CellLocatorMultiplexer_h
#define vtk_m_exec_CellLocatorMultiplexer_h

#include <vtkm/ErrorCode.h>
#include <vtkm/TypeList.h>

#include <vtkm/exec/internal/Variant.h>

namespace vtkm
{
namespace exec
{

namespace detail
{

struct FindCellFunctor
{
  template <typename Locator>
  VTKM_EXEC vtkm::ErrorCode operator()(Locator&& locator,
                                       const vtkm::Vec3f& point,
                                       vtkm::Id& cellId,
                                       vtkm::Vec3f& parametric) const
  {
    return locator.FindCell(point, cellId, parametric);
  }
};

} // namespace detail

template <typename... LocatorTypes>
class VTKM_ALWAYS_EXPORT CellLocatorMultiplexer
{
  vtkm::exec::internal::Variant<LocatorTypes...> Locators;

public:
  CellLocatorMultiplexer() = default;

  template <typename Locator>
  VTKM_CONT CellLocatorMultiplexer(const Locator& locator)
    : Locators(locator)
  {
  }

  VTKM_EXEC vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                                     vtkm::Id& cellId,
                                     vtkm::Vec3f& parametric) const
  {
    return this->Locators.CastAndCall(detail::FindCellFunctor{}, point, cellId, parametric);
  }

  VTKM_DEPRECATED(1.6, "Locators are no longer pointers. Use . operator.")
  VTKM_EXEC CellLocatorMultiplexer* operator->() { return this; }
  VTKM_DEPRECATED(1.6, "Locators are no longer pointers. Use . operator.")
  VTKM_EXEC const CellLocatorMultiplexer* operator->() const { return this; }
};

}
} // namespace vtkm::exec

#endif //vtk_m_exec_CellLocatorMultiplexer_h
