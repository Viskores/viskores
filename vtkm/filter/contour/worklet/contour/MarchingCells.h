//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_contour_MarchingCells_h
#define vtk_m_worklet_contour_MarchingCells_h

#include <vtkm/BinaryPredicates.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/ScatterPermutation.h>

#include <vtkm/filter/contour/worklet/contour/CommonState.h>
#include <vtkm/filter/contour/worklet/contour/MarchingCellTables.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/PointGradient.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/StructuredPointGradient.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

namespace vtkm
{
namespace worklet
{
namespace marching_cells
{

// -----------------------------------------------------------------------------
template <typename S>
vtkm::cont::ArrayHandle<vtkm::Float32, S> make_ScalarField(
  const vtkm::cont::ArrayHandle<vtkm::Float32, S>& ah)
{
  return ah;
}

template <typename S>
vtkm::cont::ArrayHandle<vtkm::Float64, S> make_ScalarField(
  const vtkm::cont::ArrayHandle<vtkm::Float64, S>& ah)
{
  return ah;
}

template <typename S>
vtkm::cont::ArrayHandleCast<vtkm::FloatDefault, vtkm::cont::ArrayHandle<vtkm::UInt8, S>>
make_ScalarField(const vtkm::cont::ArrayHandle<vtkm::UInt8, S>& ah)
{
  return vtkm::cont::make_ArrayHandleCast(ah, vtkm::FloatDefault());
}

template <typename S>
vtkm::cont::ArrayHandleCast<vtkm::FloatDefault, vtkm::cont::ArrayHandle<vtkm::Int8, S>>
make_ScalarField(const vtkm::cont::ArrayHandle<vtkm::Int8, S>& ah)
{
  return vtkm::cont::make_ArrayHandleCast(ah, vtkm::FloatDefault());
}

// ---------------------------------------------------------------------------
template <typename T>
class ClassifyCell : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(WholeArrayIn isoValues,
                                FieldInPoint fieldIn,
                                CellSetIn cellSet,
                                FieldOutCell outNumTriangles,
                                ExecObject classifyTable);
  using ExecutionSignature = void(CellShape, _1, _2, _4, _5);
  using InputDomain = _3;

  template <typename CellShapeType,
            typename IsoValuesType,
            typename FieldInType,
            typename ClassifyTableType>
  VTKM_EXEC void operator()(CellShapeType shape,
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn,
                            vtkm::IdComponent& numTriangles,
                            const ClassifyTableType& classifyTable) const
  {
    vtkm::IdComponent sum = 0;
    vtkm::IdComponent numIsoValues = static_cast<vtkm::IdComponent>(isovalues.GetNumberOfValues());
    vtkm::IdComponent numVerticesPerCell = classifyTable.GetNumVerticesPerCell(shape.Id);

    for (vtkm::Id i = 0; i < numIsoValues; ++i)
    {
      vtkm::IdComponent caseNumber = 0;
      for (vtkm::IdComponent j = 0; j < numVerticesPerCell; ++j)
      {
        caseNumber |= (fieldIn[j] > isovalues[i]) << j;
      }

      sum += classifyTable.GetNumTriangles(shape.Id, caseNumber);
    }
    numTriangles = sum;
  }
};

/// \brief Used to store data need for the EdgeWeightGenerate worklet.
/// This information is not passed as part of the arguments to the worklet as
/// that dramatically increase compile time by 200%
// TODO: remove unused data members.
// -----------------------------------------------------------------------------
class EdgeWeightGenerateMetaData : vtkm::cont::ExecutionObjectBase
{
public:
  class ExecObject
  {
    template <typename FieldType>
    using ReadPortalType = typename vtkm::cont::ArrayHandle<FieldType>::ReadPortalType;
    template <typename FieldType>
    using WritePortalType = typename vtkm::cont::ArrayHandle<FieldType>::WritePortalType;

  public:
    ExecObject() = default;

