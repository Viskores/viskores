//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_Contour_h
#define vtk_m_worklet_Contour_h

#include <vtkm/BinaryPredicates.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherPointNeighborhood.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/ScatterPermutation.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

#include <vtkm/worklet/contour/ContourTables.h>
#include <vtkm/worklet/gradient/PointGradient.h>
#include <vtkm/worklet/gradient/StructuredPointGradient.h>

namespace vtkm
{
namespace worklet
{

namespace contour
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
  template <typename DeviceAdapter>
  class ExecObject
  {
    template <typename FieldType>
    struct PortalTypes
    {
      using HandleType = vtkm::cont::ArrayHandle<FieldType>;
      using ExecutionTypes = typename HandleType::template ExecutionTypes<DeviceAdapter>;

      using Portal = typename ExecutionTypes::Portal;
      using PortalConst = typename ExecutionTypes::PortalConst;
    };

  public:
    ExecObject() = default;

    VTKM_CONT
    ExecObject(vtkm::Id size,
               vtkm::cont::ArrayHandle<vtkm::FloatDefault>& interpWeights,
               vtkm::cont::ArrayHandle<vtkm::Id2>& interpIds,
               vtkm::cont::ArrayHandle<vtkm::Id>& interpCellIds,
               vtkm::cont::ArrayHandle<vtkm::UInt8>& interpContourId)
      : InterpWeightsPortal(interpWeights.PrepareForOutput(3 * size, DeviceAdapter()))
      , InterpIdPortal(interpIds.PrepareForOutput(3 * size, DeviceAdapter()))
      , InterpCellIdPortal(interpCellIds.PrepareForOutput(3 * size, DeviceAdapter()))
      , InterpContourPortal(interpContourId.PrepareForOutput(3 * size, DeviceAdapter()))
    {
      // Interp needs to be 3 times longer than size as they are per point of the
      // output triangle
    }
    typename PortalTypes<vtkm::FloatDefault>::Portal InterpWeightsPortal;
    typename PortalTypes<vtkm::Id2>::Portal InterpIdPortal;
    typename PortalTypes<vtkm::Id>::Portal InterpCellIdPortal;
    typename PortalTypes<vtkm::UInt8>::Portal InterpContourPortal;
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

