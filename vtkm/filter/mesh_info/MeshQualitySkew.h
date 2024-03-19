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
#ifndef vtk_m_filter_mesh_info_MeshQualitySkew_h
#define vtk_m_filter_mesh_info_MeshQualitySkew_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/mesh_info/vtkm_filter_mesh_info_export.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

/// @brief Compute the skew of each cell.
///
/// The skew is computed as the dot product between unit vectors in the principal directions.
/// (For 3D objects, the skew is taken as the maximum of all planes.)
///
/// This only produces values for quadrilaterals and hexahedra.
///
/// Good quality cells will have a skew in the range [0, 0.5]. A unit square or cube will
/// have a skew of 0. Poor quality cells can have a skew up to 1 although a malformed cell
/// might have its skew be infinite.
class VTKM_FILTER_MESH_INFO_EXPORT MeshQualitySkew : public vtkm::filter::Filter
{
public:
  MeshQualitySkew();

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_mesh_info_MeshQualitySkew_h
