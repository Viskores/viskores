//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_geometry_refinement_SplitSharpEdges_h
#define vtk_m_filter_geometry_refinement_SplitSharpEdges_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/geometry_refinement/vtkm_filter_geometry_refinement_export.h>

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{
/// \brief Split sharp manifold edges where the feature angle between the
///  adjacent surfaces are larger than the threshold value
///
/// Split sharp manifold edges where the feature angle between the adjacent
/// surfaces are larger than the threshold value. When an edge is split, it
/// would add a new point to the coordinates and update the connectivity of
/// an adjacent surface.
/// Ex. there are two adjacent triangles(0,1,2) and (2,1,3). Edge (1,2) needs
/// to be split. Two new points 4(duplication of point 1) an 5(duplication of point 2)
/// would be added and the later triangle's connectivity would be changed
/// to (5,4,3).
/// By default, all old point's fields would be copied to the new point.
/// Use with caution.
class VTKM_FILTER_GEOMETRY_REFINEMENT_EXPORT SplitSharpEdges : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  void SetFeatureAngle(vtkm::FloatDefault value) { this->FeatureAngle = value; }

  VTKM_CONT
  vtkm::FloatDefault GetFeatureAngle() const { return this->FeatureAngle; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::FloatDefault FeatureAngle = 30.0;
};
} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_geometry_refinement_SplitSharpEdges_h
