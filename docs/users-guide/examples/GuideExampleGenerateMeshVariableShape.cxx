//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/ArrayHandleCast.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/CellSetExplicit.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>

#include <viskores/exec/CellFace.h>

#include <viskores/Hash.h>

#include <viskores/worklet/AverageByKey.h>
#include <viskores/worklet/Keys.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/filter/Filter.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

//#define CHECK_COLLISIONS

namespace
{

struct CountFacesWorklet : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOut numFaces);
  using ExecutionSignature = _2(CellShape);
  using InputDomain = _1;

  template<typename CellShapeTag>
  VISKORES_EXEC_CONT viskores::IdComponent operator()(CellShapeTag cellShape) const
  {
    viskores::IdComponent numFaces;
    viskores::ErrorCode status =
      viskores::exec::CellFaceNumberOfFaces(cellShape, numFaces);
    if (status != viskores::ErrorCode::Success)
    {
      // There is an error in the cell. As good as it would be to return an
      // error, we probably don't want to invalidate the entire run if there
      // is just one malformed cell. Instead, ignore the cell.
      return 0;
    }
    return numFaces;
  }
};

class FaceHashesWorklet : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellSet, FieldOut hashValues);
  using ExecutionSignature = _2(CellShape cellShape,
                                PointIndices globalPointIndices,
                                VisitIndex localFaceIndex);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterCounting;

  template<typename CellShapeTag, typename PointIndexVecType>
  VISKORES_EXEC viskores::HashType operator()(
    CellShapeTag cellShape,
    const PointIndexVecType& globalPointIndicesForCell,
    viskores::IdComponent localFaceIndex) const
  {
    viskores::Id3 canonicalId;
    viskores::ErrorCode status = viskores::exec::CellFaceCanonicalId(
      localFaceIndex, cellShape, globalPointIndicesForCell, canonicalId);
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
#else  // ! CHECK_COLLISIONS                                                        \
       // Intentionally use a bad hash value to cause collisions to check to make   \
       // sure that collision resolution works.
    return viskores::HashType(canonicalId[0]);
#endif // !CHECK_COLLISIONS
    //// RESUME-EXAMPLE
  }
};

class FaceHashCollisionsWorklet : public viskores::worklet::WorkletReduceByKey
{
public:
  using ControlSignature = void(KeysIn keys,
                                WholeCellSetIn<> inputCells,
                                ValuesIn originCells,
                                ValuesIn originFaces,
                                ValuesOut localFaceIndices,
                                ReducedValuesOut numFaces);
  using ExecutionSignature = _6(_2 inputCells,
                                _3 originCells,
                                _4 originFaces,
                                _5 localFaceIndices);
  using InputDomain = _1;

  template<typename CellSetType,
           typename OriginCellsType,
           typename OriginFacesType,
           typename localFaceIndicesType>
  VISKORES_EXEC viskores::IdComponent operator()(
    const CellSetType& cellSet,
    const OriginCellsType& originCells,
    const OriginFacesType& originFaces,
    localFaceIndicesType& localFaceIndices) const
  {
    viskores::IdComponent numFacesInHash = localFaceIndices.GetNumberOfComponents();

    // Sanity checks.
    VISKORES_ASSERT(originCells.GetNumberOfComponents() == numFacesInHash);
    VISKORES_ASSERT(originFaces.GetNumberOfComponents() == numFacesInHash);

    // Clear out localFaceIndices
    for (viskores::IdComponent index = 0; index < numFacesInHash; ++index)
    {
      localFaceIndices[index] = -1;
    }

    // Count how many unique faces there are and create an id for each;
    viskores::IdComponent numUniqueFaces = 0;
    for (viskores::IdComponent firstFaceIndex = 0; firstFaceIndex < numFacesInHash;
         ++firstFaceIndex)
    {
      if (localFaceIndices[firstFaceIndex] == -1)
      {
        viskores::IdComponent faceId = numUniqueFaces;
        localFaceIndices[firstFaceIndex] = faceId;
        // Find all matching faces.
        viskores::Id firstCellIndex = originCells[firstFaceIndex];
        viskores::Id3 canonicalFaceId;
        viskores::exec::CellFaceCanonicalId(originFaces[firstFaceIndex],
                                            cellSet.GetCellShape(firstCellIndex),
                                            cellSet.GetIndices(firstCellIndex),
                                            canonicalFaceId);
        for (viskores::IdComponent laterFaceIndex = firstFaceIndex + 1;
             laterFaceIndex < numFacesInHash;
             ++laterFaceIndex)
        {
          viskores::Id laterCellIndex = originCells[laterFaceIndex];
          viskores::Id3 otherCanonicalFaceId;
          viskores::exec::CellFaceCanonicalId(originFaces[laterFaceIndex],
                                              cellSet.GetCellShape(laterCellIndex),
                                              cellSet.GetIndices(laterCellIndex),
                                              otherCanonicalFaceId);
          if (canonicalFaceId == otherCanonicalFaceId)
          {
            localFaceIndices[laterFaceIndex] = faceId;
          }
        }
        ++numUniqueFaces;
      }
    }

    return numUniqueFaces;
  }
};