    VTKM_CONT
    ExecObject(vtkm::Id size,
               vtkm::cont::ArrayHandle<vtkm::FloatDefault>& interpWeights,
               vtkm::cont::ArrayHandle<vtkm::Id2>& interpIds,
               vtkm::cont::ArrayHandle<vtkm::Id>& interpCellIds,
               vtkm::cont::ArrayHandle<vtkm::UInt8>& interpContourId,
               vtkm::cont::DeviceAdapterId device,
               vtkm::cont::Token& token)
      : InterpWeightsPortal(interpWeights.PrepareForOutput(3 * size, device, token))
      , InterpIdPortal(interpIds.PrepareForOutput(3 * size, device, token))
      , InterpCellIdPortal(interpCellIds.PrepareForOutput(3 * size, device, token))
      , InterpContourPortal(interpContourId.PrepareForOutput(3 * size, device, token))
    {
      // Interp needs to be 3 times longer than size as they are per point of the
      // output triangle
    }
    WritePortalType<vtkm::FloatDefault> InterpWeightsPortal;
    WritePortalType<vtkm::Id2> InterpIdPortal;
    WritePortalType<vtkm::Id> InterpCellIdPortal;
    WritePortalType<vtkm::UInt8> InterpContourPortal;
  };

  VTKM_CONT
  EdgeWeightGenerateMetaData(vtkm::Id size,
                             vtkm::cont::ArrayHandle<vtkm::FloatDefault>& interpWeights,
                             vtkm::cont::ArrayHandle<vtkm::Id2>& interpIds,
                             vtkm::cont::ArrayHandle<vtkm::Id>& interpCellIds,
                             vtkm::cont::ArrayHandle<vtkm::UInt8>& interpContourId)
    : Size(size)
    , InterpWeights(interpWeights)
    , InterpIds(interpIds)
    , InterpCellIds(interpCellIds)
    , InterpContourId(interpContourId)
  {
  }

  VTKM_CONT ExecObject PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                           vtkm::cont::Token& token)
  {
    return ExecObject(this->Size,
                      this->InterpWeights,
                      this->InterpIds,
                      this->InterpCellIds,
                      this->InterpContourId,
                      device,
                      token);
  }

private:
  vtkm::Id Size;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> InterpWeights;
  vtkm::cont::ArrayHandle<vtkm::Id2> InterpIds;
  vtkm::cont::ArrayHandle<vtkm::Id> InterpCellIds;
  vtkm::cont::ArrayHandle<vtkm::UInt8> InterpContourId;
};

/// \brief Compute the weights for each edge that is used to generate
/// a point in the resulting iso-surface
// -----------------------------------------------------------------------------
template <typename T>
class EdgeWeightGenerate : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ScatterType = vtkm::worklet::ScatterCounting;

  template <typename ArrayHandleType>
  VTKM_CONT static ScatterType MakeScatter(const ArrayHandleType& numOutputTrisPerCell)
  {
    return ScatterType(numOutputTrisPerCell);
  }

  typedef void ControlSignature(CellSetIn cellset, // Cell set
                                WholeArrayIn isoValues,
                                FieldInPoint fieldIn, // Input point field defining the contour
                                ExecObject metaData,  // Metadata for edge weight generation
                                ExecObject classifyTable,
                                ExecObject triTable);
  using ExecutionSignature =
    void(CellShape, _2, _3, _4, _5, _6, InputIndex, WorkIndex, VisitIndex, PointIndices);

  using InputDomain = _1;

  template <typename CellShape,
            typename IsoValuesType,
            typename FieldInType, // Vec-like, one per input point
            typename ClassifyTableType,
            typename TriTableType,
            typename IndicesVecType>
  VTKM_EXEC void operator()(const CellShape shape,
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn, // Input point field defining the contour
                            const EdgeWeightGenerateMetaData::ExecObject& metaData,
                            const ClassifyTableType& classifyTable,
                            const TriTableType& triTable,
                            vtkm::Id inputCellId,
                            vtkm::Id outputCellId,
                            vtkm::IdComponent visitIndex,
                            const IndicesVecType& indices) const
  {
    const vtkm::Id outputPointId = 3 * outputCellId;
    using FieldType = typename vtkm::VecTraits<FieldInType>::ComponentType;

    vtkm::IdComponent sum = 0, caseNumber = 0;
    vtkm::IdComponent i = 0,
                      numIsoValues = static_cast<vtkm::IdComponent>(isovalues.GetNumberOfValues());
    vtkm::IdComponent numVerticesPerCell = classifyTable.GetNumVerticesPerCell(shape.Id);

    for (i = 0; i < numIsoValues; ++i)
    {
      const FieldType ivalue = isovalues[i];
      // Compute the Marching Cubes case number for this cell. We need to iterate
      // the isovalues until the sum >= our visit index. But we need to make
      // sure the caseNumber is correct before stopping
      caseNumber = 0;
      for (vtkm::IdComponent j = 0; j < numVerticesPerCell; ++j)
      {
        caseNumber |= (fieldIn[j] > ivalue) << j;
      }

      sum += classifyTable.GetNumTriangles(shape.Id, caseNumber);
      if (sum > visitIndex)
      {
        break;
      }
    }

    visitIndex = sum - visitIndex - 1;

    // Interpolate for vertex positions and associated scalar values
    for (vtkm::IdComponent triVertex = 0; triVertex < 3; triVertex++)
    {
      auto edgeVertices = triTable.GetEdgeVertices(shape.Id, caseNumber, visitIndex, triVertex);
      const FieldType fieldValue0 = fieldIn[edgeVertices.first];
      const FieldType fieldValue1 = fieldIn[edgeVertices.second];

      // Store the input cell id so that we can properly generate the normals
      // in a subsequent call, after we have merged duplicate points
      metaData.InterpCellIdPortal.Set(outputPointId + triVertex, inputCellId);

      metaData.InterpContourPortal.Set(outputPointId + triVertex, static_cast<vtkm::UInt8>(i));

      metaData.InterpIdPortal.Set(
        outputPointId + triVertex,
        vtkm::Id2(indices[edgeVertices.first], indices[edgeVertices.second]));

      vtkm::FloatDefault interpolant = static_cast<vtkm::FloatDefault>(isovalues[i] - fieldValue0) /
        static_cast<vtkm::FloatDefault>(fieldValue1 - fieldValue0);

      metaData.InterpWeightsPortal.Set(outputPointId + triVertex, interpolant);
    }
  }
};

