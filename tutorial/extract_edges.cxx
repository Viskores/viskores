//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/Initialize.h>

#include <viskores/exec/CellEdge.h>

#include <viskores/worklet/AverageByKey.h>
#include <viskores/worklet/Keys.h>
#include <viskores/worklet/ScatterCounting.h>

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/filter/Filter.h>
#include <viskores/filter/MapFieldMergeAverage.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/worklet/WorkletMapTopology.h>

namespace
{

struct CountEdgesWorklet : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOut numEdges);
  using ExecutionSignature = _2(CellShape, PointCount);

  template <typename CellShapeTag>
  VISKORES_EXEC viskores::IdComponent operator()(CellShapeTag cellShape,
                                         viskores::IdComponent numPointsInCell) const
  {
    viskores::IdComponent numEdges;
    viskores::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, numEdges);
    return numEdges;
  }
};

struct EdgeIdsWorklet : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOut canonicalIds);
  using ExecutionSignature = void(CellShape cellShape,
                                  PointIndices globalPointIndices,
                                  VisitIndex localEdgeIndex,
                                  _2 canonicalIdOut);

  using ScatterType = viskores::worklet::ScatterCounting;

  template <typename CellShapeTag, typename PointIndexVecType>
  VISKORES_EXEC void operator()(CellShapeTag cellShape,
                            const PointIndexVecType& globalPointIndicesForCell,
                            viskores::IdComponent localEdgeIndex,
                            viskores::Id2& canonicalIdOut) const
  {
    viskores::IdComponent numPointsInCell = globalPointIndicesForCell.GetNumberOfComponents();

    viskores::exec::CellEdgeCanonicalId(
      numPointsInCell, localEdgeIndex, cellShape, globalPointIndicesForCell, canonicalIdOut);
  }
};

struct EdgeIndicesWorklet : viskores::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn keys,
                                WholeCellSetIn<> inputCells,
                                ValuesIn originCells,
                                ValuesIn originEdges,
                                ReducedValuesOut connectivityOut);
  using ExecutionSignature = void(_2 inputCells, _3 originCell, _4 originEdge, _5 connectivityOut);
  using InputDomain = _1;

  template <typename CellSetType, typename OriginCellsType, typename OriginEdgesType>
  VISKORES_EXEC void operator()(const CellSetType& cellSet,
                            const OriginCellsType& originCells,
                            const OriginEdgesType& originEdges,
                            viskores::Id2& connectivityOut) const
  {
    // Regardless of how many cells are sharing the edge we are generating, we
    // know that each cell/edge given to us by the reduce-by-key refers to the
    // same edge, so we can just look at the first cell to get the edge.
    viskores::IdComponent numPointsInCell = cellSet.GetNumberOfIndices(originCells[0]);
    viskores::IdComponent edgeIndex = originEdges[0];
    auto cellShape = cellSet.GetCellShape(originCells[0]);

    viskores::ErrorCode error;
    viskores::IdComponent pointInCellIndex0;
    error =
      viskores::exec::CellEdgeLocalIndex(numPointsInCell, 0, edgeIndex, cellShape, pointInCellIndex0);
    if (error != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(error));
      return;
    }
    viskores::IdComponent pointInCellIndex1;
    error =
      viskores::exec::CellEdgeLocalIndex(numPointsInCell, 1, edgeIndex, cellShape, pointInCellIndex1);
    if (error != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(error));
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

VISKORES_CONT bool DoMapField(
  viskores::cont::DataSet& result,
  const viskores::cont::Field& inputField,
  const viskores::worklet::ScatterCounting::OutputToInputMapType& outputToInputCellMap,
  const viskores::worklet::Keys<viskores::Id2>& cellToEdgeKeys)
{
  viskores::cont::Field outputField;

  if (inputField.IsPointField())
  {
    outputField = inputField; // pass through
  }
  else if (inputField.IsCellField())
  {
    viskores::cont::Field permuted;
    viskores::filter::MapFieldPermutation(inputField, outputToInputCellMap, permuted);
    viskores::filter::MapFieldMergeAverage(permuted, cellToEdgeKeys, outputField);
  }
  else
  {
    return false;
  }

  result.AddField(outputField);

  return true;
}

} // anonymous namespace

class ExtractEdges : public viskores::filter::Filter
{
public:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inData) override;
};

VISKORES_CONT viskores::cont::DataSet ExtractEdges::DoExecute(const viskores::cont::DataSet& inData)
{
  auto inCellSet = inData.GetCellSet();

  // First, count the edges in each cell.
  viskores::cont::ArrayHandle<viskores::IdComponent> edgeCounts;
  this->Invoke(CountEdgesWorklet{}, inCellSet, edgeCounts);

  // Second, using these counts build a scatter that repeats a cell's visit
  // for each edge in the cell.
  viskores::worklet::ScatterCounting scatter(edgeCounts);
  viskores::worklet::ScatterCounting::OutputToInputMapType outputToInputCellMap;
  outputToInputCellMap = scatter.GetOutputToInputMap(inCellSet.GetNumberOfCells());
  viskores::worklet::ScatterCounting::VisitArrayType outputToInputEdgeMap =
    scatter.GetVisitArray(inCellSet.GetNumberOfCells());

  // Third, for each edge, extract a canonical id.
  viskores::cont::ArrayHandle<viskores::Id2> canonicalIds;
  this->Invoke(EdgeIdsWorklet{}, scatter, inCellSet, canonicalIds);

  // Fourth, construct a Keys object to combine all like edge ids.
  viskores::worklet::Keys<viskores::Id2> cellToEdgeKeys;
  cellToEdgeKeys = viskores::worklet::Keys<viskores::Id2>(canonicalIds);

  // Fifth, use a reduce-by-key to extract indices for each unique edge.
  viskores::cont::ArrayHandle<viskores::Id> connectivityArray;
  this->Invoke(EdgeIndicesWorklet{},
               cellToEdgeKeys,
               inCellSet,
               outputToInputCellMap,
               outputToInputEdgeMap,
               viskores::cont::make_ArrayHandleGroupVec<2>(connectivityArray));

  // Sixth, use the created connectivity array to build a cell set.
  viskores::cont::CellSetSingleType<> outCellSet;
  outCellSet.Fill(inCellSet.GetNumberOfPoints(), viskores::CELL_SHAPE_LINE, 2, connectivityArray);

  auto mapper = [&](auto& outDataSet, const auto& f) {
    DoMapField(outDataSet, f, outputToInputCellMap, cellToEdgeKeys);
  };
  return this->CreateResult(inData, outCellSet, mapper);
}

int main(int argc, char** argv)
{
  auto opts = viskores::cont::InitializeOptions::DefaultAnyDevice;
  viskores::cont::InitializeResult config = viskores::cont::Initialize(argc, argv, opts);

  const char* input = "data/kitchen.vtk";
  viskores::io::VTKDataSetReader reader(input);
  viskores::cont::DataSet ds_from_file = reader.ReadDataSet();

  viskores::filter::contour::Contour contour;
  contour.SetActiveField("c1");
  contour.SetIsoValue(0.10);
  viskores::cont::DataSet ds_from_contour = contour.Execute(ds_from_file);

  ExtractEdges extractEdges;
  viskores::cont::DataSet wireframe = extractEdges.Execute(ds_from_contour);

  viskores::io::VTKDataSetWriter writer("out_wireframe.vtk");
  writer.WriteDataSet(wireframe);

  return 0;
}
