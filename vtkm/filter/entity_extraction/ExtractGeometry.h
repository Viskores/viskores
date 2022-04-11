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
#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
/// \brief  Extract a subset of geometry based on an implicit function
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
/// This differs from Clip in that Clip will subdivide boundary cells into new
/// cells, while this filter will not, producing a more 'crinkly' output.
///
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT ExtractGeometry : public vtkm::filter::NewFilterField
{
public:
  // Set the volume of interest to extract
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

  VTKM_CONT
  bool GetExtractBoundaryCells() const { return this->ExtractBoundaryCells; }
  VTKM_CONT
  void SetExtractBoundaryCells(bool value) { this->ExtractBoundaryCells = value; }
  VTKM_CONT
  void ExtractBoundaryCellsOn() { this->ExtractBoundaryCells = true; }
  VTKM_CONT
  void ExtractBoundaryCellsOff() { this->ExtractBoundaryCells = false; }

  VTKM_CONT
  bool GetExtractOnlyBoundaryCells() const { return this->ExtractOnlyBoundaryCells; }
  VTKM_CONT
  void SetExtractOnlyBoundaryCells(bool value) { this->ExtractOnlyBoundaryCells = value; }
  VTKM_CONT
  void ExtractOnlyBoundaryCellsOn() { this->ExtractOnlyBoundaryCells = true; }
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
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::entity_extraction::ExtractGeometry.") ExtractGeometry
  : public vtkm::filter::entity_extraction::ExtractGeometry
{
  using entity_extraction::ExtractGeometry::ExtractGeometry;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_fulter_entity_extraction_ExtractGeometry_h