// ---------------------------------------------------------------------------
struct MultiContourLess
{
  template <typename T>
  VTKM_EXEC_CONT bool operator()(const T& a, const T& b) const
  {
    return a < b;
  }

  template <typename T, typename U>
  VTKM_EXEC_CONT bool operator()(const vtkm::Pair<T, U>& a, const vtkm::Pair<T, U>& b) const
  {
    return (a.first < b.first) || (!(b.first < a.first) && (a.second < b.second));
  }

  template <typename T, typename U>
  VTKM_EXEC_CONT bool operator()(const vtkm::internal::ArrayPortalValueReference<T>& a,
                                 const U& b) const
  {
    U&& t = static_cast<U>(a);
    return t < b;
  }
};

// ---------------------------------------------------------------------------
struct MergeDuplicateValues : vtkm::worklet::WorkletReduceByKey
{
  using ControlSignature = void(KeysIn keys,
                                ValuesIn valuesIn1,
                                ValuesIn valuesIn2,
                                ReducedValuesOut valueOut1,
                                ReducedValuesOut valueOut2);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  template <typename T,
            typename ValuesInType,
            typename Values2InType,
            typename ValuesOutType,
            typename Values2OutType>
  VTKM_EXEC void operator()(const T&,
                            const ValuesInType& values1,
                            const Values2InType& values2,
                            ValuesOutType& valueOut1,
                            Values2OutType& valueOut2) const
  {
    valueOut1 = values1[0];
    valueOut2 = values2[0];
  }
};

// ---------------------------------------------------------------------------
struct CopyEdgeIds : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  VTKM_EXEC
  void operator()(const vtkm::Id2& input, vtkm::Id2& output) const { output = input; }

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Pair<T, vtkm::Id2>& input, vtkm::Id2& output) const
  {
    output = input.second;
  }
};

// ---------------------------------------------------------------------------
template <typename KeyType, typename KeyStorage>
void MergeDuplicates(const vtkm::cont::Invoker& invoker,
                     const vtkm::cont::ArrayHandle<KeyType, KeyStorage>& original_keys,
                     vtkm::cont::ArrayHandle<vtkm::FloatDefault>& weights,
                     vtkm::cont::ArrayHandle<vtkm::Id2>& edgeIds,
                     vtkm::cont::ArrayHandle<vtkm::Id>& cellids,
                     vtkm::cont::ArrayHandle<vtkm::Id>& connectivity)
{
  vtkm::cont::ArrayHandle<KeyType> input_keys;
  vtkm::cont::ArrayCopyDevice(original_keys, input_keys);
  vtkm::worklet::Keys<KeyType> keys(input_keys);
  input_keys.ReleaseResources();

  {
    vtkm::cont::ArrayHandle<vtkm::Id> writeCells;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> writeWeights;
    invoker(MergeDuplicateValues{}, keys, weights, cellids, writeWeights, writeCells);
    weights = writeWeights;
    cellids = writeCells;
  }

  //need to build the new connectivity
  auto uniqueKeys = keys.GetUniqueKeys();
  vtkm::cont::Algorithm::LowerBounds(
    uniqueKeys, original_keys, connectivity, marching_cells::MultiContourLess());

  //update the edge ids
  invoker(CopyEdgeIds{}, uniqueKeys, edgeIds);
}

