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
#ifndef vtk_m_filter_mesh_info_MeshQualityShapeAndSize_h
#define vtk_m_filter_mesh_info_MeshQualityShapeAndSize_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/mesh_info/vtkm_filter_mesh_info_export.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

/// @brief Compute a metric for each cell based on the shape scaled by the cell size.
///
/// This filter multiplies the values of the shape metric by the relative size squared
/// metric. See `vtkm::filter::mesh_info::MeshQualityShape` and
/// `vtkm::filter::mesh_info::MeshQualityRelativeSizeSquared` for details on each of
/// those metrics.
///
/// This only produces values for triangles, quadrilaterals, tetrahedra, and hexahedra.
///
/// For a good quality cell, this value will be in the range [0.2, 1]. Poorer quality
/// cells can have values as low as 0.
class VTKM_FILTER_MESH_INFO_EXPORT MeshQualityShapeAndSize : public vtkm::filter::Filter
{
public:
  MeshQualityShapeAndSize();

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_mesh_info_MeshQualityShapeAndSize_h
