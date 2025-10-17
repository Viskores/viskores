//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/CellSetSingleType.h>

#include <viskores/exec/CellEdge.h>

#include <viskores/worklet/Keys.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapTopology.h>
#include <viskores/worklet/WorkletReduceByKey.h>

#include <viskores/filter/Filter.h>
#include <viskores/filter/MapFieldMergeAverage.h>
#include <viskores/filter/MapFieldPermutation.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE GenerateMeshCombineLikeCount
////
struct CountEdgesWorklet : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOut numEdges);
  using ExecutionSignature = _2(CellShape, PointCount);
  using InputDomain = _1;

  template<typename CellShapeTag>
  VISKORES_EXEC_CONT viskores::IdComponent operator()(
    CellShapeTag cellShape,
    viskores::IdComponent numPointsInCell) const
  {
    viskores::IdComponent numEdges;
    viskores::ErrorCode status =
      viskores::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, numEdges);
    if (status != viskores::ErrorCode::Success)
    {
      // There is an error in the cell. As good as it would be to return an
      // error, we probably don't want to invalidate the entire run if there
      // is just one malformed cell. Instead, ignore the cell.
      return 0;
    }
    return numEdges;
  }
};
////
//// END-EXAMPLE GenerateMeshCombineLikeCount
////

////
//// BEGIN-EXAMPLE GenerateMeshCombineLikeGenIds
////
class EdgeIdsWorklet : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet, FieldOut canonicalIds);
  using ExecutionSignature = void(CellShape cellShape,
                                  PointIndices globalPointIndices,
                                  VisitIndex localEdgeIndex,
                                  _2 canonicalIdOut);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterCounting;

  template<typename CellShapeTag, typename PointIndexVecType>
  VISKORES_EXEC void operator()(CellShapeTag cellShape,
                                const PointIndexVecType& globalPointIndicesForCell,
                                viskores::IdComponent localEdgeIndex,
                                viskores::Id2& canonicalIdOut) const
  {
    viskores::IdComponent numPointsInCell =
      globalPointIndicesForCell.GetNumberOfComponents();

    viskores::ErrorCode status =
      viskores::exec::CellEdgeCanonicalId(numPointsInCell,
                                          localEdgeIndex,
                                          cellShape,
                                          globalPointIndicesForCell,
                                          canonicalIdOut);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
    }
  }
};
////
//// END-EXAMPLE GenerateMeshCombineLikeGenIds
////

////
//// BEGIN-EXAMPLE GenerateMeshCombineLikeGenIndices
////
class EdgeIndicesWorklet : public viskores::worklet::WorkletReduceByKey
{
public:
  using ControlSignature = void(KeysIn keys,
                                WholeCellSetIn<> inputCells,
                                ValuesIn originCells,
                                ValuesIn originEdges,
                                ReducedValuesOut connectivityOut);
  using ExecutionSignature = void(_2 inputCells,
                                  _3 originCell,
                                  _4 originEdge,
                                  _5 connectivityOut);
  using InputDomain = _1;

  template<typename CellSetType, typename OriginCellsType, typename OriginEdgesType>
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

    viskores::IdComponent pointInCellIndex0;
    viskores::exec::CellEdgeLocalIndex(
      numPointsInCell, 0, edgeIndex, cellShape, pointInCellIndex0);
    viskores::IdComponent pointInCellIndex1;
    viskores::exec::CellEdgeLocalIndex(
      numPointsInCell, 1, edgeIndex, cellShape, pointInCellIndex1);

    auto globalPointIndicesForCell = cellSet.GetIndices(originCells[0]);
    connectivityOut[0] = globalPointIndicesForCell[pointInCellIndex0];
    connectivityOut[1] = globalPointIndicesForCell[pointInCellIndex1];
  }
};
////
//// END-EXAMPLE GenerateMeshCombineLikeGenIndices
////
} // anonymous namespace

namespace viskores
{
namespace filter
{
namespace entity_extraction
{

//// PAUSE-EXAMPLE
namespace
{

//// RESUME-EXAMPLE
class ExtractEdges : public viskores::filter::Filter
{
protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(
    const viskores::cont::DataSet& inData) override;
};

//// PAUSE-EXAMPLE
} // anonymous namespace
//// RESUME-EXAMPLE
} // namespace entity_extraction
} // namespace filter
} // namespace viskores

