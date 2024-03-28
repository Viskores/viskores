//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_geometry_refinement_Tube_h
#define vtk_m_filter_geometry_refinement_Tube_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/geometry_refinement/vtkm_filter_geometry_refinement_export.h>

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{

/// @brief Generate a tube around each line and polyline.
///
/// The radius, number of sides, and end capping can be specified for each tube.
/// The orientation of the geometry of the tube are computed automatically using
/// a heuristic to minimize the twisting along the input data set.
///
class VTKM_FILTER_GEOMETRY_REFINEMENT_EXPORT Tube : public vtkm::filter::Filter
{
public:
  /// @brief Specify the radius of each tube.
  VTKM_CONT void SetRadius(vtkm::FloatDefault r) { this->Radius = r; }

  /// @brief Specify the number of sides for each tube.
  ///
  /// The tubes are generated using a polygonal approximation. This option determines
  /// how many facets will be generated around the tube.
  VTKM_CONT void SetNumberOfSides(vtkm::Id n) { this->NumberOfSides = n; }

  /// The `Tube` filter can optionally add a cap at the ends of each tube. This option
  /// specifies whether that cap is generated.
  VTKM_CONT void SetCapping(bool v) { this->Capping = v; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::FloatDefault Radius{};
  vtkm::Id NumberOfSides = 6;
  bool Capping = false;
};
} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_geometry_refinement_Tube_h
