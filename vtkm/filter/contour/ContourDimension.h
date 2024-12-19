//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_contour_ContourDimension_h
#define vtk_m_filter_contour_ContourDimension_h

namespace vtkm
{
namespace filter
{
namespace contour
{

/// @brief Identifies what type cells will be contoured.
///
/// The `ContourDimension` enum is used by the contour filters to specify which
/// dimension of cell to contour by.
enum struct ContourDimension
{
  /// @copydoc vtkm::filter::contour::AbstractContour::SetInputCellDimensionToAuto
  Auto,
  /// @copydoc vtkm::filter::contour::AbstractContour::SetInputCellDimensionToAll
  All,
  /// @copydoc vtkm::filter::contour::AbstractContour::SetInputCellDimensionToPolyhedra
  Polyhedra,
  /// @copydoc vtkm::filter::contour::AbstractContour::SetInputCellDimensionToPolygons
  Polygons,
  /// @copydoc vtkm::filter::contour::AbstractContour::SetInputCellDimensionToLines
  Lines
};

}
}
} // namespace vtkm::filter::contour

#endif // vtk_m_filter_contour_ContourDimension_h
