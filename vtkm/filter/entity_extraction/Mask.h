//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_entity_extraction_Mask_h
#define vtk_m_filter_entity_extraction_Mask_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
/// \brief Subselect cells using a stride
///
/// Extract only every Nth cell where N is equal to a stride value
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT Mask : public vtkm::filter::Filter
{
public:
  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the unused elements
  VTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  // Set the stride of the subsample
  VTKM_CONT
  vtkm::Id GetStride() const { return this->Stride; }
  VTKM_CONT
  void SetStride(vtkm::Id& stride) { this->Stride = stride; }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Id Stride = 1;
  bool CompactPoints = false;
};
} // namespace entity_extraction
} // namespace filter
} // namespace vtk

#endif // vtk_m_filter_entity_extraction_Mask_h
