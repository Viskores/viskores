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

#include <viskores/Hash.h>

#include <viskores/worklet/AverageByKey.h>
#include <viskores/worklet/Keys.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapTopology.h>
#include <viskores/worklet/WorkletReduceByKey.h>

#include <viskores/filter/Filter.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

//#define CHECK_COLLISIONS

namespace
{

////
//// BEGIN-EXAMPLE GenerateMeshHashCount
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
//// END-EXAMPLE GenerateMeshHashCount
////

////
//// BEGIN-EXAMPLE GenerateMeshHashGenHashes
////
class EdgeHashesWorklet : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet, FieldOut hashValues);
  using ExecutionSignature = _2(CellShape cellShape,
                                PointIndices globalPointIndices,
                                VisitIndex localEdgeIndex);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterCounting;

  template<typename CellShapeTag, typename PointIndexVecType>
  VISKORES_EXEC viskores::HashType operator()(
    CellShapeTag cellShape,
    const PointIndexVecType& globalPointIndicesForCell,
    viskores::IdComponent localEdgeIndex) const
  {
    viskores::IdComponent numPointsInCell =
      globalPointIndicesForCell.GetNumberOfComponents();
    viskores::Id2 canonicalId;
    viskores::ErrorCode status =
      viskores::exec::CellEdgeCanonicalId(numPointsInCell,
                                          localEdgeIndex,
                                          cellShape,
                                          globalPointIndicesForCell,
                                          canonicalId);
    if (status != viskores::ErrorCode::Success)
    {
      this->RaiseError(viskores::ErrorString(status));
      return viskores::HashType(-1);
    }
    //// PAUSE-EXAMPLE
#ifndef CHECK_COLLISIONS
    //// RESUME-EXAMPLE
    return viskores::Hash(canonicalId);
    //// PAUSE-EXAMPLE
#else  // ! CHECK_COLLISIONS
    // Intentionally use a bad hash value to cause collisions to check to make
    // sure that collision resolution works.
    return viskores::HashType(canonicalId[0]);
#endif // !CHECK_COLLISIONS
    //// RESUME-EXAMPLE
  }
};
////
//// END-EXAMPLE GenerateMeshHashGenHashes
////

////
//// BEGIN-EXAMPLE GenerateMeshHashResolveCollisions
////
class EdgeHashCollisionsWorklet : public viskores::worklet::WorkletReduceByKey
{
public:
  using ControlSignature = void(KeysIn keys,
                                WholeCellSetIn<> inputCells,
                                ValuesIn originCells,
                                ValuesIn originEdges,
                                ValuesOut localEdgeIndices,
                                ReducedValuesOut numEdges);
  using ExecutionSignature = _6(_2 inputCells,
                                _3 originCells,
                                _4 originEdges,
                                _5 localEdgeIndices);
  using InputDomain = _1;

  template<typename CellSetType,
           typename OriginCellsType,
           typename OriginEdgesType,
           typename localEdgeIndicesType>
  VISKORES_EXEC viskores::IdComponent operator()(
    const CellSetType& cellSet,
    const OriginCellsType& originCells,
    const OriginEdgesType& originEdges,
    localEdgeIndicesType& localEdgeIndices) const
  {
    viskores::IdComponent numEdgesInHash = localEdgeIndices.GetNumberOfComponents();

    // Sanity checks.
    VISKORES_ASSERT(originCells.GetNumberOfComponents() == numEdgesInHash);
    VISKORES_ASSERT(originEdges.GetNumberOfComponents() == numEdgesInHash);

    // Clear out localEdgeIndices
    for (viskores::IdComponent index = 0; index < numEdgesInHash; ++index)
    {
      localEdgeIndices[index] = -1;
    }

    // Count how many unique edges there are and create an id for each;
    viskores::IdComponent numUniqueEdges = 0;
    for (viskores::IdComponent firstEdgeIndex = 0; firstEdgeIndex < numEdgesInHash;
         ++firstEdgeIndex)
    {
      if (localEdgeIndices[firstEdgeIndex] == -1)
      {
        viskores::IdComponent edgeId = numUniqueEdges;
        localEdgeIndices[firstEdgeIndex] = edgeId;
        // Find all matching edges.
        viskores::Id firstCellIndex = originCells[firstEdgeIndex];
        viskores::Id2 canonicalEdgeId;
        viskores::exec::CellEdgeCanonicalId(cellSet.GetNumberOfIndices(firstCellIndex),
                                            originEdges[firstEdgeIndex],
                                            cellSet.GetCellShape(firstCellIndex),
                                            cellSet.GetIndices(firstCellIndex),
                                            canonicalEdgeId);
        for (viskores::IdComponent laterEdgeIndex = firstEdgeIndex + 1;
             laterEdgeIndex < numEdgesInHash;
             ++laterEdgeIndex)
        {
          viskores::Id laterCellIndex = originCells[laterEdgeIndex];
          viskores::Id2 otherCanonicalEdgeId;
          viskores::exec::CellEdgeCanonicalId(cellSet.GetNumberOfIndices(laterCellIndex),
                                              originEdges[laterEdgeIndex],
                                              cellSet.GetCellShape(laterCellIndex),
                                              cellSet.GetIndices(laterCellIndex),
                                              otherCanonicalEdgeId);
          if (canonicalEdgeId == otherCanonicalEdgeId)
          {
            localEdgeIndices[laterEdgeIndex] = edgeId;
          }
        }
        ++numUniqueEdges;
      }
    }

    return numUniqueEdges;
  }
};
////
//// END-EXAMPLE GenerateMeshHashResolveCollisions
////

