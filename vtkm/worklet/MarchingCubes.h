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
//============================================================================

#ifndef vtk_m_worklet_MarchingCubes_h
#define vtk_m_worklet_MarchingCubes_h

#include <vtkm/BinaryPredicates.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherPointNeighborhood.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/ScatterPermutation.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

#include <vtkm/worklet/contour/DataTables.h>
#include <vtkm/worklet/gradient/PointGradient.h>
#include <vtkm/worklet/gradient/StructuredPointGradient.h>

namespace vtkm
{
namespace worklet
{

namespace marchingcubes
{

template <typename T>
struct float_type
{
  using type = vtkm::FloatDefault;
};
template <>
struct float_type<vtkm::Float32>
{
  using type = vtkm::Float32;
};
template <>
struct float_type<vtkm::Float64>
{
  using type = vtkm::Float64;
};

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
class ClassifyCell : public vtkm::worklet::WorkletMapPointToCell
{
public:
  struct ClassifyCellTagType : vtkm::ListTagBase<T>
  {
  };

  typedef void ControlSignature(WholeArrayIn<ClassifyCellTagType> isoValues,
                                FieldInPoint<ClassifyCellTagType> fieldIn,
                                CellSetIn cellset,
                                FieldOutCell<IdComponentType> outNumTriangles,
                                WholeArrayIn<IdComponentType> numTrianglesTable);
  typedef void ExecutionSignature(CellShape, _1, _2, _4, _5);
  typedef _3 InputDomain;

  template <typename IsoValuesType, typename FieldInType, typename NumTrianglesTablePortalType>
  VTKM_EXEC void operator()(vtkm::CellShapeTagGeneric shape,
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn,
                            vtkm::IdComponent& numTriangles,
                            const NumTrianglesTablePortalType& numTrianglesTable) const
  {
    if (shape.Id == CELL_SHAPE_HEXAHEDRON)
    {
      this->operator()(
        vtkm::CellShapeTagHexahedron(), isovalues, fieldIn, numTriangles, numTrianglesTable);
    }
    else
    {
      numTriangles = 0;
    }
  }

  template <typename IsoValuesType, typename FieldInType, typename NumTrianglesTablePortalType>
  VTKM_EXEC void operator()(vtkm::CellShapeTagQuad vtkmNotUsed(shape),
                            const IsoValuesType& vtkmNotUsed(isovalues),
                            const FieldInType& vtkmNotUsed(fieldIn),
                            vtkm::IdComponent& vtkmNotUsed(numTriangles),
                            const NumTrianglesTablePortalType& vtkmNotUsed(numTrianglesTable)) const
  {
  }

  template <typename IsoValuesType, typename FieldInType, typename NumTrianglesTablePortalType>
  VTKM_EXEC void operator()(vtkm::CellShapeTagHexahedron vtkmNotUsed(shape),
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn,
                            vtkm::IdComponent& numTriangles,
                            const NumTrianglesTablePortalType& numTrianglesTable) const
  {
    vtkm::IdComponent sum = 0;
    for (vtkm::Id i = 0; i < isovalues.GetNumberOfValues(); ++i)
    {
      const vtkm::IdComponent caseNumber =
        ((fieldIn[0] > isovalues[i]) | (fieldIn[1] > isovalues[i]) << 1 |
         (fieldIn[2] > isovalues[i]) << 2 | (fieldIn[3] > isovalues[i]) << 3 |
         (fieldIn[4] > isovalues[i]) << 4 | (fieldIn[5] > isovalues[i]) << 5 |
         (fieldIn[6] > isovalues[i]) << 6 | (fieldIn[7] > isovalues[i]) << 7);
      sum += numTrianglesTable.Get(caseNumber);
    }
    numTriangles = sum;
  }
};

/// \brief Used to store data need for the EdgeWeightGenerate worklet.
/// This information is not passed as part of the arguments to the worklet as
/// that dramatically increase compile time by 200%
// -----------------------------------------------------------------------------
template <typename DeviceAdapter>
class EdgeWeightGenerateMetaData
{
  template <typename FieldType>
  struct PortalTypes
  {
    typedef vtkm::cont::ArrayHandle<FieldType> HandleType;
    typedef typename HandleType::template ExecutionTypes<DeviceAdapter> ExecutionTypes;

