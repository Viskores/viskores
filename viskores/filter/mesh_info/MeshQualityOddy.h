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
#ifndef viskores_filter_mesh_info_MeshQualityOddy_h
#define viskores_filter_mesh_info_MeshQualityOddy_h

#include <viskores/filter/Filter.h>
#include <viskores/filter/mesh_info/viskores_filter_mesh_info_export.h>

namespace viskores
{
namespace filter
{
namespace mesh_info
{

/// @brief Compute for each cell the maximum deviation of a metric tensor from an identity matrix, over all corners and cell center.
///
/// This only produces values for quadrilaterals and hexahedra.
///
/// For a good quality quadrilateral or hexahedron, this value should be in the range
/// [0, 0.5]. Poorer quality cells can have unboundedly larger values.
class VISKORES_FILTER_MESH_INFO_EXPORT MeshQualityOddy : public viskores::filter::Filter
{
public:
  MeshQualityOddy();

private:
  viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& input) override;
};

} // namespace mesh_info
} // namespace filter
} // namespace viskores

#endif //viskores_filter_mesh_info_MeshQualityOddy_h
