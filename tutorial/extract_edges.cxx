//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/Initialize.h>

#include <vtkm/exec/CellEdge.h>

#include <vtkm/worklet/AverageByKey.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/ScatterCounting.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/MapFieldMergeAverage.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace
{

struct CountEdgesWorklet : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOut numEdges);
  using ExecutionSignature = _2(CellShape, PointCount);

  template <typename CellShapeTag>
  VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag cellShape,
                                         vtkm::IdComponent numPointsInCell) const
  {
    vtkm::IdComponent numEdges;
    vtkm::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, numEdges);
    return numEdges;
  }
};

struct EdgeIdsWorklet : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOut canonicalIds);
  using ExecutionSignature = void(CellShape cellShape,
                                  PointIndices globalPointIndices,
                                  VisitIndex localEdgeIndex,
                                  _2 canonicalIdOut);

  using ScatterType = vtkm::worklet::ScatterCounting;

  template <typename CellShapeTag, typename PointIndexVecType>
  VTKM_EXEC void operator()(CellShapeTag cellShape,
                            const PointIndexVecType& globalPointIndicesForCell,
                            vtkm::IdComponent localEdgeIndex,
                            vtkm::Id2& canonicalIdOut) const
  {
    vtkm::IdComponent numPointsInCell = globalPointIndicesForCell.GetNumberOfComponents();

    vtkm::exec::CellEdgeCanonicalId(
      numPointsInCell, localEdgeIndex, cellShape, globalPointIndicesForCell, canonicalIdOut);
  }
};

struct EdgeIndicesWorklet : vtkm::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn keys,
                                WholeCellSetIn<> inputCells,
                                ValuesIn originCells,
                                ValuesIn originEdges,
                                ReducedValuesOut connectivityOut);
  using ExecutionSignature = void(_2 inputCells, _3 originCell, _4 originEdge, _5 connectivityOut);
  using InputDomain = _1;

  template <typename CellSetType, typename OriginCellsType, typename OriginEdgesType>
  VTKM_EXEC void operator()(const CellSetType& cellSet,
                            const OriginCellsType& originCells,
                            const OriginEdgesType& originEdges,
                            vtkm::Id2& connectivityOut) const
  {
    // Regardless of how many cells are sharing the edge we are generating, we
    // know that each cell/edge given to us by the reduce-by-key refers to the
    // same edge, so we can just look at the first cell to get the edge.
    vtkm::IdComponent numPointsInCell = cellSet.GetNumberOfIndices(originCells[0]);
    vtkm::IdComponent edgeIndex = originEdges[0];
    auto cellShape = cellSet.GetCellShape(originCells[0]);

    vtkm::ErrorCode error;
    vtkm::IdComponent pointInCellIndex0;
    error =
      vtkm::exec::CellEdgeLocalIndex(numPointsInCell, 0, edgeIndex, cellShape, pointInCellIndex0);
    if (error != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(error));
      return;
    }
    vtkm::IdComponent pointInCellIndex1;
    error =
      vtkm::exec::CellEdgeLocalIndex(numPointsInCell, 1, edgeIndex, cellShape, pointInCellIndex1);
    if (error != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(error));
      return;
    }

    auto globalPointIndicesForCell = cellSet.GetIndices(originCells[0]);
    connectivityOut[0] = globalPointIndicesForCell[pointInCellIndex0];
    connectivityOut[1] = globalPointIndicesForCell[pointInCellIndex1];
  }
};

}

namespace
{

VTKM_CONT bool DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::Field& inputField,
  const vtkm::worklet::ScatterCounting::OutputToInputMapType& outputToInputCellMap,
  const vtkm::worklet::Keys<vtkm::Id2>& cellToEdgeKeys)
{
  vtkm::cont::Field outputField;

  if (inputField.IsPointField())
  {
    outputField = inputField; // pass through
  }
  else if (inputField.IsCellField())
  {
    vtkm::cont::Field permuted;
    vtkm::filter::MapFieldPermutation(inputField, outputToInputCellMap, permuted);
    vtkm::filter::MapFieldMergeAverage(permuted, cellToEdgeKeys, outputField);
  }
  else
  {
    return false;
  }

  result.AddField(outputField);

  return true;
}

} // anonymous namespace

class ExtractEdges : public vtkm::filter::Filter
{
public:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;
};

VTKM_CONT vtkm::cont::DataSet ExtractEdges::DoExecute(const vtkm::cont::DataSet& inData)
{
  auto inCellSet = inData.GetCellSet();

  // First, count the edges in each cell.
  vtkm::cont::ArrayHandle<vtkm::IdComponent> edgeCounts;
  this->Invoke(CountEdgesWorklet{}, inCellSet, edgeCounts);

  // Second, using these counts build a scatter that repeats a cell's visit
  // for each edge in the cell.
  vtkm::worklet::ScatterCounting scatter(edgeCounts);
  vtkm::worklet::ScatterCounting::OutputToInputMapType outputToInputCellMap;
  outputToInputCellMap = scatter.GetOutputToInputMap(inCellSet.GetNumberOfCells());
  vtkm::worklet::ScatterCounting::VisitArrayType outputToInputEdgeMap =
    scatter.GetVisitArray(inCellSet.GetNumberOfCells());

  // Third, for each edge, extract a canonical id.
  vtkm::cont::ArrayHandle<vtkm::Id2> canonicalIds;
  this->Invoke(EdgeIdsWorklet{}, scatter, inCellSet, canonicalIds);

  // Fourth, construct a Keys object to combine all like edge ids.
  vtkm::worklet::Keys<vtkm::Id2> cellToEdgeKeys;
  cellToEdgeKeys = vtkm::worklet::Keys<vtkm::Id2>(canonicalIds);

  // Fifth, use a reduce-by-key to extract indices for each unique edge.
  vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray;
  this->Invoke(EdgeIndicesWorklet{},
               cellToEdgeKeys,
               inCellSet,
               outputToInputCellMap,
               outputToInputEdgeMap,
               vtkm::cont::make_ArrayHandleGroupVec<2>(connectivityArray));

  // Sixth, use the created connectivity array to build a cell set.
  vtkm::cont::CellSetSingleType<> outCellSet;
  outCellSet.Fill(inCellSet.GetNumberOfPoints(), vtkm::CELL_SHAPE_LINE, 2, connectivityArray);

  auto mapper = [&](auto& outDataSet, const auto& f) {
    DoMapField(outDataSet, f, outputToInputCellMap, cellToEdgeKeys);
  };
  return this->CreateResult(inData, outCellSet, mapper);
}

int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  const char* input = "data/kitchen.vtk";
  vtkm::io::VTKDataSetReader reader(input);
  vtkm::cont::DataSet ds_from_file = reader.ReadDataSet();

  vtkm::filter::contour::Contour contour;
  contour.SetActiveField("c1");
  contour.SetIsoValue(0.10);
  vtkm::cont::DataSet ds_from_contour = contour.Execute(ds_from_file);

  ExtractEdges extractEdges;
  vtkm::cont::DataSet wireframe = extractEdges.Execute(ds_from_contour);

  vtkm::io::VTKDataSetWriter writer("out_wireframe.vtk");
  writer.WriteDataSet(wireframe);

  return 0;
}
