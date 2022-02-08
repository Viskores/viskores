//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_CellMeasures_h
#define vtk_m_filter_CellMeasures_h

#include <vtkm/Deprecated.h>
#include <vtkm/filter/mesh_info/CellMeasures.h>

namespace vtkm
{

struct VTKM_DEPRECATED(1.8, "IntegrateOver is no longer supported") IntegrateOver
{
};
struct VTKM_DEPRECATED(1.8, "IntegrateOverCurve is no longer supported") IntegrateOverCurve
  : IntegrateOver
{
  static constexpr IntegrationType value = ArcLength;
};
struct VTKM_DEPRECATED(1.8, "IntegrateOverSurface is no longer supported") IntegrateOverSurface
  : IntegrateOver
{
  static constexpr IntegrationType value = Area;
};
struct VTKM_DEPRECATED(1.8, "IntegrateOverSurface is no longer supported") IntegrateOverSolid
  : IntegrateOver
{
  static constexpr IntegrationType value = Volume;
};

// Lists of acceptable types of integration
using ArcLength VTKM_DEPRECATED(1.8, "Use vtkm::filter::mesh_info::IntegrationType::ArcLength") =
  vtkm::List<IntegrateOverCurve>;
using Area VTKM_DEPRECATED(1.8, "Use vtkm::filter::mesh_info::IntegrationType::Area") =
  vtkm::List<IntegrateOverSurface>;
using Volume VTKM_DEPRECATED(1.8, "Use vtkm::filter::mesh_info::IntegrationType::Volume") =
  vtkm::List<IntegrateOverSolid>;
using AllMeasures VTKM_DEPRECATED(1.8,
                                  "Use vtkm::filter::mesh_info::IntegrationType::AllMeasures") =
  vtkm::List<IntegrateOverSolid, IntegrateOverSurface, IntegrateOverCurve>;

namespace detail
{

IntegrationType OldToNewIntegrationType(vtkm::List<>)
{
  return static_cast<IntegrationType>(0);
}

template <typename T, typename... Ts>
IntegrationType OldToNewIntegrationType(vtkm::List<T, Ts...>)
{
  return T::value | OldToNewIntegrationType(vtkm::List<Ts...>{});
}

} // namespace detail

namespace filter
{
template <typename IntegrationTypeList>
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::mesh_info::CellMeasures.") CellMeasures
  : public vtkm::filter::mesh_info::CellMeasures
{
public:
  CellMeasures()
    : vtkm::filter::mesh_info::CellMeasures(vtkm::detail::OldToNewIntegrationType(IntegrationTypeList{})
  {
  }
};

VTKM_DEPRECATED(1.8,
                "Use vtkm/filter/mesh_info/CellMeasures.h instead of vtkm/filter/CellMeasures.h.")
inline void CellMeasures_deprecated() {}

inline void CellMeasures_deprecated_warning()
{
  CellMeasures_deprecated();
}

} // namespace filter
} // namespace vtkm
#endif //vtk_m_filter_CellMeasures_h
