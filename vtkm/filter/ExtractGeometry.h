//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ExtractGeometry_h
#define vtk_m_filter_ExtractGeometry_h

#include <vtkm/cont/ImplicitFunctionHandle.h>
#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/ExtractGeometry.h>

namespace vtkm
{
namespace filter
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
class ExtractGeometry : public vtkm::filter::FilterDataSet<ExtractGeometry>
{
public:
  //currently the ExtractGeometry filter only works on scalar data.
  using SupportedTypes = TypeListScalarAll;

  VTKM_CONT
  ExtractGeometry();

  // Set the volume of interest to extract
  void SetImplicitFunction(const vtkm::cont::ImplicitFunctionHandle& func)
  {
    this->Function = func;
  }

  const vtkm::cont::ImplicitFunctionHandle& GetImplicitFunction() const { return this->Function; }

  VTKM_CONT
  bool GetExtractInside() { return this->ExtractInside; }
  VTKM_CONT
  void SetExtractInside(bool value) { this->ExtractInside = value; }
  VTKM_CONT
  void ExtractInsideOn() { this->ExtractInside = true; }
  VTKM_CONT
  void ExtractInsideOff() { this->ExtractInside = false; }

  VTKM_CONT
  bool GetExtractBoundaryCells() { return this->ExtractBoundaryCells; }
  VTKM_CONT
  void SetExtractBoundaryCells(bool value) { this->ExtractBoundaryCells = value; }
  VTKM_CONT
  void ExtractBoundaryCellsOn() { this->ExtractBoundaryCells = true; }
  VTKM_CONT
  void ExtractBoundaryCellsOff() { this->ExtractBoundaryCells = false; }

  VTKM_CONT
  bool GetExtractOnlyBoundaryCells() { return this->ExtractOnlyBoundaryCells; }
  VTKM_CONT
  void SetExtractOnlyBoundaryCells(bool value) { this->ExtractOnlyBoundaryCells = value; }
  VTKM_CONT
  void ExtractOnlyBoundaryCellsOn() { this->ExtractOnlyBoundaryCells = true; }
  VTKM_CONT
  void ExtractOnlyBoundaryCellsOff() { this->ExtractOnlyBoundaryCells = false; }

  template <typename DerivedPolicy>
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

  //Map a new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  bool DoMapField(vtkm::cont::DataSet& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  bool ExtractInside;
  bool ExtractBoundaryCells;
  bool ExtractOnlyBoundaryCells;
  vtkm::cont::ImplicitFunctionHandle Function;
  vtkm::worklet::ExtractGeometry Worklet;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ExtractGeometry.hxx>

#endif // vtk_m_filter_ExtractGeometry_h