// -----------------------------------------------------------------------------
template <vtkm::IdComponent Comp>
struct EdgeVertex
{
  VTKM_EXEC vtkm::Id operator()(const vtkm::Id2& edge) const { return edge[Comp]; }
};

class NormalsWorkletPass1 : public vtkm::worklet::WorkletVisitPointsWithCells
{
private:
  using PointIdsArray =
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandle<vtkm::Id2>, EdgeVertex<0>>;

public:
  using ControlSignature = void(CellSetIn,
                                WholeCellSetIn<Cell, Point>,
                                WholeArrayIn pointCoordinates,
                                WholeArrayIn inputField,
                                FieldOutPoint normals);

  using ExecutionSignature = void(CellCount, CellIndices, InputIndex, _2, _3, _4, _5);

  using InputDomain = _1;
  using ScatterType = vtkm::worklet::ScatterPermutation<typename PointIdsArray::StorageTag>;

  VTKM_CONT
  static ScatterType MakeScatter(const vtkm::cont::ArrayHandle<vtkm::Id2>& edges)
  {
    return ScatterType(vtkm::cont::make_ArrayHandleTransform(edges, EdgeVertex<0>()));
  }

  template <typename FromIndexType,
            typename CellSetInType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename NormalType>
  VTKM_EXEC void operator()(const vtkm::IdComponent& numCells,
                            const FromIndexType& cellIds,
                            vtkm::Id pointId,
                            const CellSetInType& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            NormalType& normal) const
  {
    vtkm::worklet::gradient::PointGradient gradient;
    gradient(numCells, cellIds, pointId, geometry, pointCoordinates, inputField, normal);
  }

  template <typename FromIndexType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename NormalType>
  VTKM_EXEC void operator()(const vtkm::IdComponent& vtkmNotUsed(numCells),
                            const FromIndexType& vtkmNotUsed(cellIds),
                            vtkm::Id pointId,
                            vtkm::exec::ConnectivityStructured<Cell, Point, 3>& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            NormalType& normal) const
  {
    //Optimization for structured cellsets so we can call StructuredPointGradient
    //and have way faster gradients
    vtkm::exec::ConnectivityStructured<Point, Cell, 3> pointGeom(geometry);
    vtkm::exec::arg::ThreadIndicesPointNeighborhood tpn(pointId, pointId, 0, pointId, pointGeom);

    const auto& boundary = tpn.GetBoundaryState();
    auto pointPortal = pointCoordinates.GetPortal();
    auto fieldPortal = inputField.GetPortal();
    vtkm::exec::FieldNeighborhood<decltype(pointPortal)> points(pointPortal, boundary);
    vtkm::exec::FieldNeighborhood<decltype(fieldPortal)> field(fieldPortal, boundary);

    vtkm::worklet::gradient::StructuredPointGradient gradient;
    gradient(boundary, points, field, normal);
  }
};

class NormalsWorkletPass2 : public vtkm::worklet::WorkletVisitPointsWithCells
{
private:
  using PointIdsArray =
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandle<vtkm::Id2>, EdgeVertex<1>>;

public:
  typedef void ControlSignature(CellSetIn,
                                WholeCellSetIn<Cell, Point>,
                                WholeArrayIn pointCoordinates,
                                WholeArrayIn inputField,
                                WholeArrayIn weights,
                                FieldInOutPoint normals);

  using ExecutionSignature =
    void(CellCount, CellIndices, InputIndex, _2, _3, _4, WorkIndex, _5, _6);

  using InputDomain = _1;
  using ScatterType = vtkm::worklet::ScatterPermutation<typename PointIdsArray::StorageTag>;

  VTKM_CONT
  static ScatterType MakeScatter(const vtkm::cont::ArrayHandle<vtkm::Id2>& edges)
  {
    return ScatterType(vtkm::cont::make_ArrayHandleTransform(edges, EdgeVertex<1>()));
  }

