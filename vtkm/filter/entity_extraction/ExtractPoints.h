//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_filter_entity_extraction_ExtractPoints_h
#define vtkm_filter_entity_extraction_ExtractPoints_h

#include <vtkm/ImplicitFunction.h>

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
#include <vtkm/cont/ImplicitFunctionHandle.h>
#endif //VTKM_NO_DEPRECATED_VIRTUAL

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
/// @brief Extract only points from a geometry using an implicit function
///
///
/// Extract only the  points that are either inside or outside of a
/// VTK-m implicit function. Examples include planes, spheres, boxes,
/// etc.
/// Note that while any geometry type can be provided as input, the output is
/// represented by an explicit representation of points using
/// vtkm::cont::CellSetSingleType
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT ExtractPoints : public vtkm::filter::NewFilterField
{
public:
  /// When CompactPoints is set, instead of copying the points and point fields
  /// from the input, the filter will create new compact fields without the unused elements
  VTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  /// Set the volume of interest to extract
  void SetImplicitFunction(const vtkm::ImplicitFunctionGeneral& func) { this->Function = func; }

  const vtkm::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

  VTKM_CONT
  bool GetExtractInside() const { return this->ExtractInside; }
  VTKM_CONT
  void SetExtractInside(bool value) { this->ExtractInside = value; }
  VTKM_CONT
  void ExtractInsideOn() { this->ExtractInside = true; }
  VTKM_CONT
  void ExtractInsideOff() { this->ExtractInside = false; }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  bool ExtractInside = true;
  vtkm::ImplicitFunctionGeneral Function;

  bool CompactPoints = false;
};
} // namespace entity_extraction
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::entity_extraction::ExtractPoints.") ExtractPoints
  : public vtkm::filter::entity_extraction::ExtractPoints
{
  using entity_extraction::ExtractPoints::ExtractPoints;
};
} // namespace filter
} // namespace vtkm

#endif // vtkm_filter_entity_extraction_ExtractPoints_h
