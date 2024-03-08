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
#ifndef vtk_m_filter_mesh_info_MeshQualityShape_h
#define vtk_m_filter_mesh_info_MeshQualityShape_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/mesh_info/vtkm_filter_mesh_info_export.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

/// @brief Compute a shape-based metric for each cell.
///
/// This metric is based on the condition number of the Jacobian matrix.
///
/// This only produces values for triangles, quadrilaterals, tetrahedra, and hexahedra.
///
/// For good quality triangles, the acceptable range is [0.25, 1]. Good quality quadrilaterals,
/// tetrahedra, hexahedra are in the range [0.3, 1].  Poorer quality cells can have values
/// as low as 0.
///
///
class VTKM_FILTER_MESH_INFO_EXPORT MeshQualityShape : public vtkm::filter::Filter
{
public:
  MeshQualityShape();

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_mesh_info_MeshQualityShape_h
