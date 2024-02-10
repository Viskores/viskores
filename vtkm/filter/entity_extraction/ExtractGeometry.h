//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_fulter_entity_extraction_ExtractGeometry_h
#define vtk_m_fulter_entity_extraction_ExtractGeometry_h

#include <vtkm/ImplicitFunction.h>
#include <vtkm/filter/Filter.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
/// @brief Extract a subset of geometry based on an implicit function
///
/// Extracts from its input geometry all cells that are either
/// completely inside or outside of a specified implicit function. Any type of
/// data can be input to this filter.
///
/// To use this filter you must specify an implicit function. You must also
/// specify whether to extract cells laying inside or outside of the implicit
/// function. (The inside of an implicit function is the negative values
/// region.) An option exists to extract cells that are neither inside or
/// outside (i.e., boundary).
///
/// This differs from `vtkm::filter::contour::ClipWithImplicitFunction` in that
/// `vtkm::filter::contour::ClipWithImplicitFunction` will subdivide boundary
/// cells into new cells whereas this filter will not, producing a more "crinkly"
/// output.
///
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT ExtractGeometry : public vtkm::filter::Filter
{
public:
  /// @brief Specifies the implicit function to be used to perform extract geometry.
  ///
  /// Only a limited number of implicit functions are supported. See
  /// `vtkm::ImplicitFunctionGeneral` for information on which ones.
  ///
  VTKM_CONT
  void SetImplicitFunction(const vtkm::ImplicitFunctionGeneral& func) { this->Function = func; }

  VTKM_CONT
  const vtkm::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

  /// @brief Specify the region of the implicit function to keep cells.
  ///
  /// Determines whether to extract the geometry that is on the inside of the implicit
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

  /// @brief Specify whether cells on the boundary should be extracted.
  ///
  /// The implicit function used to extract geometry is likely to intersect some of the
  /// cells of the input. If this flag is true, then any cells intersected by the implicit
  /// function are extracted and included in the output. This flag is false by default.
  VTKM_CONT
  bool GetExtractBoundaryCells() const { return this->ExtractBoundaryCells; }
  /// @copydoc GetExtractBoundaryCells
  VTKM_CONT
  void SetExtractBoundaryCells(bool value) { this->ExtractBoundaryCells = value; }
  /// @copydoc GetExtractBoundaryCells
  VTKM_CONT
  void ExtractBoundaryCellsOn() { this->ExtractBoundaryCells = true; }
  /// @copydoc GetExtractBoundaryCells
  VTKM_CONT
  void ExtractBoundaryCellsOff() { this->ExtractBoundaryCells = false; }

  /// @brief Specify whether to extract cells only on the boundary.
  ///
  /// When this flag is off (the default), this filter extract the geometry in
  /// the region specified by the implicit function. When this flag is on, then
  /// only those cells that intersect the surface of the implicit function are
  /// extracted.
  VTKM_CONT
  bool GetExtractOnlyBoundaryCells() const { return this->ExtractOnlyBoundaryCells; }
  /// @brief GetExtractOnlyBoundaryCells
  VTKM_CONT
  void SetExtractOnlyBoundaryCells(bool value) { this->ExtractOnlyBoundaryCells = value; }
  /// @brief GetExtractOnlyBoundaryCells
  VTKM_CONT
  void ExtractOnlyBoundaryCellsOn() { this->ExtractOnlyBoundaryCells = true; }
  /// @brief GetExtractOnlyBoundaryCells
  VTKM_CONT
  void ExtractOnlyBoundaryCellsOff() { this->ExtractOnlyBoundaryCells = false; }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  bool ExtractInside = true;
  bool ExtractBoundaryCells = false;
  bool ExtractOnlyBoundaryCells = false;
  vtkm::ImplicitFunctionGeneral Function;
};
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm

#endif // vtk_m_fulter_entity_extraction_ExtractGeometry_h
