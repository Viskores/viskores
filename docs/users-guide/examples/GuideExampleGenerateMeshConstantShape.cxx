//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetSingleType.h>

#include <vtkm/exec/CellEdge.h>

#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/MapFieldPermutation.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace vtkm
{
namespace worklet
{

namespace
{

////
//// BEGIN-EXAMPLE GenerateMeshConstantShapeCount
////
struct CountEdgesWorklet : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOut numEdges);
  using ExecutionSignature = _2(CellShape, PointCount);
  using InputDomain = _1;

  template<typename CellShapeTag>
  VTKM_EXEC_CONT vtkm::IdComponent operator()(CellShapeTag cellShape,
                                              vtkm::IdComponent numPointsInCell) const
  {
    vtkm::IdComponent numEdges;
    vtkm::ErrorCode status =
      vtkm::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, numEdges);
    if (status != vtkm::ErrorCode::Success)
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
class EdgeIndicesWorklet : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet, FieldOut connectivityOut);
  using ExecutionSignature = void(CellShape, PointIndices, _2, VisitIndex);
  using InputDomain = _1;

  using ScatterType = vtkm::worklet::ScatterCounting;

  template<typename CellShapeTag, typename PointIndexVecType>
  VTKM_EXEC void operator()(CellShapeTag cellShape,
                            const PointIndexVecType& globalPointIndicesForCell,
                            vtkm::Id2& connectivityOut,
                            vtkm::IdComponent edgeIndex) const
  {
    vtkm::IdComponent numPointsInCell =
      globalPointIndicesForCell.GetNumberOfComponents();

    vtkm::ErrorCode status;
    vtkm::IdComponent pointInCellIndex0;
    status = vtkm::exec::CellEdgeLocalIndex(
      numPointsInCell, 0, edgeIndex, cellShape, pointInCellIndex0);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(status));
      return;
    }
    vtkm::IdComponent pointInCellIndex1;
    status = vtkm::exec::CellEdgeLocalIndex(
      numPointsInCell, 1, edgeIndex, cellShape, pointInCellIndex1);
    if (status != vtkm::ErrorCode::Success)
    {
      this->RaiseError(vtkm::ErrorString(status));
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
} // namespace vtkm

////
//// BEGIN-EXAMPLE ExtractEdgesFilterDeclaration
////
namespace vtkm
{
namespace filter
{
namespace entity_extraction
{

//// PAUSE-EXAMPLE
namespace
{

#define VTKM_FILTER_ENTITY_EXTRACTION_EXPORT

//// RESUME-EXAMPLE
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT ExtractEdges : public vtkm::filter::Filter
{
public:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;
};

//// PAUSE-EXAMPLE
} // anonymous namespace
//// RESUME-EXAMPLE
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
////
//// END-EXAMPLE ExtractEdgesFilterDeclaration
////

namespace vtkm
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
inline VTKM_CONT vtkm::cont::DataSet ExtractEdges::DoExecute(
  const vtkm::cont::DataSet& inData)
{
  auto inCellSet = inData.GetCellSet();

  // Count number of edges in each cell.
  vtkm::cont::ArrayHandle<vtkm::IdComponent> edgeCounts;
  this->Invoke(vtkm::worklet::CountEdgesWorklet{}, inCellSet, edgeCounts);

  // Build the scatter object (for non 1-to-1 mapping of input to output)
  vtkm::worklet::ScatterCounting scatter(edgeCounts);
  //// LABEL GetOutputToInputMap
  auto outputToInputCellMap = scatter.GetOutputToInputMap(inCellSet.GetNumberOfCells());

  vtkm::cont::ArrayHandle<vtkm::Id> connectivityArray;
  //// LABEL InvokeEdgeIndices
  this->Invoke(vtkm::worklet::EdgeIndicesWorklet{},
               scatter,
               inCellSet,
               vtkm::cont::make_ArrayHandleGroupVec<2>(connectivityArray));

  vtkm::cont::CellSetSingleType<> outCellSet;
  outCellSet.Fill(
    inCellSet.GetNumberOfPoints(), vtkm::CELL_SHAPE_LINE, 2, connectivityArray);

  // This lambda function maps an input field to the output data set. It is
  // used with the CreateResult method.
  //// LABEL FieldMapper
  auto fieldMapper = [&](vtkm::cont::DataSet& outData,
                         const vtkm::cont::Field& inputField) {
    if (inputField.IsCellField())
    {
      //// LABEL MapField
      vtkm::filter::MapFieldPermutation(inputField, outputToInputCellMap, outData);
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
} // namespace vtkm

namespace
{

void CheckOutput(const vtkm::cont::CellSetSingleType<>& cellSet)
{
  std::cout << "Num cells: " << cellSet.GetNumberOfCells() << std::endl;
  VTKM_TEST_ASSERT(cellSet.GetNumberOfCells() == 12 + 8 + 6 + 9, "Wrong # of cells.");

  auto connectivity = cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell(),
                                                   vtkm::TopologyElementTagPoint());
  std::cout << "Connectivity:" << std::endl;
  vtkm::cont::printSummary_ArrayHandle(connectivity, std::cout, true);

  auto connectivityPortal = connectivity.ReadPortal();
  VTKM_TEST_ASSERT(connectivityPortal.Get(0) == 0, "Bad edge index");
  VTKM_TEST_ASSERT(connectivityPortal.Get(1) == 1, "Bad edge index");
  VTKM_TEST_ASSERT(connectivityPortal.Get(2) == 1, "Bad edge index");
  VTKM_TEST_ASSERT(connectivityPortal.Get(3) == 5, "Bad edge index");
  VTKM_TEST_ASSERT(connectivityPortal.Get(68) == 9, "Bad edge index");
  VTKM_TEST_ASSERT(connectivityPortal.Get(69) == 10, "Bad edge index");
}

void TryFilter()
{
  std::cout << std::endl << "Trying calling filter." << std::endl;
  vtkm::cont::DataSet inDataSet =
    vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

  vtkm::filter::entity_extraction::ExtractEdges filter;

  vtkm::cont::DataSet outDataSet = filter.Execute(inDataSet);

  vtkm::cont::CellSetSingleType<> outCellSet;
  outDataSet.GetCellSet().AsCellSet(outCellSet);
  CheckOutput(outCellSet);

  vtkm::cont::Field outCellField = outDataSet.GetField("cellvar");
  VTKM_TEST_ASSERT(outCellField.GetAssociation() ==
                     vtkm::cont::Field::Association::Cells,
                   "Cell field not cell field.");
  vtkm::cont::ArrayHandle<vtkm::Float32> outCellData;
  outCellField.GetData().AsArrayHandle(outCellData);
  std::cout << "Cell field:" << std::endl;
  vtkm::cont::printSummary_ArrayHandle(outCellData, std::cout, true);
  VTKM_TEST_ASSERT(outCellData.GetNumberOfValues() == outCellSet.GetNumberOfCells(),
                   "Bad size of field.");

  auto cellFieldPortal = outCellData.ReadPortal();
  VTKM_TEST_ASSERT(test_equal(cellFieldPortal.Get(0), 100.1), "Bad field value.");
  VTKM_TEST_ASSERT(test_equal(cellFieldPortal.Get(1), 100.1), "Bad field value.");
  VTKM_TEST_ASSERT(test_equal(cellFieldPortal.Get(34), 130.5), "Bad field value.");
}

void DoTest()
{
  TryFilter();
}

} // anonymous namespace

int GuideExampleGenerateMeshConstantShape(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
