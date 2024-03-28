//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_geometry_refinement_Triangulate_h
#define vtk_m_filter_geometry_refinement_Triangulate_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/geometry_refinement/vtkm_filter_geometry_refinement_export.h>

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{

/// @brief Convert all polygons of a `vtkm::cont::DataSet` into triangles.
///
/// Note that although the triangles will occupy the same space of the cells that
/// they replace, the interpolation of point fields within these cells might differ.
/// For example, the first order interpolation of a quadrilateral uses bilinear
/// interpolation, which actually results in quadratic equations. This differs from the
/// purely linear field in a triangle, so the triangle replacement of the quadrilateral
/// will not have exactly the same interpolation.
class VTKM_FILTER_GEOMETRY_REFINEMENT_EXPORT Triangulate : public vtkm::filter::Filter
{
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};

} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_geometry_refinement_Triangulate_h
