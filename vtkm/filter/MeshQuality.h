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

#ifndef vtk_m_filter_MeshQuality_h
#define vtk_m_filter_MeshQuality_h

#include "vtkm/CellShape.h"
#include "vtkm/filter/FilterCell.h"
#include "vtkm/worklet/MeshQuality.h"
#include <vtkm/worklet/FieldStatistics.h>

namespace vtkm
{
namespace filter
{

//Names of the available cell metrics, for use in
//the output dataset fields
//TODO: static const?
static const std::string MetricNames[] = { "empty",
                                           "diagonalRatio",
                                           "edgeRatio",
                                           //"skew",
                                           "oddy",
                                           "relativeSizeSquared",
                                           "volume" };

//Different cell metrics available to use
//TODO: static const?
enum class CellMetric
{
  EMPTY, //0
  DIAGONAL_RATIO,
  EDGE_RATIO,
  ODDY,
  RELATIVE_SIZE,
  VOLUME,
  NUMBER_OF_CELL_METRICS //(num metrics = NUMBER_OF_CELL_METRICS - 2)
};

/** \brief Computes the quality of an unstructured cell-based mesh. The quality is defined in terms of the
  * summary statistics (frequency, mean, variance, min, max) of metrics computed over the mesh
  * cells. One of several different metrics can be specified for a given cell type, and the mesh
  * can consist of one or more different cell types. The resulting mesh quality is stored as one
  * or more new fields in the output dataset of this filter, with a separate field for each cell type.
  * Each field contains the metric summary statistics for the cell type.
  * Summary statists with all 0 values imply that the specified metric does not support the cell type.
  */
class MeshQuality : public vtkm::filter::FilterCell<MeshQuality>
{
public:
  using ShapeMetricsVecType = std::vector<vtkm::Pair<vtkm::UInt8, CellMetric>>;

  VTKM_CONT MeshQuality(const ShapeMetricsVecType& metrics);

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& points,
    const vtkm::filter::FieldMetadata& fieldMeta,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy);

private:
  //A user-assigned cell metric per shape/cell type
  //Empty metric if not provided by user
  //Length of vector is the number of different VTK-m cell types
  std::vector<CellMetric> CellTypeMetrics;
};


template <>
class FilterTraits<vtkm::filter::MeshQuality>
{
public:
  using InputFieldTypeList = vtkm::TypeListTagFieldVec3;
};


} // namespace filter
} // namespace vtkm

#include <vtkm/filter/MeshQuality.hxx>

#endif // vtk_m_filter_MeshQuality_h