////
//// BEGIN-EXAMPLE GenerateMeshVariableShapeCountPointsInFace
////
class CountPointsInFaceWorklet : public viskores::worklet::WorkletReduceByKey
{
public:
  using ControlSignature = void(KeysIn keys,
                                WholeCellSetIn<> inputCells,
                                ValuesIn originCells,
                                ValuesIn originFaces,
                                ValuesIn localFaceIndices,
                                ReducedValuesOut faceShape,
                                ReducedValuesOut numPointsInEachFace);
  using ExecutionSignature = void(_2 inputCells,
                                  _3 originCell,
                                  _4 originFace,
                                  _5 localFaceIndices,
                                  VisitIndex localFaceIndex,
                                  _6 faceShape,
                                  _7 numPointsInFace);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterCounting;

  template<typename CellSetType,
           typename OriginCellsType,
           typename OriginFacesType,
           typename LocalFaceIndicesType>
  VISKORES_EXEC void operator()(const CellSetType& cellSet,
                                const OriginCellsType& originCells,
                                const OriginFacesType& originFaces,
                                const LocalFaceIndicesType& localFaceIndices,
                                viskores::IdComponent localFaceIndex,
                                viskores::UInt8& faceShape,
                                viskores::IdComponent& numPointsInFace) const
  {
    // Find the first face that matches the index given.
    for (viskores::IdComponent faceIndex = 0;; ++faceIndex)
    {
      if (localFaceIndices[faceIndex] == localFaceIndex)
      {
        viskores::Id cellIndex = originCells[faceIndex];
        viskores::exec::CellFaceShape(
          originFaces[faceIndex], cellSet.GetCellShape(cellIndex), faceShape);
        viskores::exec::CellFaceNumberOfPoints(
          originFaces[faceIndex], cellSet.GetCellShape(cellIndex), numPointsInFace);
        break;
      }
    }
  }
};
////
//// END-EXAMPLE GenerateMeshVariableShapeCountPointsInFace
////

////
//// BEGIN-EXAMPLE GenerateMeshVariableShapeGenIndices
////
class FaceIndicesWorklet : public viskores::worklet::WorkletReduceByKey
{
public:
  using ControlSignature = void(KeysIn keys,
                                WholeCellSetIn<> inputCells,
                                ValuesIn originCells,
                                ValuesIn originFaces,
                                ValuesIn localFaceIndices,
                                ReducedValuesOut connectivityOut);
  using ExecutionSignature = void(_2 inputCells,
                                  _3 originCell,
                                  _4 originFace,
                                  _5 localFaceIndices,
                                  VisitIndex localFaceIndex,
                                  _6 connectivityOut);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterCounting;