  template <typename DeviceAdapter>
  VTKM_CONT ExecObject<DeviceAdapter> PrepareForExecution(DeviceAdapter)
  {
    return ExecObject<DeviceAdapter>(
      this->Size, this->InterpWeights, this->InterpIds, this->InterpCellIds, this->InterpContourId);
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
            typename IndicesVecType,
            typename DeviceAdapter>
  VTKM_EXEC void operator()(const CellShape shape,
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn, // Input point field defining the contour
                            const EdgeWeightGenerateMetaData::ExecObject<DeviceAdapter>& metaData,
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
class MapPointField : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn interpolation_ids,
                                FieldIn interpolation_weights,
                                WholeArrayIn inputField,
                                FieldOut output);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  VTKM_CONT
  MapPointField() {}

  template <typename WeightType, typename InFieldPortalType, typename OutFieldType>
  VTKM_EXEC void operator()(const vtkm::Id2& low_high,
                            const WeightType& weight,
                            const InFieldPortalType& inPortal,
                            OutFieldType& result) const
  {
    //fetch the low / high values from inPortal
    result = static_cast<OutFieldType>(
      vtkm::Lerp(inPortal.Get(low_high[0]), inPortal.Get(low_high[1]), weight));
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
void MergeDuplicates(const vtkm::cont::ArrayHandle<KeyType, KeyStorage>& original_keys,
                     vtkm::cont::ArrayHandle<vtkm::FloatDefault>& weights,
                     vtkm::cont::ArrayHandle<vtkm::Id2>& edgeIds,
                     vtkm::cont::ArrayHandle<vtkm::Id>& cellids,
                     vtkm::cont::ArrayHandle<vtkm::Id>& connectivity)
{
  vtkm::cont::ArrayHandle<KeyType> input_keys;
  vtkm::cont::ArrayCopy(original_keys, input_keys);
  vtkm::worklet::Keys<KeyType> keys(input_keys);
  input_keys.ReleaseResources();

  {
    vtkm::worklet::DispatcherReduceByKey<MergeDuplicateValues> dispatcher;
    vtkm::cont::ArrayHandle<vtkm::Id> writeCells;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> writeWeights;
    dispatcher.Invoke(keys, weights, cellids, writeWeights, writeCells);
    weights = writeWeights;
    cellids = writeCells;
  }

  //need to build the new connectivity
  auto uniqueKeys = keys.GetUniqueKeys();
  vtkm::cont::Algorithm::LowerBounds(
    uniqueKeys, original_keys, connectivity, contour::MultiContourLess());

  //update the edge ids
  vtkm::worklet::DispatcherMapField<CopyEdgeIds> edgeDispatcher;
  edgeDispatcher.Invoke(uniqueKeys, edgeIds);
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
    using T = typename WholeFieldIn::ValueType;
    vtkm::worklet::gradient::PointGradient<T> gradient;
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
    using T = typename WholeFieldIn::ValueType;

    //Optimization for structured cellsets so we can call StructuredPointGradient
    //and have way faster gradients
    vtkm::exec::ConnectivityStructured<Point, Cell, 3> pointGeom(geometry);
    vtkm::exec::arg::ThreadIndicesPointNeighborhood tpn(pointId, pointId, 0, pointId, pointGeom, 0);

    const auto& boundary = tpn.GetBoundaryState();
    auto pointPortal = pointCoordinates.GetPortal();
    auto fieldPortal = inputField.GetPortal();
    vtkm::exec::FieldNeighborhood<decltype(pointPortal)> points(pointPortal, boundary);
    vtkm::exec::FieldNeighborhood<decltype(fieldPortal)> field(fieldPortal, boundary);

    vtkm::worklet::gradient::StructuredPointGradient<T> gradient;
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
    using T = typename WholeFieldIn::ValueType;
    vtkm::worklet::gradient::PointGradient<T> gradient;
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
    using T = typename WholeFieldIn::ValueType;
    //Optimization for structured cellsets so we can call StructuredPointGradient
    //and have way faster gradients
    vtkm::exec::ConnectivityStructured<Point, Cell, 3> pointGeom(geometry);
    vtkm::exec::arg::ThreadIndicesPointNeighborhood tpn(pointId, pointId, 0, pointId, pointGeom, 0);

    const auto& boundary = tpn.GetBoundaryState();
    auto pointPortal = pointCoordinates.GetPortal();
    auto fieldPortal = inputField.GetPortal();
    vtkm::exec::FieldNeighborhood<decltype(pointPortal)> points(pointPortal, boundary);
    vtkm::exec::FieldNeighborhood<decltype(fieldPortal)> field(fieldPortal, boundary);

    vtkm::worklet::gradient::StructuredPointGradient<T> gradient;
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

template <typename NormalCType,
          typename InputFieldType,
          typename InputStorageType,
          typename CellSet>
struct GenerateNormalsDeduced
{
  vtkm::cont::ArrayHandle<vtkm::Vec<NormalCType, 3>>* normals;
  const vtkm::cont::ArrayHandle<InputFieldType, InputStorageType>* field;
  const CellSet* cellset;
  vtkm::cont::ArrayHandle<vtkm::Id2>* edges;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault>* weights;

  template <typename CoordinateSystem>
  void operator()(const CoordinateSystem& coordinates) const
  {
    // To save memory, the normals computation is done in two passes. In the first
    // pass the gradient at the first vertex of each edge is computed and stored in
    // the normals array. In the second pass the gradient at the second vertex is
    // computed and the gradient of the first vertex is read from the normals array.
    // The final normal is interpolated from the two gradient values and stored
    // in the normals array.
    //
    vtkm::worklet::DispatcherMapTopology<NormalsWorkletPass1> dispatcherNormalsPass1(
      NormalsWorkletPass1::MakeScatter(*edges));
    dispatcherNormalsPass1.Invoke(
      *cellset, *cellset, coordinates, contour::make_ScalarField(*field), *normals);

    vtkm::worklet::DispatcherMapTopology<NormalsWorkletPass2> dispatcherNormalsPass2(
      NormalsWorkletPass2::MakeScatter(*edges));
    dispatcherNormalsPass2.Invoke(
      *cellset, *cellset, coordinates, contour::make_ScalarField(*field), *weights, *normals);
  }
};

template <typename NormalCType,
          typename InputFieldType,
          typename InputStorageType,
          typename CellSet,
          typename CoordinateSystem>
void GenerateNormals(vtkm::cont::ArrayHandle<vtkm::Vec<NormalCType, 3>>& normals,
                     const vtkm::cont::ArrayHandle<InputFieldType, InputStorageType>& field,
                     const CellSet& cellset,
                     const CoordinateSystem& coordinates,
                     vtkm::cont::ArrayHandle<vtkm::Id2>& edges,
                     vtkm::cont::ArrayHandle<vtkm::FloatDefault>& weights)
{
  GenerateNormalsDeduced<NormalCType, InputFieldType, InputStorageType, CellSet> functor;
  functor.normals = &normals;
  functor.field = &field;
  functor.cellset = &cellset;
  functor.edges = &edges;
  functor.weights = &weights;


  vtkm::cont::CastAndCall(coordinates, functor);
}
}

/// \brief Compute the isosurface for a uniform grid data set
class Contour
{
public:
  //----------------------------------------------------------------------------
  Contour(bool mergeDuplicates = true)
    : MergeDuplicatePoints(mergeDuplicates)
    , InterpolationWeights()
    , InterpolationEdgeIds()
  {
  }

  //----------------------------------------------------------------------------
  vtkm::cont::ArrayHandle<vtkm::Id2> GetInterpolationEdgeIds() const
  {
    return this->InterpolationEdgeIds;
  }

  //----------------------------------------------------------------------------
  void SetMergeDuplicatePoints(bool merge) { this->MergeDuplicatePoints = merge; }

  //----------------------------------------------------------------------------
  bool GetMergeDuplicatePoints() const { return this->MergeDuplicatePoints; }

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices>
  vtkm::cont::CellSetSingleType<> Run(
    const ValueType* const isovalues,
    const vtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>> normals;
    return this->DeduceRun(
      isovalues, numIsoValues, cells, coordinateSystem, input, vertices, normals, false);
  }

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices,
            typename StorageTagNormals>
  vtkm::cont::CellSetSingleType<> Run(
    const ValueType* const isovalues,
    const vtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagNormals> normals)
  {
    return this->DeduceRun(
      isovalues, numIsoValues, cells, coordinateSystem, input, vertices, normals, true);
  }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& input) const
  {
    using vtkm::worklet::contour::MapPointField;
    MapPointField applyToField;
    vtkm::worklet::DispatcherMapField<MapPointField> applyFieldDispatcher(applyToField);

    vtkm::cont::ArrayHandle<ValueType> output;
    applyFieldDispatcher.Invoke(
      this->InterpolationEdgeIds, this->InterpolationWeights, input, output);
    return output;
  }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& in) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->CellIdMap, in);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    vtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

