//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_entity_extraction_Threshold_h
#define vtk_m_filter_entity_extraction_Threshold_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
/// \brief Extracts cells where scalar value in cell satisfies threshold criterion
///
/// Extracts all cells from any dataset type that
/// satisfy a threshold criterion. A cell satisfies the criterion if the
/// scalar value of every point or cell satisfies the criterion. The
/// criterion takes the form of between two values. The output of this
/// filter is an permutation of the input dataset.
///
/// You can threshold either on point or cell fields
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT Threshold : public vtkm::filter::NewFilterField
{
public:
  VTKM_CONT
  void SetLowerThreshold(vtkm::Float64 value) { this->LowerValue = value; }
  VTKM_CONT
  void SetUpperThreshold(vtkm::Float64 value) { this->UpperValue = value; }

  VTKM_CONT
  vtkm::Float64 GetLowerThreshold() const { return this->LowerValue; }
  VTKM_CONT
  vtkm::Float64 GetUpperThreshold() const { return this->UpperValue; }

  //If using scalars from point data, all scalars for all points in a cell must
  //satisfy the threshold criterion if AllScalars is set. Otherwise, just a
  //single scalar value satisfying the threshold criterion will extract the cell.
  VTKM_CONT
  void SetAllInRange(bool value) { this->ReturnAllInRange = value; }

  VTKM_CONT
  bool GetAllInRange() const { return this->ReturnAllInRange; }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  double LowerValue = 0;
  double UpperValue = 0;
  bool ReturnAllInRange = false;
};
} // namespace entity_extraction
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::entity_extraction::Threshold.") Threshold
  : public vtkm::filter::entity_extraction::Threshold
{
  using entity_extraction::Threshold::Threshold;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_entity_extraction_Threshold_h