    typedef typename ExecutionTypes::Portal Portal;
    typedef typename ExecutionTypes::PortalConst PortalConst;
  };

public:
  VTKM_CONT
  EdgeWeightGenerateMetaData(vtkm::Id size,
                             vtkm::cont::ArrayHandle<vtkm::FloatDefault>& interpWeights,
                             vtkm::cont::ArrayHandle<vtkm::Id2>& interpIds,
                             vtkm::cont::ArrayHandle<vtkm::Id>& interpCellIds,
                             vtkm::cont::ArrayHandle<vtkm::UInt8>& interpContourId,
                             const vtkm::cont::ArrayHandle<vtkm::IdComponent>& edgeTable,
                             const vtkm::cont::ArrayHandle<vtkm::IdComponent>& numTriTable,
                             const vtkm::cont::ArrayHandle<vtkm::IdComponent>& triTable,
                             const vtkm::worklet::ScatterCounting& scatter)
    : InterpWeightsPortal(interpWeights.PrepareForOutput(3 * size, DeviceAdapter()))
    , InterpIdPortal(interpIds.PrepareForOutput(3 * size, DeviceAdapter()))
    , InterpCellIdPortal(interpCellIds.PrepareForOutput(3 * size, DeviceAdapter()))
    , InterpContourPortal(interpContourId.PrepareForOutput(3 * size, DeviceAdapter()))
    , EdgeTable(edgeTable.PrepareForInput(DeviceAdapter()))
    , NumTriTable(numTriTable.PrepareForInput(DeviceAdapter()))
    , TriTable(triTable.PrepareForInput(DeviceAdapter()))
    , Scatter(scatter)
  {
    // Interp needs to be 3 times longer than size as they are per point of the
    // output triangle
  }
  typename PortalTypes<vtkm::FloatDefault>::Portal InterpWeightsPortal;
  typename PortalTypes<vtkm::Id2>::Portal InterpIdPortal;
  typename PortalTypes<vtkm::Id>::Portal InterpCellIdPortal;
  typename PortalTypes<vtkm::UInt8>::Portal InterpContourPortal;
  typename PortalTypes<vtkm::IdComponent>::PortalConst EdgeTable;
  typename PortalTypes<vtkm::IdComponent>::PortalConst NumTriTable;
  typename PortalTypes<vtkm::IdComponent>::PortalConst TriTable;
  vtkm::worklet::ScatterCounting Scatter;
};

/// \brief Compute the weights for each edge that is used to generate
/// a point in the resulting iso-surface
// -----------------------------------------------------------------------------
template <typename T, typename DeviceAdapter>
class EdgeWeightGenerate : public vtkm::worklet::WorkletMapPointToCell
{
public:
  struct ClassifyCellTagType : vtkm::ListTagBase<T>
  {
  };

  typedef vtkm::worklet::ScatterCounting ScatterType;

  typedef void ControlSignature(
    CellSetIn cellset, // Cell set
    WholeArrayIn<ClassifyCellTagType> isoValues,
    FieldInPoint<ClassifyCellTagType> fieldIn // Input point field defining the contour
    );
  typedef void
    ExecutionSignature(CellShape, _2, _3, InputIndex, WorkIndex, VisitIndex, FromIndices);

  typedef _1 InputDomain;

  VTKM_CONT
  EdgeWeightGenerate(const EdgeWeightGenerateMetaData<DeviceAdapter>& meta)
    : MetaData(meta)
  {
  }

  template <typename IsoValuesType,
            typename FieldInType, // Vec-like, one per input point
            typename IndicesVecType>
  VTKM_EXEC void operator()(vtkm::CellShapeTagGeneric shape,
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn, // Input point field defining the contour
                            vtkm::Id inputCellId,
                            vtkm::Id outputCellId,
                            vtkm::IdComponent visitIndex,
                            const IndicesVecType& indices) const
  { //covers when we have hexs coming from unstructured data
    if (shape.Id == CELL_SHAPE_HEXAHEDRON)
    {
      this->operator()(vtkm::CellShapeTagHexahedron(),
                       isovalues,
                       fieldIn,
                       inputCellId,
                       outputCellId,
                       visitIndex,
                       indices);
    }
  }