////
//// BEGIN-EXAMPLE GenerateMeshHashGenIndices
////
class EdgeIndicesWorklet : public viskores::worklet::WorkletReduceByKey
{
public:
  using ControlSignature = void(KeysIn keys,
                                WholeCellSetIn<> inputCells,
                                ValuesIn originCells,
                                ValuesIn originEdges,
                                ValuesIn localEdgeIndices,
                                ReducedValuesOut connectivityOut);
  using ExecutionSignature = void(_2 inputCells,
                                  _3 originCell,
                                  _4 originEdge,
                                  _5 localEdgeIndices,
                                  VisitIndex localEdgeIndex,
                                  _6 connectivityOut);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterCounting;

  template<typename CellSetType,
           typename OriginCellsType,
           typename OriginEdgesType,
           typename LocalEdgeIndicesType>
  VISKORES_EXEC void operator()(const CellSetType& cellSet,
                                const OriginCellsType& originCells,
                                const OriginEdgesType& originEdges,
                                const LocalEdgeIndicesType& localEdgeIndices,
                                viskores::IdComponent localEdgeIndex,
                                viskores::Id2& connectivityOut) const
  {
    // Find the first edge that matches the index given and return it.
    for (viskores::IdComponent edgeIndex = 0;; ++edgeIndex)
    {
      if (localEdgeIndices[edgeIndex] == localEdgeIndex)
      {
        viskores::Id cellIndex = originCells[edgeIndex];
        viskores::IdComponent numPointsInCell = cellSet.GetNumberOfIndices(cellIndex);
        viskores::IdComponent edgeInCellIndex = originEdges[edgeIndex];
        auto cellShape = cellSet.GetCellShape(cellIndex);

        viskores::IdComponent pointInCellIndex0;
        viskores::exec::CellEdgeLocalIndex(
          numPointsInCell, 0, edgeInCellIndex, cellShape, pointInCellIndex0);
        viskores::IdComponent pointInCellIndex1;
        viskores::exec::CellEdgeLocalIndex(
          numPointsInCell, 1, edgeInCellIndex, cellShape, pointInCellIndex1);

        auto globalPointIndicesForCell = cellSet.GetIndices(cellIndex);
        connectivityOut[0] = globalPointIndicesForCell[pointInCellIndex0];
        connectivityOut[1] = globalPointIndicesForCell[pointInCellIndex1];

        break;
      }
    }
  }
};
////
//// END-EXAMPLE GenerateMeshHashGenIndices
////

////
//// BEGIN-EXAMPLE GenerateMeshHashAverageField
////
class AverageCellEdgesFieldWorklet : public viskores::worklet::WorkletReduceByKey
{
public:
  using ControlSignature = void(KeysIn keys,
                                ValuesIn inFieldValues,
                                ValuesIn localEdgeIndices,
                                ReducedValuesOut averagedField);
  using ExecutionSignature = void(_2 inFieldValues,
                                  _3 localEdgeIndices,
                                  VisitIndex localEdgeIndex,
                                  _4 averagedField);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterCounting;

  template<typename InFieldValuesType,
           typename LocalEdgeIndicesType,
           typename OutFieldValuesType>
  VISKORES_EXEC void operator()(const InFieldValuesType& inFieldValues,
                                const LocalEdgeIndicesType& localEdgeIndices,
                                viskores::IdComponent localEdgeIndex,
                                OutFieldValuesType& averageField) const
  {
    using FieldType = typename InFieldValuesType::ComponentType;

    viskores::IdComponent numValues = 0;
    for (viskores::IdComponent reduceIndex = 0;
         reduceIndex < inFieldValues.GetNumberOfComponents();
         ++reduceIndex)
    {
      if (localEdgeIndices[reduceIndex] == localEdgeIndex)
      {
        FieldType fieldValue = inFieldValues[reduceIndex];
        if (numValues == 0)
        {
          averageField = fieldValue;
        }
        else
        {
          averageField = averageField + fieldValue;
        }
        ++numValues;
      }
    }
    VISKORES_ASSERT(numValues > 0);
    averageField = averageField / numValues;
  }
};

