//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_geometry_refinement_VertexClustering_h
#define vtk_m_filter_geometry_refinement_VertexClustering_h

#include <vtkm/filter/NewFilter.h>
#include <vtkm/filter/geometry_refinement/vtkm_filter_geometry_refinement_export.h>

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{
/// \brief Reduce the number of triangles in a mesh
///
/// VertexClustering is a filter to reduce the number of triangles in a
/// triangle mesh, forming a good approximation to the original geometry. The
/// input must be a dataset that only contains triangles.
///
/// The general approach of the algorithm is to cluster vertices in a uniform
/// binning of space, accumulating to an average point within each bin. In
/// more detail, the algorithm first gets the bounds of the input poly data.
/// It then breaks this bounding volume into a user-specified number of
/// spatial bins.  It then reads each triangle from the input and hashes its
/// vertices into these bins. Then, if 2 or more vertices of
/// the triangle fall in the same bin, the triangle is dicarded.  If the
/// triangle is not discarded, it adds the triangle to the list of output
/// triangles as a list of vertex identifiers.  (There is one vertex id per
/// bin.)  After all the triangles have been read, the representative vertex
/// for each bin is computed.  This determines the spatial location of the
/// vertices of each of the triangles in the output.
///
/// To use this filter, specify the divisions defining the spatial subdivision
/// in the x, y, and z directions. Compared to algorithms such as
/// vtkQuadricClustering, a significantly higher bin count is recommended as it
/// doesn't increase the computation or memory of the algorithm and will produce
/// significantly better results.
///
/// @warning
/// This filter currently doesn't propagate cell or point fields

class VTKM_FILTER_GEOMETRY_REFINEMENT_EXPORT VertexClustering : public vtkm::filter::NewFilter
{
public:
  VTKM_CONT
  void SetNumberOfDivisions(const vtkm::Id3& num) { this->NumberOfDivisions = num; }

  VTKM_CONT
  const vtkm::Id3& GetNumberOfDivisions() const { return this->NumberOfDivisions; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Id3 NumberOfDivisions = { 256, 256, 256 };
};
} // namespace geometry_refinement
class VTKM_DEPRECATED(1.8,
                      "Use vtkm::filter::geometry_refinement::VertexClustering.") VertexClustering
  : public vtkm::filter::geometry_refinement::VertexClustering
{
  using geometry_refinement::VertexClustering::VertexClustering;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_geometry_refinement_VertexClustering_h
