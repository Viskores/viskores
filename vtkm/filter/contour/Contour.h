//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_contour_Contour_h
#define vtk_m_filter_contour_Contour_h

#include <vtkm/filter/contour/AbstractContour.h>
#include <vtkm/filter/contour/vtkm_filter_contour_export.h>

namespace vtkm
{
namespace filter
{
namespace contour
{

/// \brief Generate contours or isosurfaces from a region of space.
///
/// `Contour` takes as input a mesh, often a volume, and generates on
/// output one or more surfaces where a field equals a specified value.
///
/// This filter implements multiple algorithms for contouring, and the best algorithm
/// will be selected based on the type of the input.
///
/// The scalar field to extract the contour from is selected with the `SetActiveField()`
/// and related methods.
///
class VTKM_FILTER_CONTOUR_EXPORT Contour : public vtkm::filter::contour::AbstractContour
{
public:
  VTKM_DEPRECATED(2.1, "Use SetComputeFastNormals.")
  VTKM_CONT void SetComputeFastNormalsForStructured(bool on) { this->SetComputeFastNormals(on); }
  VTKM_DEPRECATED(2.1, "Use GetComputeFastNormals.")
  VTKM_CONT bool GetComputeFastNormalsForStructured() const
  {
    return this->GetComputeFastNormals();
  }

  VTKM_DEPRECATED(2.1, "Use SetComputeFastNormals.")
  VTKM_CONT void SetComputeFastNormalsForUnstructured(bool on) { this->SetComputeFastNormals(on); }
  VTKM_DEPRECATED(2.1, "Use GetComputeFastNormals.")
  VTKM_CONT bool GetComputeFastNormalsForUnstructured() const
  {
    return this->GetComputeFastNormals();
  }

protected:
  // Needed by the subclass Slice
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& result) override;
};
} // namespace contour
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_Contour_h