  template <typename FromIndexType,
            typename CellSetInType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename WholeWeightsIn,
            typename NormalType>
  VTKM_EXEC void operator()(const vtkm::IdComponent& numCells,
                            const FromIndexType& cellIds,
                            vtkm::Id pointId,
                            const CellSetInType& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            vtkm::Id edgeId,
                            const WholeWeightsIn& weights,
                            NormalType& normal) const
  {
    vtkm::worklet::gradient::PointGradient gradient;
    NormalType grad1;
    gradient(numCells, cellIds, pointId, geometry, pointCoordinates, inputField, grad1);

    NormalType grad0 = normal;
    auto weight = weights.Get(edgeId);
    normal = vtkm::Normal(vtkm::Lerp(grad0, grad1, weight));
  }

  template <typename FromIndexType,
            typename WholeCoordinatesIn,
            typename WholeFieldIn,
            typename WholeWeightsIn,
            typename NormalType>
  VTKM_EXEC void operator()(const vtkm::IdComponent& vtkmNotUsed(numCells),
                            const FromIndexType& vtkmNotUsed(cellIds),
                            vtkm::Id pointId,
                            vtkm::exec::ConnectivityStructured<Cell, Point, 3>& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            vtkm::Id edgeId,
                            const WholeWeightsIn& weights,
                            NormalType& normal) const
  {
    //Optimization for structured cellsets so we can call StructuredPointGradient
    //and have way faster gradients
    vtkm::exec::ConnectivityStructured<Point, Cell, 3> pointGeom(geometry);
    vtkm::exec::arg::ThreadIndicesPointNeighborhood tpn(pointId, pointId, 0, pointId, pointGeom);

    const auto& boundary = tpn.GetBoundaryState();
    auto pointPortal = pointCoordinates.GetPortal();
    auto fieldPortal = inputField.GetPortal();
    vtkm::exec::FieldNeighborhood<decltype(pointPortal)> points(pointPortal, boundary);
    vtkm::exec::FieldNeighborhood<decltype(fieldPortal)> field(fieldPortal, boundary);

    vtkm::worklet::gradient::StructuredPointGradient gradient;
    NormalType grad1;
    gradient(boundary, points, field, grad1);

    NormalType grad0 = normal;
    auto weight = weights.Get(edgeId);
    normal = vtkm::Lerp(grad0, grad1, weight);
    const auto mag2 = vtkm::MagnitudeSquared(normal);
    if (mag2 > 0.)
    {
      normal = normal * vtkm::RSqrt(mag2);
    }
  }
};


struct GenerateNormals
{
  template <typename CoordinateSystem,
            typename NormalCType,
            typename InputFieldType,
            typename InputStorageType,
            typename CellSet>
  void operator()(const CoordinateSystem& coordinates,
                  const vtkm::cont::Invoker& invoker,
                  vtkm::cont::ArrayHandle<vtkm::Vec<NormalCType, 3>>& normals,
                  const vtkm::cont::ArrayHandle<InputFieldType, InputStorageType>& field,
                  const CellSet cellset,
                  const vtkm::cont::ArrayHandle<vtkm::Id2>& edges,
                  const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& weights) const
  {
    // To save memory, the normals computation is done in two passes. In the first
    // pass the gradient at the first vertex of each edge is computed and stored in
    // the normals array. In the second pass the gradient at the second vertex is
    // computed and the gradient of the first vertex is read from the normals array.
    // The final normal is interpolated from the two gradient values and stored
    // in the normals array.
    //
    auto scalarField = marching_cells::make_ScalarField(field);
    invoker(NormalsWorkletPass1{},
            NormalsWorkletPass1::MakeScatter(edges),
            cellset,
            cellset,
            coordinates,
            scalarField,
            normals);

    invoker(NormalsWorkletPass2{},
            NormalsWorkletPass2::MakeScatter(edges),
            cellset,
            cellset,
            coordinates,
            scalarField,
            weights,
            normals);
  }
};

//----------------------------------------------------------------------------
template <typename CellSetType,
          typename CoordinateSystem,
          typename ValueType,
          typename StorageTagField,
          typename StorageTagVertices,
          typename StorageTagNormals,
          typename CoordinateType,
          typename NormalType>
