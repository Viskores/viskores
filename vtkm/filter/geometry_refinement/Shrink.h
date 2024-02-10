//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_geometry_refinement_Shrink_h
#define vtk_m_filter_geometry_refinement_Shrink_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/geometry_refinement/vtkm_filter_geometry_refinement_export.h>

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{
/// \brief Shrink cells of an arbitrary dataset by a constant factor.
///
/// The Shrink filter shrinks the cells of a DataSet towards their centroid,
/// computed as the average position of the cell points.
/// This filter disconnects the cells, duplicating the points connected to multiple cells.
/// The resulting CellSet is always an `ExplicitCellSet`.
class VTKM_FILTER_GEOMETRY_REFINEMENT_EXPORT Shrink : public vtkm::filter::Filter
{
public:
  /// @brief Specify the scale factor to size each cell.
  ///
  /// The shrink factor specifies the ratio of the shrunk cell to its original size.
  /// This value must be between 0 and 1.
  /// A value of 1 is the same size as the input, and a value of 0 shrinks each cell to a point.
  VTKM_CONT void SetShrinkFactor(vtkm::FloatDefault factor)
  {
    this->ShrinkFactor = vtkm::Min(vtkm::Max(0, factor), 1); // Clamp shrink factor value
  }

  /// @copydoc SetShrinkFactor
  VTKM_CONT vtkm::FloatDefault GetShrinkFactor() const { return this->ShrinkFactor; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
  vtkm::FloatDefault ShrinkFactor = 0.5f;
};
} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_geometry_refinement_Shrink_h
