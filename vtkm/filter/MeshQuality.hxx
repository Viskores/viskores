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
//=========================================================================

#include "vtkm/cont/DynamicCellSet.h"
#include "vtkm/cont/ErrorFilterExecution.h"
#include "vtkm/cont/Field.h"
#include "vtkm/filter/internal/CreateResult.h"
#include "vtkm/worklet/DispatcherMapTopology.h"

#define DEBUG_PRINT

namespace vtkm
{
namespace filter
{

namespace debug
{
#ifdef DEBUG_PRINT
//----------------------------------------------------------------------------
template <typename T, typename S = vtkm::cont::DeviceAdapterId>
void MeshQualityDebug(const vtkm::cont::ArrayHandle<T, S>& outputArray, const char* name)
{
  typedef vtkm::cont::internal::Storage<T, S> StorageType;
  typedef typename StorageType::PortalConstType PortalConstType;
  PortalConstType readPortal = outputArray.GetPortalConstControl();
  vtkm::Id numElements = readPortal.GetNumberOfValues();
  std::cout << name << "= " << numElements << " [";
  for (vtkm::Id i = 0; i < numElements; i++)
    std::cout << (int)readPortal.Get(i) << " ";
  std::cout << "]\n";
}
#else
template <typename T, typename S>
void MeshQualityDebug(const vtkm::cont::ArrayHandle<T, S>& vtkmNotUsed(outputArray),
                      const char* vtkmNotUsed(name))
{
}
#endif
} // namespace debug


inline VTKM_CONT MeshQuality::MeshQuality(
  const std::vector<vtkm::Pair<vtkm::UInt8, CellMetric>>& metrics)
  : vtkm::filter::FilterCell<MeshQuality>()
{
  this->SetUseCoordinateSystemAsField(true);
  this->CellTypeMetrics.assign(vtkm::NUMBER_OF_CELL_SHAPES, CellMetric::EMPTY);
  for (auto p : metrics)
    this->CellTypeMetrics[p.first] = p.second;
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet MeshQuality::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& points,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  VTKM_ASSERT(fieldMeta.IsPointField());

  using Algorithm = vtkm::cont::Algorithm;
  using ShapeHandle = vtkm::cont::ArrayHandle<vtkm::UInt8>;
  using IdHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using QualityWorklet = vtkm::worklet::MeshQuality<CellMetric>;
  using FieldStatsWorklet = vtkm::worklet::FieldStatistics<T>;

  //TODO: Should other cellset types be supported?
  vtkm::cont::CellSetExplicit<> cellSet;
  input.GetCellSet(this->GetActiveCellSetIndex()).CopyTo(cellSet);

  ShapeHandle cellShapes =
    cellSet.GetShapesArray(vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

  //Obtain the frequency counts of each cell type in the input dataset
  IdHandle uniqueCellCounts;
  ShapeHandle uniqueCellShapes, sortedShapes;
  Algorithm::Copy(cellShapes, sortedShapes);
  Algorithm::Sort(sortedShapes);
  Algorithm::ReduceByKey(
    sortedShapes,
    vtkm::cont::make_ArrayHandleConstant(vtkm::Id(1), cellShapes.GetNumberOfValues()),
    uniqueCellShapes,
    uniqueCellCounts,
    vtkm::Add());

  std::cout << "uniqueCellCounts: " << uniqueCellCounts.GetNumberOfValues() << "\n";

  const vtkm::Id numUniqueShapes = uniqueCellShapes.GetNumberOfValues();
  auto uniqueCellShapesPortal = uniqueCellShapes.GetPortalConstControl();
  auto numCellsPerShapePortal = uniqueCellCounts.GetPortalConstControl();
  std::vector<vtkm::Id> tempCounts(vtkm::NUMBER_OF_CELL_SHAPES);
  for (vtkm::Id i = 0; i < numUniqueShapes; i++)
    tempCounts[uniqueCellShapesPortal.Get(i)] = numCellsPerShapePortal.Get(i);
  IdHandle cellShapeCounts = vtkm::cont::make_ArrayHandle(tempCounts);
  std::cout << "cellShapeCounts: " << cellShapeCounts.GetNumberOfValues() << "\n";

  //Invoke the MeshQuality worklet
  vtkm::cont::ArrayHandle<T> outArray;
  vtkm::cont::ArrayHandle<CellMetric> cellMetrics = vtkm::cont::make_ArrayHandle(CellTypeMetrics);
  std::cout << "cellMetrics: " << cellMetrics.GetNumberOfValues() << "\n";
  vtkm::worklet::DispatcherMapTopology<QualityWorklet> dispatcher;
  dispatcher.Invoke(
    vtkm::filter::ApplyPolicy(cellSet, policy), cellShapeCounts, cellMetrics, points, outArray);

  //Build the output dataset: a separate field for each cell type that has a specified metric
  vtkm::cont::DataSet result;
  result.CopyStructure(input); //clone of the input dataset

  auto cellShapePortal = cellShapes.GetPortalConstControl();
  auto metricValuesPortal = outArray.GetPortalConstControl();

  const vtkm::Id numCells = outArray.GetNumberOfValues();
  T currMetric = 0;
  vtkm::UInt8 currShape = 0;

  //Output metric values stored in separate containers
  //based on shape type. Unsupported shape types in VTK-m
  //are represented with an empty "placeholder" container.
  std::vector<std::vector<T>> metricValsPerShape = {
    { /*placeholder*/ }, { /*vertices*/ },  { /*placeholder*/ },  { /*lines*/ },
    { /*placeholder*/ }, { /*triangles*/ }, { /*placeholder*/ },  { /*polygons*/ },
    { /*placeholder*/ }, { /*quads*/ },     { /*tetrahedrons*/ }, { /*placeholder*/ },
    { /*hexahedrons*/ }, { /*wedges*/ },    { /*pyramids*/ }
  };

  for (vtkm::Id metricArrayIndex = 0; metricArrayIndex < numCells; metricArrayIndex++)
  {
    currShape = cellShapePortal.Get(metricArrayIndex);
    currMetric = metricValuesPortal.Get(metricArrayIndex);
    metricValsPerShape[currShape].emplace_back(currMetric);
  }

  //Compute the mesh quality for each shape type. This consists
  //of computing the summary statistics of the metric values for
  //each cell of the given shape type.
  std::string fieldName = "", metricName = "";
  vtkm::UInt8 cellShape = 0;
  vtkm::Id cellCount = 0;
  bool skipShape = false;
  for (vtkm::Id shapeIndex = 0; shapeIndex < numUniqueShapes; shapeIndex++)
  {
    cellShape = uniqueCellShapesPortal.Get(shapeIndex);
    cellCount = numCellsPerShapePortal.Get(shapeIndex);
    metricName = MetricNames[static_cast<vtkm::UInt8>(CellTypeMetrics[cellShape])];

    //Skip over shapes with an empty/unspecified metric;
    //don't include a field for them
    if (CellTypeMetrics[cellShape] == CellMetric::EMPTY)
      continue;

    switch (cellShape)
    {
      case vtkm::CELL_SHAPE_EMPTY:
        skipShape = true;
        break;
      case vtkm::CELL_SHAPE_VERTEX:
        fieldName = "vertices";
        break;
      case vtkm::CELL_SHAPE_LINE:
        fieldName = "lines";
        break;
      case vtkm::CELL_SHAPE_TRIANGLE:
        fieldName = "triangles";
        break;
      case vtkm::CELL_SHAPE_POLYGON:
        fieldName = "polygons";
        break;
      case vtkm::CELL_SHAPE_QUAD:
        fieldName = "quads";
        break;
      case vtkm::CELL_SHAPE_TETRA:
        fieldName = "tetrahedrons";
        break;
      case vtkm::CELL_SHAPE_HEXAHEDRON:
        fieldName = "hexahedrons";
        break;
      case vtkm::CELL_SHAPE_WEDGE:
        fieldName = "wedges";
        break;
      case vtkm::CELL_SHAPE_PYRAMID:
        fieldName = "pyramids";
        break;
      default:
        skipShape = true;
        break;
    }

    //Skip over shapes of empty cell type; don't include a field for them
    if (skipShape)
      continue;

    fieldName += "-" + metricName;
    auto shapeMetricVals = metricValsPerShape[cellShape];
    auto shapeMetricValsHandle = vtkm::cont::make_ArrayHandle(std::move(shapeMetricVals));

    //Invoke the field stats worklet on the array of metric values for this shape type
    typename FieldStatsWorklet::StatInfo statinfo;
    FieldStatsWorklet().Run(shapeMetricValsHandle, statinfo);

    //Retrieve summary stats from the output stats struct.
    //These stats define the mesh quality with respect to this shape type.
    std::vector<T> shapeMeshQuality = {
      T(cellCount), statinfo.mean, statinfo.variance, statinfo.minimum, statinfo.maximum
    };

    //Append the summary stats into the output dataset as a new field
    result.AddField(vtkm::cont::make_Field(fieldName,
                                           vtkm::cont::Field::Association::CELL_SET,
                                           "cells",
                                           shapeMeshQuality,
                                           vtkm::CopyFlag::On));

    std::cout << "-----------------------------------------------------\n"
              << "Mesh quality of " << fieldName << ":\n"
              << "Number of cells: " << cellCount << "\n"
              << "Mean:            " << statinfo.mean << "\n"
              << "Variance:        " << statinfo.variance << "\n"
              << "Minimum:         " << statinfo.minimum << "\n"
              << "Maximum:         " << statinfo.maximum << "\n"
              << "-----------------------------------------------------\n";
  }

  auto metricValsPortal = outArray.GetPortalConstControl();
  std::cout << "-----------------------------------------------------\n"
            << "Metric values - all cells:\n";
  for (vtkm::Id v = 0; v < outArray.GetNumberOfValues(); v++)
    std::cout << metricValsPortal.Get(v) << "\n";
  std::cout << "-----------------------------------------------------\n";

  //Append the metric values of all cells into the output
  //dataset as a new field
  std::string s = "allCells-metricValues";
  result.AddField(
    vtkm::cont::Field(s, vtkm::cont::Field::Association::CELL_SET, "cells", outArray));

  return result;
}

} // namespace filter
} // namespace vtkm