vtkm::cont::CellSetSingleType<> execute(
  const CellSetType& cells,
  const CoordinateSystem& coordinateSystem,
  const std::vector<ValueType>& isovalues,
  const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
  vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices>& vertices,
  vtkm::cont::ArrayHandle<vtkm::Vec<NormalType, 3>, StorageTagNormals>& normals,
  vtkm::worklet::contour::CommonState& sharedState)
{
  using vtkm::worklet::contour::MapPointField;
  using vtkm::worklet::marching_cells::ClassifyCell;
  using vtkm::worklet::marching_cells::EdgeWeightGenerate;
  using vtkm::worklet::marching_cells::EdgeWeightGenerateMetaData;

  vtkm::worklet::marching_cells::CellClassifyTable classTable;
  vtkm::worklet::marching_cells::TriangleGenerationTable triTable;

  // Setup the invoker
  vtkm::cont::Invoker invoker;

  vtkm::cont::ArrayHandle<ValueType> isoValuesHandle =
    vtkm::cont::make_ArrayHandle(isovalues, vtkm::CopyFlag::Off);

  // Call the ClassifyCell functor to compute the Marching Cubes case numbers
  // for each cell, and the number of vertices to be generated
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutputTrisPerCell;
  {
    marching_cells::ClassifyCell<ValueType> classifyCell;
    invoker(classifyCell, isoValuesHandle, inputField, cells, numOutputTrisPerCell, classTable);
  }

  //Pass 2 Generate the edges
  vtkm::cont::ArrayHandle<vtkm::UInt8> contourIds;
  vtkm::cont::ArrayHandle<vtkm::Id> originalCellIdsForPoints;
  {
    auto scatter = EdgeWeightGenerate<ValueType>::MakeScatter(numOutputTrisPerCell);

    // Maps output cells to input cells. Store this for cell field mapping.
    sharedState.CellIdMap = scatter.GetOutputToInputMap();

    EdgeWeightGenerateMetaData metaData(
      scatter.GetOutputRange(numOutputTrisPerCell.GetNumberOfValues()),
      sharedState.InterpolationWeights,
      sharedState.InterpolationEdgeIds,
      originalCellIdsForPoints,
      contourIds);

    invoker(EdgeWeightGenerate<ValueType>{},
            scatter,
            cells,
            //cast to a scalar field if not one, as cellderivative only works on those
            isoValuesHandle,
            inputField,
            metaData,
            classTable,
            triTable);
  }

  if (isovalues.size() <= 1 || !sharedState.MergeDuplicatePoints)
  { //release memory early that we are not going to need again
    contourIds.ReleaseResources();
  }

  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  if (sharedState.MergeDuplicatePoints)
  {
    // In all the below cases you will notice that only interpolation ids
    // are updated. That is because MergeDuplicates will internally update
    // the InterpolationWeights and InterpolationOriginCellIds arrays to be the correct for the
    // output. But for InterpolationEdgeIds we need to do it manually once done
    if (isovalues.size() == 1)
    {
      marching_cells::MergeDuplicates(invoker,
                                      sharedState.InterpolationEdgeIds, //keys
                                      sharedState.InterpolationWeights, //values
                                      sharedState.InterpolationEdgeIds, //values
                                      originalCellIdsForPoints,         //values
                                      connectivity); // computed using lower bounds
    }
    else
    {
      marching_cells::MergeDuplicates(
        invoker,
        vtkm::cont::make_ArrayHandleZip(contourIds, sharedState.InterpolationEdgeIds), //keys
        sharedState.InterpolationWeights,                                              //values
        sharedState.InterpolationEdgeIds,                                              //values
        originalCellIdsForPoints,                                                      //values
        connectivity); // computed using lower bounds
    }
  }
  else
  {
    //when we don't merge points, the connectivity array can be represented
    //by a counting array. The danger of doing it this way is that the output
    //type is unknown. That is why we copy it into an explicit array
    vtkm::cont::ArrayHandleIndex temp(sharedState.InterpolationEdgeIds.GetNumberOfValues());
    vtkm::cont::ArrayCopy(temp, connectivity);
  }

  //generate the vertices's
  invoker(MapPointField{},
          sharedState.InterpolationEdgeIds,
          sharedState.InterpolationWeights,
          coordinateSystem,
          vertices);

  //assign the connectivity to the cell set
  vtkm::cont::CellSetSingleType<> outputCells;
  outputCells.Fill(vertices.GetNumberOfValues(), vtkm::CELL_SHAPE_TRIANGLE, 3, connectivity);

  //now that the vertices have been generated we can generate the normals
  if (sharedState.GenerateNormals)
  {
    GenerateNormals genNorms;
    genNorms(coordinateSystem,
             invoker,
             normals,
             inputField,
             cells,
             sharedState.InterpolationEdgeIds,
             sharedState.InterpolationWeights);
  }

  return outputCells;
}
}
}
} // namespace vtkm::worklet::marching_cells

#endif // vtk_m_worklet_contour_MarchingCells_h
