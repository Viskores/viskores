//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_mesh_info_CellMeasures_h
#define vtk_m_filter_mesh_info_CellMeasures_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/mesh_info/vtkm_filter_mesh_info_export.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

/// \brief Specifies over what types of mesh elements CellMeasures will operate.
enum struct IntegrationType
{
  None = 0x00,
  ArcLength = 0x01,
  Area = 0x02,
  Volume = 0x04,
  AllMeasures = ArcLength | Area | Volume
};

VTKM_EXEC_CONT inline IntegrationType operator&(IntegrationType left, IntegrationType right)
{
  return static_cast<IntegrationType>(static_cast<int>(left) & static_cast<int>(right));
}
VTKM_EXEC_CONT inline IntegrationType operator|(IntegrationType left, IntegrationType right)
{
  return static_cast<IntegrationType>(static_cast<int>(left) | static_cast<int>(right));
}

/// \brief Compute the measure of each (3D) cell in a dataset.
///
/// CellMeasures is a filter that generates a new cell data array (i.e., one value
/// specified per cell) holding the signed measure of the cell
/// or 0 (if measure is not well defined or the cell type is unsupported).
///
/// By default, the new cell-data array is named "measure".
class VTKM_FILTER_MESH_INFO_EXPORT CellMeasures : public vtkm::filter::NewFilterField
{
public:
  VTKM_CONT
  explicit CellMeasures(IntegrationType);

  /// Set/Get the name of the cell measure field. If not set, "measure" is used.
  void SetCellMeasureName(const std::string& name) { this->SetOutputFieldName(name); }
  const std::string& GetCellMeasureName() const { return this->GetOutputFieldName(); }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  IntegrationType measure;
};
} // namespace mesh_info
} // namespace filter
} // namespace vtkm

// Implement the deprecated functionality of vtkm::filter::CellMeasures, which was moved into the
// mesh_info namespace (along with some other API changes). Everything below this line (up to the
// #endif for the include guard) can be deleted once the deprecated functionality is removed.

// Don't warn about deprecation while implementing deprecated functionality.
VTKM_DEPRECATED_SUPPRESS_BEGIN

namespace vtkm
{

struct VTKM_DEPRECATED(1.8, "IntegrateOver is no longer supported") IntegrateOver
{
};
struct VTKM_DEPRECATED(1.8, "IntegrateOverCurve is no longer supported") IntegrateOverCurve
  : IntegrateOver
{
  static constexpr vtkm::filter::mesh_info::IntegrationType value =
    vtkm::filter::mesh_info::IntegrationType::ArcLength;
};
struct VTKM_DEPRECATED(1.8, "IntegrateOverSurface is no longer supported") IntegrateOverSurface
  : IntegrateOver
{
  static constexpr vtkm::filter::mesh_info::IntegrationType value =
    vtkm::filter::mesh_info::IntegrationType::Area;
};
struct VTKM_DEPRECATED(1.8, "IntegrateOverSurface is no longer supported") IntegrateOverSolid
  : IntegrateOver
{
  static constexpr vtkm::filter::mesh_info::IntegrationType value =
    vtkm::filter::mesh_info::IntegrationType::Volume;
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

inline vtkm::filter::mesh_info::IntegrationType OldToNewIntegrationType(vtkm::List<>)
{
  return vtkm::filter::mesh_info::IntegrationType::None;
}

template <typename T, typename... Ts>
inline vtkm::filter::mesh_info::IntegrationType OldToNewIntegrationType(vtkm::List<T, Ts...>)
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
    : vtkm::filter::mesh_info::CellMeasures(
        vtkm::detail::OldToNewIntegrationType(IntegrationTypeList{}))
  {
  }
};

} // namespace filter
} // namespace vtkm

VTKM_DEPRECATED_SUPPRESS_END

#endif // vtk_m_filter_mesh_info_CellMeasures_h
