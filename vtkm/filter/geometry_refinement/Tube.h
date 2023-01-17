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

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/geometry_refinement/vtkm_filter_geometry_refinement_export.h>

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{
/// \brief generate tube geometry from polylines.

/// Takes as input a set of polylines, radius, num sides and capping flag.
/// Produces tubes along each polyline

class VTKM_FILTER_GEOMETRY_REFINEMENT_EXPORT Tube : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  void SetRadius(vtkm::FloatDefault r) { this->Radius = r; }

  VTKM_CONT
  void SetNumberOfSides(vtkm::Id n) { this->NumberOfSides = n; }

  VTKM_CONT
  void SetCapping(bool v) { this->Capping = v; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::FloatDefault Radius{};
  vtkm::Id NumberOfSides{};
  bool Capping{};
};
} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_geometry_refinement_Tube_h