  template <typename IsoValuesType,
            typename FieldInType, // Vec-like, one per input point
            typename IndicesVecType>
  VTKM_EXEC void operator()(
    CellShapeTagQuad vtkmNotUsed(shape),
    const IsoValuesType& vtkmNotUsed(isovalues),
    const FieldInType& vtkmNotUsed(fieldIn), // Input point field defining the contour
    vtkm::Id vtkmNotUsed(inputCellId),
    vtkm::Id vtkmNotUsed(outputCellId),
    vtkm::IdComponent vtkmNotUsed(visitIndex),
    const IndicesVecType& vtkmNotUsed(indices)) const
  { //covers when we have quads coming from 2d structured data
  }

  template <typename IsoValuesType,
            typename FieldInType, // Vec-like, one per input point
            typename IndicesVecType>
  VTKM_EXEC void operator()(vtkm::CellShapeTagHexahedron,
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn, // Input point field defining the contour
                            vtkm::Id inputCellId,
                            vtkm::Id outputCellId,
                            vtkm::IdComponent visitIndex,
                            const IndicesVecType& indices) const
  { //covers when we have hexs coming from 3d structured data
    const vtkm::Id outputPointId = 3 * outputCellId;
    typedef typename vtkm::VecTraits<FieldInType>::ComponentType FieldType;

    vtkm::IdComponent sum = 0, caseNumber = 0;
    vtkm::IdComponent i = 0, size = static_cast<vtkm::IdComponent>(isovalues.GetNumberOfValues());
    for (i = 0; i < size; ++i)
    {
      const FieldType ivalue = isovalues[i];
      // Compute the Marching Cubes case number for this cell. We need to iterate
      // the isovalues until the sum >= our visit index. But we need to make
      // sure the caseNumber is correct before stoping
      caseNumber =
        ((fieldIn[0] > ivalue) | (fieldIn[1] > ivalue) << 1 | (fieldIn[2] > ivalue) << 2 |
         (fieldIn[3] > ivalue) << 3 | (fieldIn[4] > ivalue) << 4 | (fieldIn[5] > ivalue) << 5 |
         (fieldIn[6] > ivalue) << 6 | (fieldIn[7] > ivalue) << 7);
      sum += MetaData.NumTriTable.Get(caseNumber);
      if (sum > visitIndex)
      {
        break;
      }
    }

    visitIndex = sum - visitIndex - 1;

    // Interpolate for vertex positions and associated scalar values
    const vtkm::Id triTableOffset = static_cast<vtkm::Id>(caseNumber * 16 + visitIndex * 3);
    for (vtkm::IdComponent triVertex = 0; triVertex < 3; triVertex++)
    {
      const vtkm::IdComponent edgeIndex = MetaData.TriTable.Get(triTableOffset + triVertex);
      const vtkm::IdComponent edgeVertex0 = MetaData.EdgeTable.Get(2 * edgeIndex + 0);
      const vtkm::IdComponent edgeVertex1 = MetaData.EdgeTable.Get(2 * edgeIndex + 1);
      const FieldType fieldValue0 = fieldIn[edgeVertex0];
      const FieldType fieldValue1 = fieldIn[edgeVertex1];

      // Store the input cell id so that we can properly generate the normals
      // in a subsequent call, after we have merged duplicate points
      MetaData.InterpCellIdPortal.Set(outputPointId + triVertex, inputCellId);

      MetaData.InterpContourPortal.Set(outputPointId + triVertex, static_cast<vtkm::UInt8>(i));

      MetaData.InterpIdPortal.Set(outputPointId + triVertex,
                                  vtkm::Id2(indices[edgeVertex0], indices[edgeVertex1]));

      vtkm::FloatDefault interpolant = static_cast<vtkm::FloatDefault>(isovalues[i] - fieldValue0) /
        static_cast<vtkm::FloatDefault>(fieldValue1 - fieldValue0);

      MetaData.InterpWeightsPortal.Set(outputPointId + triVertex, interpolant);
    }
  }