namespace viskores
{
namespace filter
{
namespace entity_extraction
{

//// PAUSE-EXAMPLE
namespace
{

//// RESUME-EXAMPLE
////
//// BEGIN-EXAMPLE GenerateMeshCombineLikeInvoke
////
inline VISKORES_CONT viskores::cont::DataSet ExtractEdges::DoExecute(
  const viskores::cont::DataSet& inData)
{
  auto inCellSet = inData.GetCellSet();

  // First, count the edges in each cell.
  viskores::cont::ArrayHandle<viskores::IdComponent> edgeCounts;
  this->Invoke(CountEdgesWorklet{}, inCellSet, edgeCounts);

  // Second, using these counts build a scatter that repeats a cell's visit
  // for each edge in the cell.
  viskores::worklet::ScatterCounting scatter(edgeCounts);
  viskores::worklet::ScatterCounting::VisitArrayType outputToInputEdgeMap =
    scatter.GetVisitArray(inCellSet.GetNumberOfCells());

  // Third, for each edge, extract a canonical id.
  viskores::cont::ArrayHandle<viskores::Id2> canonicalIds;
  this->Invoke(EdgeIdsWorklet{}, scatter, inCellSet, canonicalIds);

  // Fourth, construct a Keys object to combine all like edge ids.
  viskores::worklet::Keys<viskores::Id2> cellToEdgeKeys(canonicalIds);

  // Fifth, use a reduce-by-key to extract indices for each unique edge.
  viskores::cont::ArrayHandle<viskores::Id> connectivityArray;
  //// LABEL InvokeEdgeIndices
  this->Invoke(EdgeIndicesWorklet{},
               cellToEdgeKeys,
               inCellSet,
               scatter.GetOutputToInputMap(inCellSet.GetNumberOfCells()),
               outputToInputEdgeMap,
               viskores::cont::make_ArrayHandleGroupVec<2>(connectivityArray));

  // Sixth, use the created connectivity array to build a cell set.
  viskores::cont::CellSetSingleType<> outCellSet;
  outCellSet.Fill(
    inCellSet.GetNumberOfPoints(), viskores::CELL_SHAPE_LINE, 2, connectivityArray);

  ////
  //// BEGIN-EXAMPLE GenerateMeshCombineLikeMapCellField
  ////
  //// LABEL FieldMapperBegin
  // Finally, we need to create an output data set. A lambda expression is one
  // of the easiest ways to map fields from the input to the output with the
  // CreateResult method.
  auto fieldMapper =
    [&](viskores::cont::DataSet& outData, const viskores::cont::Field& inField)
  {
    if (inField.IsCellField())
    {
      // New cells were created. Need to find cells that created the output.
      // First, the cells were subselected with a scatter. Use the
      // output-to-input array from the scatter to permute the array.
      viskores::cont::Field subselectionField;
      viskores::filter::MapFieldPermutation(
        inField,
        scatter.GetOutputToInputMap(inCellSet.GetNumberOfCells()),
        subselectionField);
      // Next, coicident edges are combined together. Use the keys object
      // for combining the cells to average out the cell values.
      viskores::filter::MapFieldMergeAverage(subselectionField, cellToEdgeKeys, outData);
    }
    else
    {
      outData.AddField(inField); // Pass through
    }
    //// LABEL FieldMapperEnd
  };

  //// LABEL CreateResult
  return this->CreateResult(inData, outCellSet, fieldMapper);
  ////
  //// END-EXAMPLE GenerateMeshCombineLikeMapCellField
  ////
}
////
//// END-EXAMPLE GenerateMeshCombineLikeInvoke
////

//// PAUSE-EXAMPLE
} // anonymous namespace

//// RESUME-EXAMPLE
} // namespace entity_extraction
} // namespace filter
} // namespace viskores

namespace
{

void CheckOutput(const viskores::cont::CellSetSingleType<>& cellSet)
{
  std::cout << "Num cells: " << cellSet.GetNumberOfCells() << std::endl;
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == 22, "Wrong # of cells.");

  auto connectivity = cellSet.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                   viskores::TopologyElementTagPoint());
  std::cout << "Connectivity:" << std::endl;
  viskores::cont::printSummary_ArrayHandle(connectivity, std::cout, true);

  auto connectivityPortal = connectivity.ReadPortal();
  VISKORES_TEST_ASSERT(connectivityPortal.Get(0) == 0, "Bad edge index");
  VISKORES_TEST_ASSERT(connectivityPortal.Get(1) == 1, "Bad edge index");
  VISKORES_TEST_ASSERT(connectivityPortal.Get(2) == 0, "Bad edge index");
  VISKORES_TEST_ASSERT(connectivityPortal.Get(3) == 3, "Bad edge index");
  VISKORES_TEST_ASSERT(connectivityPortal.Get(42) == 9, "Bad edge index");
  VISKORES_TEST_ASSERT(connectivityPortal.Get(43) == 10, "Bad edge index");
}

void TryFilter()
{
  std::cout << std::endl << "Trying calling filter." << std::endl;
  viskores::cont::DataSet inDataSet =
    viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

  viskores::filter::entity_extraction::ExtractEdges filter;

  viskores::cont::DataSet outDataSet = filter.Execute(inDataSet);

  viskores::cont::CellSetSingleType<> outCellSet;
  outDataSet.GetCellSet().AsCellSet(outCellSet);
  CheckOutput(outCellSet);

  viskores::cont::Field outCellField = outDataSet.GetField("cellvar");
  VISKORES_TEST_ASSERT(outCellField.GetAssociation() ==
                         viskores::cont::Field::Association::Cells,
                       "Cell field not cell field.");
  viskores::cont::ArrayHandle<viskores::Float32> outCellData;
  outCellField.GetData().AsArrayHandle(outCellData);
  std::cout << "Cell field:" << std::endl;
  viskores::cont::printSummary_ArrayHandle(outCellData, std::cout, true);
  VISKORES_TEST_ASSERT(outCellData.GetNumberOfValues() == outCellSet.GetNumberOfCells(),
                       "Bad size of field.");

  auto cellFieldPortal = outCellData.ReadPortal();
  VISKORES_TEST_ASSERT(test_equal(cellFieldPortal.Get(0), 100.1), "Bad field value.");
  VISKORES_TEST_ASSERT(test_equal(cellFieldPortal.Get(1), 100.1), "Bad field value.");
  VISKORES_TEST_ASSERT(test_equal(cellFieldPortal.Get(21), 130.5), "Bad field value.");
}

void DoTest()
{
  TryFilter();
}

} // anonymous namespace

int GuideExampleGenerateMeshCombineLike(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
