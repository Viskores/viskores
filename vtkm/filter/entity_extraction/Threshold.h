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

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
/// \brief Extracts cells which satisfy threshold criterion
///
/// Extracts all cells from any dataset type that satisfy a threshold criterion.
/// The output of this filter is an permutation of the input dataset.
///
/// You can threshold either on point or cell fields
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT Threshold : public vtkm::filter::FilterField
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

  /// @brief Set the threshold criterion to pass any value <= to the specified value.
  VTKM_CONT
  void SetThresholdBelow(vtkm::Float64 value);

  /// @brief Set the threshold criterion to pass any value >= to the specified value.
  VTKM_CONT
  void SetThresholdAbove(vtkm::Float64 value);

  /// @brief Set the threshold criterion to pass any value between (inclusive) the given values.
  VTKM_CONT
  void SetThresholdBetween(vtkm::Float64 value1, vtkm::Float64 value2);

  ///@{
  /// @brief For multi-component fields, select how to apply the threshold criterion.
  /// The default is to test the 0th component.
  VTKM_CONT
  void SetComponentToTest(vtkm::IdComponent component)
  {
    this->ComponentMode = Component::Selected;
    this->SelectedComponent = component;
  }
  VTKM_CONT
  void SetComponentToTestToAny() { this->ComponentMode = Component::Any; }
  VTKM_CONT
  void SetComponentToTestToAll() { this->ComponentMode = Component::All; }
  ///@}

  /// @brief If using field from point data, all values for all points in a cell must
  /// satisfy the threshold criterion if `AllInRange` is set. Otherwise, just a
  /// single point's value satisfying the threshold criterion will extract the cell.
  VTKM_CONT
  void SetAllInRange(bool value) { this->AllInRange = value; }
  VTKM_CONT
  bool GetAllInRange() const { return this->AllInRange; }

  /// @brief Invert the threshold result, i.e. cells that would have been in the output with this
  /// option off are excluded, while cells that would have been excluded from the output are
  /// included.
  VTKM_CONT
  void SetInvert(bool value) { this->Invert = value; }
  VTKM_CONT
  bool GetInvert() const { return this->Invert; }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  double LowerValue = 0;
  double UpperValue = 0;

  enum struct Component
  {
    Any,
    All,
    Selected
  };

  Component ComponentMode = Component::Selected;
  vtkm::IdComponent SelectedComponent = 0;

  bool AllInRange = false;
  bool Invert = false;
};
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_entity_extraction_Threshold_h