  VTKM_CONT
  ScatterType GetScatter() const { return this->MetaData.Scatter; }

private:
  EdgeWeightGenerateMetaData<DeviceAdapter> MetaData;

  void operator=(const EdgeWeightGenerate<T, DeviceAdapter>&) = delete;
};

// ---------------------------------------------------------------------------
class MapPointField : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<Id2Type> interpolation_ids,
                                FieldIn<Scalar> interpolation_weights,
                                WholeArrayIn<> inputField,
                                FieldOut<> output);
  typedef void ExecutionSignature(_1, _2, _3, _4);
  typedef _1 InputDomain;

  VTKM_CONT
  MapPointField() {}

  template <typename WeightType, typename InFieldPortalType, typename OutFieldType>
  VTKM_EXEC void operator()(const vtkm::Id2& low_high,
                            const WeightType& weight,
                            const InFieldPortalType& inPortal,
                            OutFieldType& result) const
  {
    //fetch the low / high values from inPortal
    result = vtkm::Lerp(inPortal.Get(low_high[0]), inPortal.Get(low_high[1]), weight);
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
  typedef void ControlSignature(KeysIn keys,
                                ValuesIn<> valuesIn1,
                                ValuesIn<> valuesIn2,
                                ReducedValuesOut<> valueOut1,
                                ReducedValuesOut<> valueOut2);
  typedef void ExecutionSignature(_1, _2, _3, _4, _5);
  typedef _1 InputDomain;

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
  typedef void ControlSignature(FieldIn<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2);
  typedef _1 InputDomain;

  VTKM_EXEC
  void operator()(const vtkm::Id2& input, vtkm::Id2& output) const { output = input; }

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Pair<T, vtkm::Id2>& input, vtkm::Id2& output) const
  {
    output = input.second;
  }
};

// ---------------------------------------------------------------------------
template <typename KeyType, typename KeyStorage, typename DeviceAdapterTag>
void MergeDuplicates(const vtkm::cont::ArrayHandle<KeyType, KeyStorage>& original_keys,
                     vtkm::cont::ArrayHandle<vtkm::FloatDefault>& weights,
                     vtkm::cont::ArrayHandle<vtkm::Id2>& edgeIds,
                     vtkm::cont::ArrayHandle<vtkm::Id>& cellids,
                     vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
                     DeviceAdapterTag)
{
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;

  vtkm::cont::ArrayHandle<KeyType> input_keys;
  Algorithm::Copy(original_keys, input_keys);
  vtkm::worklet::Keys<KeyType> keys(input_keys, DeviceAdapterTag());
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
  Algorithm::LowerBounds(
    uniqueKeys, original_keys, connectivity, marchingcubes::MultiContourLess());

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

class NormalsWorkletPass1 : public vtkm::worklet::WorkletMapCellToPoint
{
private:
  using PointIdsArray =
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandle<vtkm::Id2>, EdgeVertex<0>>;

public:
  typedef void ControlSignature(CellSetIn,
                                WholeCellSetIn<Point, Cell>,
                                WholeArrayIn<Vec3> pointCoordinates,
                                WholeArrayIn<Scalar> inputField,
                                FieldOutPoint<Vec3> normals);

  typedef void ExecutionSignature(CellCount, CellIndices, InputIndex, _2, _3, _4, _5);

  using InputDomain = _1;
  using ScatterType = vtkm::worklet::ScatterPermutation<typename PointIdsArray::StorageTag>;

  NormalsWorkletPass1(const vtkm::cont::ArrayHandle<vtkm::Id2>& edges)
    : Scatter(vtkm::cont::make_ArrayHandleTransform(edges, EdgeVertex<0>()))
  {
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
                            vtkm::exec::ConnectivityStructured<Point, Cell, 3>& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            NormalType& normal) const
  {
    using T = typename WholeFieldIn::ValueType;

    //Optimization for structured cellsets so we can call StructuredPointGradient
    //and have way faster gradients
    vtkm::exec::ConnectivityStructured<Cell, Point, 3> pointGeom(geometry);
    vtkm::exec::arg::ThreadIndicesPointNeighborhood<3> tpn(pointId, pointId, 0, pointGeom, 0);

    const auto& boundary = tpn.GetBoundaryState();
    auto pointPortal = pointCoordinates.GetPortal();
    auto fieldPortal = inputField.GetPortal();
    vtkm::exec::arg::Neighborhood<1, decltype(pointPortal)> points(pointPortal, boundary);
    vtkm::exec::arg::Neighborhood<1, decltype(fieldPortal)> field(fieldPortal, boundary);

    vtkm::worklet::gradient::StructuredPointGradient<T> gradient;
    gradient(boundary, points, field, normal);
  }

  ScatterType GetScatter() const { return this->Scatter; }

private:
  ScatterType Scatter;
};

class NormalsWorkletPass2 : public vtkm::worklet::WorkletMapCellToPoint
{
private:
  using PointIdsArray =
    vtkm::cont::ArrayHandleTransform<vtkm::cont::ArrayHandle<vtkm::Id2>, EdgeVertex<1>>;

public:
  typedef void ControlSignature(CellSetIn,
                                WholeCellSetIn<Point, Cell>,
                                WholeArrayIn<Vec3> pointCoordinates,
                                WholeArrayIn<Scalar> inputField,
                                WholeArrayIn<Scalar> weights,
                                FieldInOutPoint<Vec3> normals);

  typedef void
    ExecutionSignature(CellCount, CellIndices, InputIndex, _2, _3, _4, WorkIndex, _5, _6);

  using InputDomain = _1;
  using ScatterType = vtkm::worklet::ScatterPermutation<typename PointIdsArray::StorageTag>;

  NormalsWorkletPass2(const vtkm::cont::ArrayHandle<vtkm::Id2>& edges)
    : Scatter(vtkm::cont::make_ArrayHandleTransform(edges, EdgeVertex<1>()))
  {
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
                            vtkm::exec::ConnectivityStructured<Point, Cell, 3>& geometry,
                            const WholeCoordinatesIn& pointCoordinates,
                            const WholeFieldIn& inputField,
                            vtkm::Id edgeId,
                            const WholeWeightsIn& weights,
                            NormalType& normal) const
  {
    using T = typename WholeFieldIn::ValueType;
    //Optimization for structured cellsets so we can call StructuredPointGradient
    //and have way faster gradients
    vtkm::exec::ConnectivityStructured<Cell, Point, 3> pointGeom(geometry);
    vtkm::exec::arg::ThreadIndicesPointNeighborhood<3> tpn(pointId, pointId, 0, pointGeom, 0);

    const auto& boundary = tpn.GetBoundaryState();
    auto pointPortal = pointCoordinates.GetPortal();
    auto fieldPortal = inputField.GetPortal();
    vtkm::exec::arg::Neighborhood<1, decltype(pointPortal)> points(pointPortal, boundary);
    vtkm::exec::arg::Neighborhood<1, decltype(fieldPortal)> field(fieldPortal, boundary);

    vtkm::worklet::gradient::StructuredPointGradient<T> gradient;
    NormalType grad1;
    gradient(boundary, points, field, grad1);

    NormalType grad0 = normal;
    auto weight = weights.Get(edgeId);
    normal = vtkm::Normal(vtkm::Lerp(grad0, grad1, weight));
  }

  ScatterType GetScatter() const { return this->Scatter; }

private:
  ScatterType Scatter;
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
    NormalsWorkletPass1 pass1(*edges);
    vtkm::worklet::DispatcherMapTopology<NormalsWorkletPass1>(pass1).Invoke(
      *cellset, *cellset, coordinates, marchingcubes::make_ScalarField(*field), *normals);

    NormalsWorkletPass2 pass2(*edges);
    vtkm::worklet::DispatcherMapTopology<NormalsWorkletPass2>(pass2).Invoke(
      *cellset, *cellset, coordinates, marchingcubes::make_ScalarField(*field), *weights, *normals);
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
class MarchingCubes
{
public:
  //----------------------------------------------------------------------------
  MarchingCubes(bool mergeDuplicates = true)
    : MergeDuplicatePoints(mergeDuplicates)
    , EdgeTable()
    , NumTrianglesTable()
    , TriangleTable()
    , InterpolationWeights()
    , InterpolationEdgeIds()
  {
    // Set up the Marching Cubes case tables as part of the filter so that
    // we cache these tables in the execution environment between execution runs
    this->EdgeTable = vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::edgeTable, 24);

    this->NumTrianglesTable =
      vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::numTrianglesTable, 256);

    this->TriangleTable = vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::triTable, 256 * 16);
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
            typename StorageTagVertices,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(
    const ValueType* const isovalues,
    const vtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    const DeviceAdapter& device)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>> normals;
    return this->DeduceRun(
      isovalues, numIsoValues, cells, coordinateSystem, input, vertices, normals, false, device);
  }