void MapCellEdgesField(
  viskores::cont::DataSet& dataset,
  const viskores::cont::Field& inField,
  const viskores::worklet::ScatterCounting::OutputToInputMapType& cellPermutationMap,
  const viskores::worklet::Keys<viskores::HashType>& cellToEdgeKeys,
  const viskores::cont::ArrayHandle<viskores::IdComponent>& localEdgeIndices,
  const viskores::worklet::ScatterCounting& hashCollisionScatter)
{
  if (inField.IsCellField())
  {
    viskores::cont::Invoker invoke;
    viskores::cont::UnknownArrayHandle inArray = inField.GetData();
    viskores::cont::UnknownArrayHandle outArray = inArray.NewInstanceBasic();

    auto doMap = [&](auto& concreteInput)
    {
      using T = typename std::decay_t<decltype(concreteInput)>::ValueType::ComponentType;
      auto concreteOutput =
        outArray.ExtractArrayFromComponents<T>(viskores::CopyFlag::Off);
      invoke(
        AverageCellEdgesFieldWorklet{},
        hashCollisionScatter,
        cellToEdgeKeys,
        viskores::cont::make_ArrayHandlePermutation(cellPermutationMap, concreteInput),
        localEdgeIndices,
        concreteOutput);
    };
    inArray.CastAndCallWithExtractedArray(doMap);

    dataset.AddCellField(inField.GetName(), outArray);
  }
  else
  {
    dataset.AddField(inField); // pass through
  }
}
////
//// END-EXAMPLE GenerateMeshHashAverageField
////

} // anonymous namespace

namespace viskores
{
namespace filter
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
} // namespace filter
} // namespace viskores

namespace viskores
{
namespace filter
{

//// PAUSE-EXAMPLE
namespace
{

//// RESUME-EXAMPLE
////
//// BEGIN-EXAMPLE GenerateMeshHashInvoke
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
  viskores::worklet::ScatterCounting::OutputToInputMapType outputToInputCellMap =
    scatter.GetOutputToInputMap(inCellSet.GetNumberOfCells());
  viskores::worklet::ScatterCounting::VisitArrayType outputToInputEdgeMap =
    scatter.GetVisitArray(inCellSet.GetNumberOfCells());

  // Third, for each edge, extract a hash.
  viskores::cont::ArrayHandle<viskores::HashType> hashValues;
  this->Invoke(EdgeHashesWorklet{}, scatter, inCellSet, hashValues);

  // Fourth, use a Keys object to combine all like hashes.
  viskores::worklet::Keys<viskores::HashType> cellToEdgeKeys(hashValues);

  // Fifth, use a reduce-by-key to collect like hash values, resolve collisions,
  // and count the number of unique edges associated with each hash.
  viskores::cont::ArrayHandle<viskores::IdComponent> numUniqueEdgesInEachHash;
  viskores::cont::ArrayHandle<viskores::IdComponent> localEdgeIndices;
  this->Invoke(EdgeHashCollisionsWorklet{},
               cellToEdgeKeys,
               inCellSet,
               outputToInputCellMap,
               outputToInputEdgeMap,
               localEdgeIndices,
               numUniqueEdgesInEachHash);

  // Sixth, use a reduce-by-key to extract indices for each unique edge.
  viskores::worklet::ScatterCounting hashCollisionScatter(numUniqueEdgesInEachHash);

  viskores::cont::ArrayHandle<viskores::Id> connectivityArray;
  //// LABEL InvokeEdgeIndices
  this->Invoke(EdgeIndicesWorklet{},
               hashCollisionScatter,
               cellToEdgeKeys,
               inCellSet,
               outputToInputCellMap,
               outputToInputEdgeMap,
               localEdgeIndices,
               viskores::cont::make_ArrayHandleGroupVec<2>(connectivityArray));

  // Seventh, use the created connectivity array to build a cell set.
  viskores::cont::CellSetSingleType<> outCellSet;
  outCellSet.Fill(
    inCellSet.GetNumberOfPoints(), viskores::CELL_SHAPE_LINE, 2, connectivityArray);

  //// LABEL FieldMapperBegin
  auto fieldMapper =
    [&](viskores::cont::DataSet& dataset, const viskores::cont::Field& inField)
  {
    MapCellEdgesField(dataset,
                      inField,
                      outputToInputCellMap,
                      cellToEdgeKeys,
                      localEdgeIndices,
                      hashCollisionScatter);
    //// LABEL FieldMapperEnd
  };
  return this->CreateResult(inData, outCellSet, fieldMapper);
}
////
//// END-EXAMPLE GenerateMeshHashInvoke
////

//// PAUSE-EXAMPLE
} // anonymous namespace

//// RESUME-EXAMPLE
} // namespace filter
} // namespace viskores

