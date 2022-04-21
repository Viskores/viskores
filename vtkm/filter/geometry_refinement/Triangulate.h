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

#include <vtkm/filter/NewFilter.h>
#include <vtkm/filter/geometry_refinement/vtkm_filter_geometry_refinement_export.h>

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{
class VTKM_FILTER_GEOMETRY_REFINEMENT_EXPORT Triangulate : public vtkm::filter::NewFilter
{
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace geometry_refinement
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::geometry_refinement::Triangulate.") Triangulate
  : public vtkm::filter::geometry_refinement::Triangulate
{
  using geometry_refinement::Triangulate::Triangulate;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_geometry_refinement_Triangulate_h
