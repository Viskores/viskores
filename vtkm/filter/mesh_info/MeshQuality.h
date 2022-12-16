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

#ifndef vtk_m_filter_mesh_info_MeshQuality_h
#define vtk_m_filter_mesh_info_MeshQuality_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/mesh_info/vtkm_filter_mesh_info_export.h>

namespace vtkm
{
namespace filter
{
namespace mesh_info
{

enum struct CellMetric
{
  Area,
  AspectGamma,
  AspectRatio,
  Condition,
  DiagonalRatio,
  Dimension,
  Jacobian,
  MaxAngle,
  MaxDiagonal,
  MinAngle,
  MinDiagonal,
  Oddy,
  RelativeSizeSquared,
  ScaledJacobian,
  Shape,
  ShapeAndSize,
  Shear,
  Skew,
  Stretch,
  Taper,
  Volume,
  Warpage,
  None
};

/** \brief Computes the quality of an unstructured cell-based mesh. The quality is defined in terms of the
  * summary statistics (frequency, mean, variance, min, max) of metrics computed over the mesh
  * cells. One of several different metrics can be specified for a given cell type, and the mesh
  * can consist of one or more different cell types. The resulting mesh quality is stored as one
  * or more new fields in the output dataset of this filter, with a separate field for each cell type.
  * Each field contains the metric summary statistics for the cell type.
  * Summary statists with all 0 values imply that the specified metric does not support the cell type.
  */
class VTKM_FILTER_MESH_INFO_EXPORT MeshQuality : public vtkm::filter::FilterField
{
public:
  VTKM_CONT explicit MeshQuality(CellMetric);

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  CellMetric MyMetric;
};
} // namespace mesh_info
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_mesh_info_MeshQuality_h
