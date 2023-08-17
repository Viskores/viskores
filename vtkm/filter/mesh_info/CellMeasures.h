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

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/mesh_info/vtkm_filter_mesh_info_export.h>

#include <vtkm/Deprecated.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

/// \brief Specifies over what types of mesh elements CellMeasures will operate.
///
/// The values of `IntegrationType` may be `|`-ed together to select multiple
enum struct IntegrationType
{
  None = 0x00,
  /// @copydoc CellMeasures::SetMeasureToArcLength
  ArcLength = 0x01,
  /// @copydoc CellMeasures::SetMeasureToArea
  Area = 0x02,
  /// @copydoc CellMeasures::SetMeasureToVolume
  Volume = 0x04,
  /// @copydoc CellMeasures::SetMeasureToAll
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

/// @brief Compute the size measure of each cell in a dataset.
///
/// CellMeasures is a filter that generates a new cell data array (i.e., one value
/// specified per cell) holding the signed measure of the cell
/// or 0 (if measure is not well defined or the cell type is unsupported).
///
/// By default, the new cell-data array is named "measure".
class VTKM_FILTER_MESH_INFO_EXPORT CellMeasures : public vtkm::filter::Filter
{
public:
  VTKM_CONT CellMeasures();

  VTKM_DEPRECATED(2.2, "Use default constructor and `SetIntegrationType`.")
  VTKM_CONT explicit CellMeasures(IntegrationType);

  /// @brief Specify the type of integrations to support.
  ///
  /// This filter can support integrating the size of 1D elements (arclength measurements),
  /// 2D elements (area measurements), and 3D elements (volume measurements). The measures to
  /// perform are specified with a `vtkm::filter::mesh_info::IntegrationType`.
  ///
  /// By default, the size measure for all types of elements is performed.
  VTKM_CONT void SetMeasure(vtkm::filter::mesh_info::IntegrationType measure)
  {
    this->Measure = measure;
  }
  /// @copydoc SetMeasure
  VTKM_CONT vtkm::filter::mesh_info::IntegrationType GetMeasure() const { return this->Measure; }
  /// @brief Compute the length of 1D elements.
  VTKM_CONT void SetMeasureToArcLength()
  {
    this->SetMeasure(vtkm::filter::mesh_info::IntegrationType::ArcLength);
  }
  /// @brief Compute the area of 2D elements.
  VTKM_CONT void SetMeasureToArea()
  {
    this->SetMeasure(vtkm::filter::mesh_info::IntegrationType::Area);
  }
  /// @brief Compute the volume of 3D elements.
  VTKM_CONT void SetMeasureToVolume()
  {
    this->SetMeasure(vtkm::filter::mesh_info::IntegrationType::Volume);
  }
  /// @brief Compute the size of all types of elements.
  VTKM_CONT void SetMeasureToAll()
  {
    this->SetMeasure(vtkm::filter::mesh_info::IntegrationType::AllMeasures);
  }

  /// @brief Specify the name of the field generated.
  ///
  /// If not set, `measure` is used.
  VTKM_CONT void SetCellMeasureName(const std::string& name) { this->SetOutputFieldName(name); }
  /// @copydoc SetCellMeasureName
  VTKM_CONT const std::string& GetCellMeasureName() const { return this->GetOutputFieldName(); }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  IntegrationType Measure = vtkm::filter::mesh_info::IntegrationType::AllMeasures;
};
} // namespace mesh_info
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_mesh_info_CellMeasures_h
