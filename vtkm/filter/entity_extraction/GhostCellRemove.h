//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_entity_extraction_GhostCellRemove_h
#define vtk_m_filter_entity_extraction_GhostCellRemove_h

#include <vtkm/CellClassification.h>
#include <vtkm/Deprecated.h>
#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
/// \brief Removes cells marked as ghost cells.
///
/// This filter inspects the ghost cell field of the input and removes any cells
/// marked as ghost cells. Although this filter nominally operates on ghost cells,
/// other classifications, such as blanked cells, can also be recorded in the ghost
/// cell array. See `vtkm::CellClassification` for the list of flags typical in a
/// ghost array.
///
/// By default, if the input is a structured data set the filter will attempt to
/// output a structured data set. This will be the case if all the cells along a
/// boundary are marked as ghost cells together, which is common. If creating a
/// structured data set is not possible, an explicit data set is produced.
///
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT GhostCellRemove : public vtkm::filter::FilterField
{
public:
  VTKM_CONT GhostCellRemove();

  /// @brief Specify whether the ghost cell array should be removed from the input.
  ///
  /// If this flag is true, then the ghost cell array will not be
  /// passed to the output.
  VTKM_CONT void SetRemoveGhostField(bool flag) { this->RemoveField = flag; }
  /// @copydoc SetRemoveGhostField
  VTKM_CONT bool GetRemoveGhostField() const { return this->RemoveField; }

  /// @brief Specify which types of cells to remove.
  ///
  /// The types to remove are specified by the flags in `vtkm::CellClassification`.
  /// Any cell with a ghost array flag matching one or more of these flags will be removed.
  VTKM_CONT void SetTypesToRemove(vtkm::UInt8 typeFlags) { this->TypesToRemove = typeFlags; }
  /// @copydoc SetTypesToRemove
  VTKM_CONT vtkm::UInt8 GetTypesToRemove() const { return this->TypesToRemove; }

  /// @brief Set filter to remove any special cell type.
  ///
  /// This method sets the state to remove any cell that does not have a "normal" ghost
  /// cell value of 0. Any other value represents a cell that is placeholder or otherwise
  /// not really considered part of the cell set.
  VTKM_CONT void SetTypesToRemoveToAll() { this->SetTypesToRemove(0xFF); }
  /// @brief Returns true if all abnormal cell types are removed.
  VTKM_CONT bool AreAllTypesRemoved() const { return this->GetTypesToRemove() == 0xFF; }

  VTKM_DEPRECATED(2.1, "Use SetRemoveGhostField(true).")
  VTKM_CONT void RemoveGhostField() { this->SetRemoveGhostField(true); }
  VTKM_DEPRECATED(2.1, "Use SetTypesToRemoveToAll().")
  VTKM_CONT void RemoveAllGhost() { this->SetTypesToRemoveToAll(); }
  VTKM_DEPRECATED(2.1, "Use SetTypesToRemove(vals).")
  VTKM_CONT void RemoveByType(const vtkm::UInt8& vals) { this->SetTypesToRemove(vals); }

  VTKM_DEPRECATED(2.1, "Use AreAllTypesRemoved().")
  VTKM_CONT bool GetRemoveAllGhost() const { return this->AreAllTypesRemoved(); }

  /// @brief Specify whether the marked ghost cells or a named field should be used as the ghost field.
  ///
  /// When this flag is true (the default), the filter will get from the input
  /// `vtkm::cont::DataSet` the field (with the `GetGhostCellField` method). When
  /// this flag is false, the `SetActiveField` method of this class should be used
  /// to select which field to use as ghost cells.
  VTKM_CONT bool GetUseGhostCellsAsField() const { return this->UseGhostCellsAsField; }
  /// @copydoc GetUseGhostCellsAsField
  VTKM_CONT void SetUseGhostCellsAsField(bool flag) { this->UseGhostCellsAsField = flag; }

  VTKM_DEPRECATED(2.1, "Use !AreAllTypesRemoved().")
  VTKM_CONT bool GetRemoveByType() const { return !this->AreAllTypesRemoved(); }
  VTKM_DEPRECATED(2.1, "Use GetTypesToRemove().")
  VTKM_CONT vtkm::UInt8 GetRemoveType() const { return this->GetTypesToRemove(); }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  bool UseGhostCellsAsField = true;
  bool RemoveField = false;
  vtkm::UInt8 TypesToRemove = 0xFF;
};

} // namespace entity_extraction
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_entity_extraction_GhostCellRemove_h