  //----------------------------------------------------------------------------
  template <typename ValueType,
            typename CellSetType,
            typename CoordinateSystem,
            typename StorageTagField,
            typename CoordinateType,
            typename StorageTagVertices,
            typename StorageTagNormals,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(
    const ValueType* const isovalues,
    const vtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagNormals> normals,
    const DeviceAdapter& device)
  {
    return this->DeduceRun(
      isovalues, numIsoValues, cells, coordinateSystem, input, vertices, normals, true, device);
  }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType, typename DeviceAdapter>
  vtkm::cont::ArrayHandle<ValueType> ProcessPointField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& input,
    const DeviceAdapter&) const
  {
    using vtkm::worklet::marchingcubes::MapPointField;
    MapPointField applyToField;
    vtkm::worklet::DispatcherMapField<MapPointField, DeviceAdapter> applyFieldDispatcher(
      applyToField);

    vtkm::cont::ArrayHandle<ValueType> output;
    applyFieldDispatcher.Invoke(
      this->InterpolationEdgeIds, this->InterpolationWeights, input, output);
    return output;
  }

  //----------------------------------------------------------------------------
  template <typename ValueType, typename StorageType, typename DeviceAdapter>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& in,
    const DeviceAdapter&) const
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->CellIdMap, in);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    Algo::Copy(tmp, result);

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
            typename NormalType,
            typename DeviceAdapter>
  struct DeduceCellType
  {
    MarchingCubes* MC = nullptr;
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
                                        *withNormals,
                                        DeviceAdapter());
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
            typename NormalType,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> DeduceRun(
    const ValueType* isovalues,
    const vtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec<NormalType, 3>, StorageTagNormals> normals,
    bool withNormals,
    const DeviceAdapter&)
  {
    vtkm::cont::CellSetSingleType<> outputCells("contour");

    DeduceCellType<ValueType,
                   CoordinateSystem,
                   StorageTagField,
                   StorageTagVertices,
                   StorageTagNormals,
                   CoordinateType,
                   NormalType,
                   DeviceAdapter>
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
            typename NormalType,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> DoRun(
    const ValueType* isovalues,
    const vtkm::Id numIsoValues,
    const CellSetType& cells,
    const CoordinateSystem& coordinateSystem,
    const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
    vtkm::cont::ArrayHandle<vtkm::Vec<CoordinateType, 3>, StorageTagVertices> vertices,
    vtkm::cont::ArrayHandle<vtkm::Vec<NormalType, 3>, StorageTagNormals> normals,
    bool withNormals,
    const DeviceAdapter&)
  {
    using vtkm::worklet::marchingcubes::MapPointField;
    using vtkm::worklet::marchingcubes::EdgeWeightGenerate;
    using vtkm::worklet::marchingcubes::EdgeWeightGenerateMetaData;
    using vtkm::worklet::marchingcubes::ClassifyCell;

    // Setup the Dispatcher Typedefs
    using ClassifyDispatcher =
      typename vtkm::worklet::DispatcherMapTopology<ClassifyCell<ValueType>, DeviceAdapter>;

    using GenerateDispatcher =
      typename vtkm::worklet::DispatcherMapTopology<EdgeWeightGenerate<ValueType, DeviceAdapter>,
                                                    DeviceAdapter>;

    vtkm::cont::ArrayHandle<ValueType> isoValuesHandle =
      vtkm::cont::make_ArrayHandle(isovalues, numIsoValues);
    // Call the ClassifyCell functor to compute the Marching Cubes case numbers
    // for each cell, and the number of vertices to be generated

    vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutputTrisPerCell;

    {
      ClassifyCell<ValueType> classifyCell;
      ClassifyDispatcher classifyCellDispatcher(classifyCell);
      classifyCellDispatcher.Invoke(
        isoValuesHandle, inputField, cells, numOutputTrisPerCell, this->NumTrianglesTable);
    }

    //Pass 2 Generate the edges
    vtkm::cont::ArrayHandle<vtkm::UInt8> contourIds;
    vtkm::cont::ArrayHandle<vtkm::Id> originalCellIdsForPoints;
    {
      vtkm::worklet::ScatterCounting scatter(numOutputTrisPerCell, DeviceAdapter());

      // Maps output cells to input cells. Store this for cell field mapping.
      this->CellIdMap = scatter.GetOutputToInputMap();

      EdgeWeightGenerateMetaData<DeviceAdapter> metaData(
        scatter.GetOutputRange(numOutputTrisPerCell.GetNumberOfValues()),
        this->InterpolationWeights,
        this->InterpolationEdgeIds,
        originalCellIdsForPoints,
        contourIds,
        this->EdgeTable,
        this->NumTrianglesTable,
        this->TriangleTable,
        scatter);

      EdgeWeightGenerate<ValueType, DeviceAdapter> weightGenerate(metaData);
      GenerateDispatcher edgeDispatcher(weightGenerate);
      edgeDispatcher.Invoke(
        cells,
        //cast to a scalar field if not one, as cellderivative only works on those
        isoValuesHandle,
        inputField);
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
        marchingcubes::MergeDuplicates(this->InterpolationEdgeIds, //keys
                                       this->InterpolationWeights, //values
                                       this->InterpolationEdgeIds, //values
                                       originalCellIdsForPoints,   //values
                                       connectivity,               // computed using lower bounds
                                       DeviceAdapter());
      }
      else if (numIsoValues > 1)
      {
        marchingcubes::MergeDuplicates(
          vtkm::cont::make_ArrayHandleZip(contourIds, this->InterpolationEdgeIds), //keys
          this->InterpolationWeights,                                              //values
          this->InterpolationEdgeIds,                                              //values
          originalCellIdsForPoints,                                                //values
          connectivity, // computed using lower bounds
          DeviceAdapter());
      }
    }
    else
    {
      //when we don't merge points, the connectivity array can be represented
      //by a counting array. The danger of doing it this way is that the output
      //type is unknown. That is why we copy it into an explicit array
      using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
      vtkm::cont::ArrayHandleIndex temp(this->InterpolationEdgeIds.GetNumberOfValues());
      Algorithm::Copy(temp, connectivity);
    }

    //generate the vertices's
    MapPointField applyToField;
    vtkm::worklet::DispatcherMapField<MapPointField, DeviceAdapter> applyFieldDispatcher(
      applyToField);

    applyFieldDispatcher.Invoke(
      this->InterpolationEdgeIds, this->InterpolationWeights, coordinateSystem, vertices);

    //assign the connectivity to the cell set
    vtkm::cont::CellSetSingleType<> outputCells("contour");
    outputCells.Fill(vertices.GetNumberOfValues(), vtkm::CELL_SHAPE_TRIANGLE, 3, connectivity);

    //now that the vertices have been generated we can generate the normals
    if (withNormals)
    {
      marchingcubes::GenerateNormals(normals,
                                     inputField,
                                     cells,
                                     coordinateSystem,
                                     this->InterpolationEdgeIds,
                                     this->InterpolationWeights);
    }

    return outputCells;
  }

  bool MergeDuplicatePoints;

  vtkm::cont::ArrayHandle<vtkm::IdComponent> EdgeTable;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumTrianglesTable;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> TriangleTable;

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> InterpolationWeights;
  vtkm::cont::ArrayHandle<vtkm::Id2> InterpolationEdgeIds;

  vtkm::cont::ArrayHandle<vtkm::Id> CellIdMap;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_MarchingCubes_h
