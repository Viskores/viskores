//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandlePermutation.h>
#include <viskores/cont/CellSetSingleType.h>

#include <viskores/exec/CellEdge.h>

#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/filter/Filter.h>
#include <viskores/filter/MapFieldPermutation.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace viskores
{
namespace worklet
{

namespace
{

////
//// BEGIN-EXAMPLE GenerateMeshConstantShapeCount
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
//// END-EXAMPLE GenerateMeshConstantShapeCount
////

////
//// BEGIN-EXAMPLE GenerateMeshConstantShapeGenIndices
////
class EdgeIndicesWorklet : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet, FieldOut connectivityOut);
  using ExecutionSignature = void(CellShape, PointIndices, _2, VisitIndex);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterCounting;

  template<typename CellShapeTag, typename PointIndexVecType>
  VISKORES_EXEC void operator()(CellShapeTag cellShape,
                                const PointIndexVecType& globalPointIndicesForCell,
                                viskores::Id2& connectivityOut,
                                viskores::IdComponent edgeIndex) const
  {
    viskores::IdComponent numPointsInCell =
      globalPointIndicesForCell.GetNumberOfComponents();

    viskores::ErrorCode status;
    viskores::IdComponent pointInCellIndex0;
    status = viskores::exec::CellEdgeLocalIndex(
      numPointsInCell, 0, edgeIndex, cellShape, pointInCellIndex0);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
      return;
    }
    viskores::IdComponent pointInCellIndex1;
    status = viskores::exec::CellEdgeLocalIndex(
      numPointsInCell, 1, edgeIndex, cellShape, pointInCellIndex1);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
      return;
    }

    connectivityOut[0] = globalPointIndicesForCell[pointInCellIndex0];
    connectivityOut[1] = globalPointIndicesForCell[pointInCellIndex1];
  }
};
////
//// END-EXAMPLE GenerateMeshConstantShapeGenIndices
////

} // anonymous namespace

} // namespace worklet
} // namespace viskores

////
//// BEGIN-EXAMPLE ExtractEdgesFilterDeclaration
////
namespace viskores
{
namespace filter
{
namespace entity_extraction
{

//// PAUSE-EXAMPLE
namespace
{

#define VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT

//// RESUME-EXAMPLE
class VISKORES_FILTER_ENTITY_EXTRACTION_EXPORT ExtractEdges
  : public viskores::filter::Filter
{
public:
  VISKORES_CONT viskores::cont::DataSet DoExecute(
    const viskores::cont::DataSet& inData) override;
};

//// PAUSE-EXAMPLE
} // anonymous namespace
//// RESUME-EXAMPLE
} // namespace entity_extraction
} // namespace filter
} // namespace viskores
////
//// END-EXAMPLE ExtractEdgesFilterDeclaration
////

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
// TODO: It would be nice if there was a simpler example of DoExecute.
////
//// BEGIN-EXAMPLE ExtractEdgesFilterDoExecute
////
////
//// BEGIN-EXAMPLE GenerateMeshConstantShapeInvoke
////
inline VISKORES_CONT viskores::cont::DataSet ExtractEdges::DoExecute(
  const viskores::cont::DataSet& inData)
{
  auto inCellSet = inData.GetCellSet();

  // Count number of edges in each cell.
  viskores::cont::ArrayHandle<viskores::IdComponent> edgeCounts;
  this->Invoke(viskores::worklet::CountEdgesWorklet{}, inCellSet, edgeCounts);

  // Build the scatter object (for non 1-to-1 mapping of input to output)
  viskores::worklet::ScatterCounting scatter(edgeCounts);
  //// LABEL GetOutputToInputMap
  auto outputToInputCellMap = scatter.GetOutputToInputMap(inCellSet.GetNumberOfCells());

  viskores::cont::ArrayHandle<viskores::Id> connectivityArray;
  //// LABEL InvokeEdgeIndices
  this->Invoke(viskores::worklet::EdgeIndicesWorklet{},
               scatter,
               inCellSet,
               viskores::cont::make_ArrayHandleGroupVec<2>(connectivityArray));

  viskores::cont::CellSetSingleType<> outCellSet;
  outCellSet.Fill(
    inCellSet.GetNumberOfPoints(), viskores::CELL_SHAPE_LINE, 2, connectivityArray);

  // This lambda function maps an input field to the output data set. It is
  // used with the CreateResult method.
  //// LABEL FieldMapper
  auto fieldMapper =
    [&](viskores::cont::DataSet& outData, const viskores::cont::Field& inputField)
  {
    if (inputField.IsCellField())
    {
      //// LABEL MapField
      viskores::filter::MapFieldPermutation(inputField, outputToInputCellMap, outData);
    }
    else
    {
      outData.AddField(inputField); // pass through
    }
  };

  //// LABEL CreateResult
  return this->CreateResult(inData, outCellSet, fieldMapper);
}
////
//// END-EXAMPLE GenerateMeshConstantShapeInvoke
////
////
//// END-EXAMPLE ExtractEdgesFilterDoExecute
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
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == 12 + 8 + 6 + 9,
                       "Wrong # of cells.");

  auto connectivity = cellSet.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                   viskores::TopologyElementTagPoint());
  std::cout << "Connectivity:" << std::endl;
  viskores::cont::printSummary_ArrayHandle(connectivity, std::cout, true);

  auto connectivityPortal = connectivity.ReadPortal();
  VISKORES_TEST_ASSERT(connectivityPortal.Get(0) == 0, "Bad edge index");
  VISKORES_TEST_ASSERT(connectivityPortal.Get(1) == 1, "Bad edge index");
  VISKORES_TEST_ASSERT(connectivityPortal.Get(2) == 1, "Bad edge index");
  VISKORES_TEST_ASSERT(connectivityPortal.Get(3) == 5, "Bad edge index");
  VISKORES_TEST_ASSERT(connectivityPortal.Get(68) == 9, "Bad edge index");
  VISKORES_TEST_ASSERT(connectivityPortal.Get(69) == 10, "Bad edge index");
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
  VISKORES_TEST_ASSERT(test_equal(cellFieldPortal.Get(34), 130.5), "Bad field value.");
}

void DoTest()
{
  TryFilter();
}

} // anonymous namespace

int GuideExampleGenerateMeshConstantShape(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
