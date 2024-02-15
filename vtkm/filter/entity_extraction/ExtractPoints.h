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

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
/// @brief Extract only points from a geometry using an implicit function
///
/// Extract only the  points that are either inside or outside of a
/// VTK-m implicit function. Examples include planes, spheres, boxes,
/// etc.
///
/// Note that while any geometry type can be provided as input, the output is
/// represented by an explicit representation of points using
/// `vtkm::cont::CellSetSingleType` with one vertex cell per point.
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT ExtractPoints : public vtkm::filter::Filter
{
public:
  /// @brief Option to remove unused points and compact result int a smaller array.
  ///
  /// When CompactPoints is on, instead of copying the points and point fields
  /// from the input, the filter will create new compact fields without the
  /// unused elements.
  /// When off (the default), unused points will remain listed in the topology,
  /// but point fields and coordinate systems will be shallow-copied to the output.
  VTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  /// @copydoc GetCompactPoints
  VTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  /// @brief Specifies the implicit function to be used to perform extract points.
  ///
  /// Only a limited number of implicit functions are supported. See
  /// `vtkm::ImplicitFunctionGeneral` for information on which ones.
  ///
  void SetImplicitFunction(const vtkm::ImplicitFunctionGeneral& func) { this->Function = func; }

  const vtkm::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

  /// @brief Specify the region of the implicit function to keep points.
  ///
  /// Determines whether to extract the points that are on the inside of the implicit
  /// function (where the function is less than 0) or the outside (where the function is
  /// greater than 0). This flag is true by default (i.e., the interior of the implicit
  /// function will be extracted).
  VTKM_CONT
  bool GetExtractInside() const { return this->ExtractInside; }
  /// @copydoc GetExtractInside
  VTKM_CONT
  void SetExtractInside(bool value) { this->ExtractInside = value; }
  /// @copydoc GetExtractInside
  VTKM_CONT
  void ExtractInsideOn() { this->ExtractInside = true; }
  /// @copydoc GetExtractInside
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
} // namespace filter
} // namespace vtkm

#endif // vtkm_filter_entity_extraction_ExtractPoints_h