  //----------------------------------------------------------------------------
  void ReleaseCellMapArrays() { this->CellIdMap.ReleaseResources(); }

private:
  template <typename ValueType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename StorageTagVertices,
            typename StorageTagNormals,
            typename CoordinateType,
            typename NormalType>
  struct DeduceCellType
  {
    Contour* MC = nullptr;
    const ValueType* isovalues = nullptr;
    const vtkm::Id* numIsoValues = nullptr;
    const CoordinateSystem* coordinateSystem = nullptr;
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>* inputField = nullptr;
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices>* vertices;
    vtkm::cont::ArrayHandle<vtkm::Vec<NormalType, 3>, StorageTagNormals>* normals;
    const bool* withNormals;
    vtkm::cont::CellSetSingleType<>* result;

    template <typename CellSetType>
    void operator()(const CellSetType& cells) const
    {
      if (this->MC)
      {
        *this->result = this->MC->DoRun(isovalues,
                                        *numIsoValues,
                                        cells,
                                        *coordinateSystem,
                                        *inputField,
                                        *vertices,
                                        *normals,
                                        *withNormals);
      }
    }
  };

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename StorageTagVertices,
            typename StorageTagNormals,
            typename CoordinateType,
            typename NormalType>
  vtkm::cont::CellSetSingleType<> DeduceRun(
    const ValueType* isovalues,
    const vtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec<NormalType, 3>, StorageTagNormals> normals,
    bool withNormals)
  {
    vtkm::cont::CellSetSingleType<> outputCells;

    DeduceCellType<ValueType,
                   CoordinateSystem,
                   StorageTagField,
                   StorageTagVertices,
                   StorageTagNormals,
                   CoordinateType,
                   NormalType>
      functor;
    functor.MC = this;
    functor.isovalues = isovalues;
    functor.numIsoValues = &numIsoValues;
    functor.coordinateSystem = &coordinateSystem;
    functor.inputField = &inputField;
    functor.vertices = &vertices;
    functor.normals = &normals;
    functor.withNormals = &withNormals;
    functor.result = &outputCells;

    vtkm::cont::CastAndCall(cells, functor);

    return outputCells;
  }

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename StorageTagVertices,
            typename StorageTagNormals,
            typename CoordinateType,
            typename NormalType>
  vtkm::cont::CellSetSingleType<> DoRun(
    const ValueType* isovalues,
    const vtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec<NormalType, 3>, StorageTagNormals> normals,
    bool withNormals)
  {
    using vtkm::worklet::contour::ClassifyCell;
    using vtkm::worklet::contour::EdgeWeightGenerate;
    using vtkm::worklet::contour::EdgeWeightGenerateMetaData;
    using vtkm::worklet::contour::MapPointField;

    // Setup the Dispatcher Typedefs
    using ClassifyDispatcher = vtkm::worklet::DispatcherMapTopology<ClassifyCell<ValueType>>;

    using GenerateDispatcher = vtkm::worklet::DispatcherMapTopology<EdgeWeightGenerate<ValueType>>;

    vtkm::cont::ArrayHandle<ValueType> isoValuesHandle =
      vtkm::cont::make_ArrayHandle(isovalues, numIsoValues);

    // Call the ClassifyCell functor to compute the Marching Cubes case numbers
    // for each cell, and the number of vertices to be generated
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutputTrisPerCell;
    {
      contour::ClassifyCell<ValueType> classifyCell;
      ClassifyDispatcher dispatcher(classifyCell);
      dispatcher.Invoke(isoValuesHandle, inputField, cells, numOutputTrisPerCell, this->classTable);
    }

    //Pass 2 Generate the edges
    vtkm::cont::ArrayHandle<vtkm::UInt8> contourIds;
    vtkm::cont::ArrayHandle<vtkm::Id> originalCellIdsForPoints;
    {
      auto scatter = EdgeWeightGenerate<ValueType>::MakeScatter(numOutputTrisPerCell);

      // Maps output cells to input cells. Store this for cell field mapping.
      this->CellIdMap = scatter.GetOutputToInputMap();

      EdgeWeightGenerateMetaData metaData(
        scatter.GetOutputRange(numOutputTrisPerCell.GetNumberOfValues()),
        this->InterpolationWeights,
        this->InterpolationEdgeIds,
        originalCellIdsForPoints,
        contourIds);

      EdgeWeightGenerate<ValueType> weightGenerate;
      GenerateDispatcher edgeDispatcher(weightGenerate, scatter);
      edgeDispatcher.Invoke(
        cells,
        //cast to a scalar field if not one, as cellderivative only works on those
        isoValuesHandle,
        inputField,
        metaData,
        this->classTable,
        this->triTable);
    }

    if (numIsoValues <= 1 || !this->MergeDuplicatePoints)
    { //release memory early that we are not going to need again
      contourIds.ReleaseResources();
    }

    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    if (this->MergeDuplicatePoints)
    {
      // In all the below cases you will notice that only interpolation ids
      // are updated. That is because MergeDuplicates will internally update
      // the InterpolationWeights and InterpolationOriginCellIds arrays to be the correct for the
      // output. But for InterpolationEdgeIds we need to do it manually once done
      if (numIsoValues == 1)
      {
        contour::MergeDuplicates(this->InterpolationEdgeIds, //keys
                                 this->InterpolationWeights, //values
                                 this->InterpolationEdgeIds, //values
                                 originalCellIdsForPoints,   //values
                                 connectivity);              // computed using lower bounds
      }
      else if (numIsoValues > 1)
      {
        contour::MergeDuplicates(
          vtkm::cont::make_ArrayHandleZip(contourIds, this->InterpolationEdgeIds), //keys
          this->InterpolationWeights,                                              //values
          this->InterpolationEdgeIds,                                              //values
          originalCellIdsForPoints,                                                //values
          connectivity); // computed using lower bounds
      }
    }
    else
    {
      //when we don't merge points, the connectivity array can be represented
      //by a counting array. The danger of doing it this way is that the output
      //type is unknown. That is why we copy it into an explicit array
      vtkm::cont::ArrayHandleIndex temp(this->InterpolationEdgeIds.GetNumberOfValues());
      vtkm::cont::ArrayCopy(temp, connectivity);
    }

    //generate the vertices's
    MapPointField applyToField;
    vtkm::worklet::DispatcherMapField<MapPointField> applyFieldDispatcher(applyToField);

    applyFieldDispatcher.Invoke(
      this->InterpolationEdgeIds, this->InterpolationWeights, coordinateSystem, vertices);

    //assign the connectivity to the cell set
    vtkm::cont::CellSetSingleType<> outputCells;
    outputCells.Fill(vertices.GetNumberOfValues(), vtkm::CELL_SHAPE_TRIANGLE, 3, connectivity);

    //now that the vertices have been generated we can generate the normals
    if (withNormals)
    {
      contour::GenerateNormals(normals,
                               inputField,
                               cells,
                               coordinateSystem,
                               this->InterpolationEdgeIds,
                               this->InterpolationWeights);
    }

    return outputCells;
  }

  bool MergeDuplicatePoints;
  vtkm::worklet::internal::CellClassifyTable classTable;
  vtkm::worklet::internal::TriangleGenerationTable triTable;

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> InterpolationWeights;
  vtkm::cont::ArrayHandle<vtkm::Id2> InterpolationEdgeIds;

  vtkm::cont::ArrayHandle<vtkm::Id> CellIdMap;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_Contour_h