  template<typename CellSetType,
           typename OriginCellsType,
           typename OriginFacesType,
           typename LocalFaceIndicesType,
           typename ConnectivityVecType>
  VISKORES_EXEC void operator()(const CellSetType& cellSet,
                                const OriginCellsType& originCells,
                                const OriginFacesType& originFaces,
                                const LocalFaceIndicesType& localFaceIndices,
                                viskores::IdComponent localFaceIndex,
                                ConnectivityVecType& connectivityOut) const
  {
    // Find the first face that matches the index given and return it.
    for (viskores::IdComponent faceIndex = 0;; ++faceIndex)
    {
      if (localFaceIndices[faceIndex] == localFaceIndex)
      {
        viskores::Id cellIndex = originCells[faceIndex];
        viskores::IdComponent faceInCellIndex = originFaces[faceIndex];
        auto cellShape = cellSet.GetCellShape(cellIndex);
        viskores::IdComponent numPointsInFace = connectivityOut.GetNumberOfComponents();

        auto globalPointIndicesForCell = cellSet.GetIndices(cellIndex);
        for (viskores::IdComponent localPointI = 0; localPointI < numPointsInFace;
             ++localPointI)
        {
          viskores::IdComponent pointInCellIndex;
          viskores::exec::CellFaceLocalIndex(
            localPointI, faceInCellIndex, cellShape, pointInCellIndex);
          connectivityOut[localPointI] = globalPointIndicesForCell[pointInCellIndex];
        }

        break;
      }
    }
  }
};
////
//// END-EXAMPLE GenerateMeshVariableShapeGenIndices
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
  const viskores::worklet::Keys<viskores::HashType>& cellToFaceKeys,
  const viskores::cont::ArrayHandle<viskores::IdComponent>& localFaceIndices,
  const viskores::worklet::ScatterCounting& hashCollisionScatter)
{
  if (inField.IsCellField())
  {
    viskores::cont::Invoker invoke;
    viskores::cont::UnknownArrayHandle inArray = inField.GetData();
    viskores::cont::UnknownArrayHandle outArray = inArray.NewInstanceBasic();

    // Need to pre-allocate outArray because the way it is accessed in
    // doMap it cannot be resized.
    outArray.Allocate(hashCollisionScatter.GetOutputRange(
      cellToFaceKeys.GetUniqueKeys().GetNumberOfValues()));

    auto doMap = [&](auto& concreteInput)
    {
      using T = typename std::decay_t<decltype(concreteInput)>::ValueType::ComponentType;
      auto concreteOutput =
        outArray.ExtractArrayFromComponents<T>(viskores::CopyFlag::Off);
      invoke(
        AverageCellEdgesFieldWorklet{},
        hashCollisionScatter,
        cellToFaceKeys,
        viskores::cont::make_ArrayHandlePermutation(cellPermutationMap, concreteInput),
        localFaceIndices,
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

} // anonymous namespace

namespace viskores
{
namespace filter
{

//// PAUSE-EXAMPLE
namespace
{

//// RESUME-EXAMPLE
class ExtractFaces : public viskores::filter::Filter
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
//// BEGIN-EXAMPLE GenerateMeshVariableShapeInvoke
////
inline VISKORES_CONT viskores::cont::DataSet ExtractFaces::DoExecute(
  const viskores::cont::DataSet& inData)
{

  auto inCellSet = inData.GetCellSet();

  // First, count the faces in each cell.
  viskores::cont::ArrayHandle<viskores::IdComponent> faceCounts;
  this->Invoke(CountFacesWorklet{}, inCellSet, faceCounts);

  // Second, using these counts build a scatter that repeats a cell's visit
  // for each edge in the cell.
  viskores::worklet::ScatterCounting scatter(faceCounts);
  viskores::worklet::ScatterCounting::OutputToInputMapType outputToInputCellMap =
    scatter.GetOutputToInputMap(inCellSet.GetNumberOfCells());
  viskores::worklet::ScatterCounting::VisitArrayType outputToInputFaceMap =
    scatter.GetVisitArray(inCellSet.GetNumberOfCells());

  // Third, for each face, extract a hash.
  viskores::cont::ArrayHandle<viskores::HashType> hashValues;
  this->Invoke(FaceHashesWorklet{}, scatter, inCellSet, hashValues);

  // Fourth, use a Keys object to combine all like hashes.
  viskores::worklet::Keys<viskores::HashType> cellToFaceKeys(hashValues);

  // Fifth, use a reduce-by-key to collect like hash values, resolve collisions,
  // and count the number of unique faces associated with each hash.
  viskores::cont::ArrayHandle<viskores::IdComponent> localFaceIndices;
  viskores::cont::ArrayHandle<viskores::IdComponent> numUniqueFacesInEachHash;
  this->Invoke(FaceHashCollisionsWorklet{},
               cellToFaceKeys,
               inCellSet,
               outputToInputCellMap,
               outputToInputFaceMap,
               localFaceIndices,
               numUniqueFacesInEachHash);

  // Sixth, use a reduce-by-key to count the number of points in each unique face.
  // Also identify the shape of each face.
  viskores::worklet::ScatterCounting hashCollisionScatter(numUniqueFacesInEachHash);

  viskores::cont::CellSetExplicit<>::ShapesArrayType shapeArray;
  viskores::cont::ArrayHandle<viskores::IdComponent> numPointsInEachFace;

  this->Invoke(CountPointsInFaceWorklet{},
               hashCollisionScatter,
               cellToFaceKeys,
               inCellSet,
               outputToInputCellMap,
               outputToInputFaceMap,
               localFaceIndices,
               shapeArray,
               numPointsInEachFace);

  // Seventh, convert the numPointsInEachFace array to an offsets array and use that
  // to create an ArrayHandleGroupVecVariable.
  ////
  //// BEGIN-EXAMPLE GenerateMeshVariableShapeOffsetsArray
  ////
  viskores::cont::ArrayHandle<viskores::Id> offsets;
  viskores::Id connectivityArraySize;
  viskores::cont::ConvertNumComponentsToOffsets(
    numPointsInEachFace, offsets, connectivityArraySize);

  viskores::cont::CellSetExplicit<>::ConnectivityArrayType connectivityArray;
  connectivityArray.Allocate(connectivityArraySize);
  auto connectivityArrayVecs =
    viskores::cont::make_ArrayHandleGroupVecVariable(connectivityArray, offsets);
  ////
  //// END-EXAMPLE GenerateMeshVariableShapeOffsetsArray
  ////

  // Eigth, use a reduce-by-key to extract indices for each unique face.
  this->Invoke(FaceIndicesWorklet{},
               hashCollisionScatter,
               cellToFaceKeys,
               inCellSet,
               outputToInputCellMap,
               outputToInputFaceMap,
               localFaceIndices,
               connectivityArrayVecs);

  // Ninth, use the created connectivity array and others to build a cell set.
  viskores::cont::CellSetExplicit<> outCellSet;
  outCellSet.Fill(inCellSet.GetNumberOfPoints(), shapeArray, connectivityArray, offsets);

  auto fieldMapper =
    [&](viskores::cont::DataSet& dataset, const viskores::cont::Field& inField)
  {
    MapCellEdgesField(dataset,
                      inField,
                      outputToInputCellMap,
                      cellToFaceKeys,
                      localFaceIndices,
                      hashCollisionScatter);
  };
  return this->CreateResult(inData, outCellSet, fieldMapper);
}
////
//// END-EXAMPLE GenerateMeshVariableShapeInvoke
////

//// PAUSE-EXAMPLE
} // anonymous namespace

//// RESUME-EXAMPLE
} // namespace filter
} // namespace viskores

namespace
{

template<typename FaceVecLikeType>
bool IsFace(const FaceVecLikeType& faceIndices, const viskores::Id3& expectedIndices)
{
  for (viskores::IdComponent expectedIndex = 0; expectedIndex < 3; ++expectedIndex)
  {
    bool foundIndex = false;
    for (viskores::IdComponent faceIndex = 0;
         faceIndex < faceIndices.GetNumberOfComponents();
         ++faceIndex)
    {
      if (expectedIndices[expectedIndex] == faceIndices[faceIndex])
      {
        foundIndex = true;
        break;
      }
    }
    if (!foundIndex)
    {
      return false;
    }
  }
  return true;
}

template<typename NumPointsInFaceType,
         typename ConnectivityPortalType,
         typename OffsetsPortalType>
viskores::Id FindFace(const NumPointsInFaceType& numPointsInFace,
                      const ConnectivityPortalType& connectivity,
                      const OffsetsPortalType& offsets,
                      const viskores::Id3& face)
{
  viskores::Id faceIndex = -1;

  for (viskores::Id offsetIndex = 0; offsetIndex < offsets.GetNumberOfValues();
       ++offsetIndex)
  {
    if (IsFace(
          viskores::VecFromPortal<ConnectivityPortalType>(
            connectivity, numPointsInFace.Get(offsetIndex), offsets.Get(offsetIndex)),
          face))
    {
      faceIndex = offsetIndex;
      break;
    }
  }

  VISKORES_TEST_ASSERT(faceIndex >= 0, "Did not find expected face.");

  for (viskores::Id offsetIndex = faceIndex + 1;
       offsetIndex < offsets.GetNumberOfValues();
       ++offsetIndex)
  {
    if (IsFace(
          viskores::VecFromPortal<ConnectivityPortalType>(
            connectivity, numPointsInFace.Get(offsetIndex), offsets.Get(offsetIndex)),
          face))
    {
      VISKORES_TEST_FAIL("Face duplicated.");
    }
  }

  return faceIndex;
}

template<typename NumPointsInFaceType,
         typename ConnectivityPortalType,
         typename OffsetsPortalType,
         typename FieldPortalType>
void CheckFace(const NumPointsInFaceType& numPointsInFace,
               const ConnectivityPortalType& connectivity,
               const OffsetsPortalType& offsets,
               const FieldPortalType& field,
               const viskores::Id3& face,
               const viskores::Float32 expectedFieldValue)
{
  std::cout << "  Checking for face " << face << " with field value "
            << expectedFieldValue << std::endl;

  viskores::Id faceIndex = FindFace(numPointsInFace, connectivity, offsets, face);

  viskores::Float32 fieldValue = field.Get(faceIndex);
  VISKORES_TEST_ASSERT(test_equal(expectedFieldValue, fieldValue), "Bad field value.");
}

void CheckOutput(const viskores::cont::CellSetExplicit<>& cellSet,
                 const viskores::cont::ArrayHandle<viskores::Float32>& cellField)
{
  std::cout << "Num cells: " << cellSet.GetNumberOfCells() << std::endl;
  VISKORES_TEST_ASSERT(cellSet.GetNumberOfCells() == 16, "Wrong # of cells.");

  auto numPointsPerFace = cellSet.GetNumIndicesArray(
    viskores::TopologyElementTagCell(), viskores::TopologyElementTagPoint());
  auto connectivity = cellSet.GetConnectivityArray(viskores::TopologyElementTagCell(),
                                                   viskores::TopologyElementTagPoint());
  auto offsetsExtended = cellSet.GetOffsetsArray(viskores::TopologyElementTagCell(),
                                                 viskores::TopologyElementTagPoint());
  auto offsetsExclusive = viskores::cont::make_ArrayHandleView(
    offsetsExtended, 0, offsetsExtended.GetNumberOfValues() - 1);

  std::cout << "Offsets:" << std::endl;
  viskores::cont::printSummary_ArrayHandle(offsetsExclusive, std::cout, true);
  std::cout << "Connectivity:" << std::endl;
  viskores::cont::printSummary_ArrayHandle(connectivity, std::cout, true);

  std::cout << "Cell field:" << std::endl;
  viskores::cont::printSummary_ArrayHandle(cellField, std::cout, true);

  auto nPointsP = numPointsPerFace.ReadPortal();
  auto connectivityP = connectivity.ReadPortal();
  auto offsetsP = offsetsExclusive.ReadPortal();
  auto fieldP = cellField.ReadPortal();
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(0, 1, 2), 100.1f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(0, 1, 5), 100.1f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(0, 3, 7), 100.1f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(6, 2, 3), 100.1f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(6, 2, 1), 105.05f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(6, 5, 4), 115.3f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(8, 1, 2), 110.0f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(8, 1, 5), 110.0f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(8, 6, 2), 110.0f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(8, 6, 5), 115.1f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(10, 6, 8), 120.2f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(10, 5, 8), 120.2f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(10, 6, 5), 125.35f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(9, 4, 7), 130.5f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(9, 4, 5), 130.5f);
  CheckFace(nPointsP, connectivityP, offsetsP, fieldP, viskores::Id3(9, 4, 10), 130.5f);
}

// void TryWorklet()
//{
//  std::cout << std::endl << "Trying calling worklet." << std::endl;
//  viskores::cont::DataSet inDataSet =
//      viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();
//  viskores::cont::CellSetExplicit<> inCellSet;
//  inDataSet.GetCellSet().CopyTo(inCellSet);

//  viskores::worklet::ExtractFaces worklet;
//  viskores::cont::CellSetExplicit<> outCellSet = worklet.Run(inCellSet);
//  CheckOutput(outCellSet);
//}

void TryFilter()
{
  std::cout << std::endl << "Trying calling filter." << std::endl;
  viskores::cont::DataSet inDataSet =
    viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

  viskores::filter::ExtractFaces filter;

  viskores::cont::DataSet outDataSet = filter.Execute(inDataSet);

  viskores::cont::CellSetExplicit<> outCellSet;
  outDataSet.GetCellSet().AsCellSet(outCellSet);

  viskores::cont::Field outCellField = outDataSet.GetField("cellvar");
  viskores::cont::ArrayHandle<viskores::Float32> outCellData;
  outCellField.GetData().AsArrayHandle(outCellData);

  CheckOutput(outCellSet, outCellData);
}

void DoTest()
{
  //  TryWorklet();
  TryFilter();
}

} // anonymous namespace

int GuideExampleGenerateMeshVariableShape(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTest, argc, argv);
}
