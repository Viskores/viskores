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
#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
/// \brief Removes ghost cells
///
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT GhostCellRemove : public vtkm::filter::NewFilterField
{
public:
  VTKM_CONT
  GhostCellRemove();

  VTKM_CONT
  void RemoveGhostField() { this->RemoveField = true; }
  VTKM_CONT
  void RemoveAllGhost() { this->RemoveAll = true; }
  VTKM_CONT
  void RemoveByType(const vtkm::UInt8& vals)
  {
    this->RemoveAll = false;
    this->RemoveVals = vals;
  }
  VTKM_CONT
  bool GetRemoveGhostField() const { return this->RemoveField; }
  VTKM_CONT
  bool GetRemoveAllGhost() const { return this->RemoveAll; }

  VTKM_CONT
  bool GetRemoveByType() const { return !this->RemoveAll; }
  VTKM_CONT
  vtkm::UInt8 GetRemoveType() const { return this->RemoveVals; }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  bool RemoveAll = false;
  bool RemoveField = false;
  vtkm::UInt8 RemoveVals = 0;
};

} // namespace entity_extraction
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::entity_extraction::GhostCellRemove.") GhostCellRemove
  : public vtkm::filter::entity_extraction::GhostCellRemove
{
  using entity_extraction::GhostCellRemove::GhostCellRemove;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_entity_extraction_GhostCellRemove_h
