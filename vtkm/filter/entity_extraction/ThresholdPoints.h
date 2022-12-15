//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_entity_extraction_ThresholdPoints_h
#define vtk_m_filter_entity_extraction_ThresholdPoints_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT ThresholdPoints : public vtkm::filter::FilterField
{
public:
  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the unused elements
  VTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  VTKM_CONT
  vtkm::Float64 GetLowerThreshold() const { return this->LowerValue; }
  VTKM_CONT
  void SetLowerThreshold(vtkm::Float64 value) { this->LowerValue = value; }

  VTKM_CONT
  vtkm::Float64 GetUpperThreshold() const { return this->UpperValue; }
  VTKM_CONT
  void SetUpperThreshold(vtkm::Float64 value) { this->UpperValue = value; }

  VTKM_CONT
  void SetThresholdBelow(vtkm::Float64 value);
  VTKM_CONT
  void SetThresholdAbove(vtkm::Float64 value);
  VTKM_CONT
  void SetThresholdBetween(vtkm::Float64 value1, vtkm::Float64 value2);

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  constexpr static int THRESHOLD_BELOW = 0;
  constexpr static int THRESHOLD_ABOVE = 1;
  constexpr static int THRESHOLD_BETWEEN = 2;

  double LowerValue = 0;
  double UpperValue = 0;
  int ThresholdType = THRESHOLD_BETWEEN;

  bool CompactPoints = false;
};
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_entity_extraction_ThresholdPoints_h