namespace
{

template<typename ConnectivityPortalType>
viskores::Id FindEdge(const ConnectivityPortalType& connectivity,
                      const viskores::Id2& edge)
{
  viskores::Id edgeIndex = 0;
  bool foundEdge = false;

  for (viskores::Id connectivityIndex = 0;
       connectivityIndex < connectivity.GetNumberOfValues() - 1;
       connectivityIndex += 2)
  {
    viskores::Id p0 = connectivity.Get(connectivityIndex + 0);
    viskores::Id p1 = connectivity.Get(connectivityIndex + 1);
    if (((edge[0] == p0) && (edge[1] == p1)) || ((edge[0] == p1) && (edge[1] == p0)))
    {
      foundEdge = true;
      break;
    }
    edgeIndex++;
  }

  VISKORES_TEST_ASSERT(foundEdge, "Did not find expected edge.");

  for (viskores::Id connectivityIndex = 2 * (edgeIndex + 1);
       connectivityIndex < connectivity.GetNumberOfValues() - 1;
       connectivityIndex += 2)
  {
    viskores::Id p0 = connectivity.Get(connectivityIndex + 0);
    viskores::Id p1 = connectivity.Get(connectivityIndex + 1);
    if (((edge[0] == p0) && (edge[1] == p1)) || ((edge[0] == p1) && (edge[1] == p0)))
    {
      VISKORES_TEST_FAIL("Edge duplicated.");
    }
  }

  return edgeIndex;
}

template<typename ConnectivityPortalType, typename FieldPortalType>
void CheckEdge(const ConnectivityPortalType& connectivity,
               const FieldPortalType& field,
               const viskores::Id2& edge,
               const viskores::Float32 expectedFieldValue)
{
  std::cout << "  Checking for edge " << edge << " with field value "
            << expectedFieldValue << std::endl;

  viskores::Id edgeIndex = FindEdge(connectivity, edge);

  viskores::Float32 fieldValue = field.Get(edgeIndex);
  VISKORES_TEST_ASSERT(test_equal(expectedFieldValue, fieldValue), "Bad field value.");
}

void CheckOutput(const viskores::cont::CellSetSingleType<>& cellSet,
                 const viskores::cont::ArrayHandle<viskores::Float32>& cellField)
{
  std::cout << "Num cells: " << cellSet.GetNumberOfCells() << std::endl;
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == 22, "Wrong # of cells.");

  auto connectivity = cellSet.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                   viskores::TopologyElementTagPoint());
  std::cout << "Connectivity:" << std::endl;
  viskores::cont::printSummary_ArrayHandle(connectivity, std::cout, true);

  std::cout << "Cell field:" << std::endl;
  viskores::cont::printSummary_ArrayHandle(cellField, std::cout, true);

  auto connectivityPortal = connectivity.ReadPortal();
  auto fieldPortal = cellField.ReadPortal();
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(0, 1), 100.1f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(0, 3), 100.1f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(0, 4), 100.1f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(7, 4), 115.3f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(7, 6), 115.3f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(7, 3), 100.1f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(5, 1), 105.05f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(5, 4), 115.3f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(5, 6), 115.2f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(2, 1), 105.05f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(2, 3), 100.1f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(2, 6), 105.05f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(8, 1), 110.0f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(8, 2), 110.0f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(8, 5), 115.1f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(8, 6), 115.1f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(10, 5), 125.35f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(10, 6), 125.35f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(10, 8), 120.2f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(9, 4), 130.5f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(9, 7), 130.5f);
  CheckEdge(connectivityPortal, fieldPortal, viskores::Id2(9, 10), 130.5f);
}

void TryFilter()
{
  std::cout << std::endl << "Trying calling filter." << std::endl;
  viskores::cont::DataSet inDataSet =
    viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

  viskores::filter::ExtractEdges filter;

  viskores::cont::DataSet outDataSet = filter.Execute(inDataSet);

  viskores::cont::CellSetSingleType<> outCellSet;
  outDataSet.GetCellSet().AsCellSet(outCellSet);

  viskores::cont::Field outCellField = outDataSet.GetField("cellvar");
  viskores::cont::ArrayHandle<viskores::Float32> outCellData;
  outCellField.GetData().AsArrayHandle(outCellData);

  CheckOutput(outCellSet, outCellData);
}

void DoTest()
{
  TryFilter();
}

} // anonymous namespace

int GuideExampleGenerateMeshHash(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
