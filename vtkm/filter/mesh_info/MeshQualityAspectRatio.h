//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_filter_mesh_info_MeshQualityAspectRatio_h
#define vtk_m_filter_mesh_info_MeshQualityAspectRatio_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/mesh_info/vtkm_filter_mesh_info_export.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

/// @brief Compute for each cell the ratio of its longest edge to its circumradius.
///
/// This only produces values for triangles, quadrilaterals, tetrahedra, and hexahedra.
///
/// An acceptable range of this mesh for a good quality polygon is [1, 1.3], and the acceptable
/// range for a good quality polyhedron is [1, 3]. Normal values for any cell type have
/// the range [1, FLOAT_MAX].
class VTKM_FILTER_MESH_INFO_EXPORT MeshQualityAspectRatio : public vtkm::filter::Filter
{
public:
  MeshQualityAspectRatio();

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_mesh_info_MeshQualityAspectRatio_h
