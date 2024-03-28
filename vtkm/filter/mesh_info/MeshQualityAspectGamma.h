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
#ifndef vtk_m_filter_mesh_info_MeshQualityAspectGamma_h
#define vtk_m_filter_mesh_info_MeshQualityAspectGamma_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/mesh_info/vtkm_filter_mesh_info_export.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

/// @brief For each cell, compute the normalized root-mean-square of the edge lengths.
///
/// This only produces values for tetrahedra.
///
/// The root-mean-square edge length is normalized to the volume such that the value is
/// 1 for an equilateral tetrahedron. The acceptable range for good quality meshes is
/// considered to be [1, 3]. The normal range of values is [1, FLOAT_MAX].
class VTKM_FILTER_MESH_INFO_EXPORT MeshQualityAspectGamma : public vtkm::filter::Filter
{
public:
  MeshQualityAspectGamma();

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_mesh_info_MeshQualityAspectGamma_h
