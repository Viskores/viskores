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
/// \brief generate isosurface(s) from a Volume

/// Takes as input a volume (e.g., 3D structured point set) and generates on
/// output one or more isosurfaces.
/// Multiple contour values must be specified to generate the isosurfaces.
/// This filter automatically selects the right implmentation between Marching Cells
/// and Flying Edges algorithms depending on the type of input \c DataSet : Flying Edges
/// is only available for 3-dimensional datasets using uniform point coordinates.
/// @warning
/// This filter is currently only supports 3D volumes.
class VTKM_FILTER_CONTOUR_EXPORT Contour : public vtkm::filter::contour::AbstractContour
{
public:
  /// Set/Get whether the fast path should be used for normals computation for
  /// structured datasets. Off by default.
  VTKM_DEPRECATED(2.1, "Use SetComputeFastNormals.")
  VTKM_CONT void SetComputeFastNormalsForStructured(bool on) { this->SetComputeFastNormals(on); }
  VTKM_DEPRECATED(2.1, "Use GetComputeFastNormals.")
  VTKM_CONT bool GetComputeFastNormalsForStructured() const
  {
    return this->GetComputeFastNormals();
  }

  /// Set/Get whether the fast path should be used for normals computation for
  /// unstructured datasets. On by default.
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
