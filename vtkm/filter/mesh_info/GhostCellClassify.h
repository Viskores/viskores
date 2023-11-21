//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_mesh_info_GhostCellClassify_h
#define vtk_m_filter_mesh_info_GhostCellClassify_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/mesh_info/vtkm_filter_mesh_info_export.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

/// @brief Determines which cells should be considered ghost cells in a structured data set.
///
/// The ghost cells are expected to be on the border. The outer layer of cells are marked
/// as ghost cells and the remainder marked as normal.
///
/// This filter generates a new cell-centered field marking the status of each cell.
/// Each entry is set to either `vtkm::CellClassification::Normal` or
/// `vtkm::CellClassification::Ghost`.
///
class VTKM_FILTER_MESH_INFO_EXPORT GhostCellClassify : public vtkm::filter::Filter
{
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;
  std::string GhostCellName;

public:
  VTKM_CONT GhostCellClassify()
    : Filter()
    , GhostCellName(vtkm::cont::GetGlobalGhostCellFieldName())
  {
  }

  /// @brief Set the name of the output field name.
  ///
  /// The output field is also marked as the ghost cell field in the output
  /// `vtkm::cont::DataSet`.
  VTKM_CONT void SetGhostCellName(const std::string& fieldName) { this->GhostCellName = fieldName; }
  /// @copydoc SetGhostCellName
  VTKM_CONT const std::string& GetGhostCellName() { return this->GhostCellName; }
};

} // namespace mesh_info
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_mesh_info_GhostCellClassify_h
