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
#include <vtkm/exec/CellEdge.h>
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
#include <vtkm/filter/contour/worklet/contour/FieldPropagation.h>
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
template <vtkm::UInt8 InCellDim>
struct OutCellTraits;

template <>
struct OutCellTraits<3>
{
  static constexpr vtkm::UInt8 NUM_POINTS = 3;
  static constexpr vtkm::UInt8 CELL_SHAPE = vtkm::CELL_SHAPE_TRIANGLE;
};

template <>
struct OutCellTraits<2>
{
  static constexpr vtkm::UInt8 NUM_POINTS = 2;
  static constexpr vtkm::UInt8 CELL_SHAPE = vtkm::CELL_SHAPE_LINE;
};

template <>
struct OutCellTraits<1>
{
  static constexpr vtkm::UInt8 NUM_POINTS = 1;
  static constexpr vtkm::UInt8 CELL_SHAPE = vtkm::CELL_SHAPE_VERTEX;
};

template <vtkm::UInt8 Dims, typename FieldType, typename FieldVecType>
VTKM_EXEC vtkm::IdComponent TableNumOutCells(vtkm::UInt8 shape,
                                             FieldType isoValue,
                                             const FieldVecType& fieldIn)
{
  const vtkm::IdComponent numPoints = fieldIn.GetNumberOfComponents();
  // Compute the Marching Cubes case number for this cell. We need to iterate
  // the isovalues until the sum >= our visit index. But we need to make
  // sure the caseNumber is correct before stopping
  vtkm::IdComponent caseNumber = 0;
  for (vtkm::IdComponent point = 0; point < numPoints; ++point)
  {
    caseNumber |= (fieldIn[point] > isoValue) << point;
  }

  return vtkm::worklet::marching_cells::GetNumOutCells<Dims>(shape, caseNumber);
}

template <typename FieldType, typename FieldVecType>
VTKM_EXEC vtkm::IdComponent NumOutCellsSpecialCases(std::integral_constant<vtkm::UInt8, 3>,
                                                    vtkm::UInt8 shape,
                                                    FieldType isoValue,
                                                    const FieldVecType& fieldIn)
{
  return TableNumOutCells<3>(shape, isoValue, fieldIn);
}

template <typename FieldType, typename FieldVecType>
VTKM_EXEC vtkm::IdComponent NumOutCellsSpecialCases(std::integral_constant<vtkm::UInt8, 2>,
                                                    vtkm::UInt8 shape,
                                                    FieldType isoValue,
                                                    const FieldVecType& fieldIn)
{
  if (shape == vtkm::CELL_SHAPE_POLYGON)
  {
    const vtkm::IdComponent numPoints = fieldIn.GetNumberOfComponents();
    vtkm::IdComponent numCrossings = 0;
    bool lastOver = (fieldIn[numPoints - 1] > isoValue);
    for (vtkm::IdComponent point = 0; point < numPoints; ++point)
    {
      bool nextOver = (fieldIn[point] > isoValue);
      if (lastOver != nextOver)
      {
        ++numCrossings;
      }
      lastOver = nextOver;
    }
    VTKM_ASSERT((numCrossings % 2) == 0);
    return numCrossings / 2;
  }
  else
  {
    return TableNumOutCells<2>(shape, isoValue, fieldIn);
  }
}

template <typename FieldType, typename FieldVecType>
VTKM_EXEC vtkm::IdComponent NumOutCellsSpecialCases(std::integral_constant<vtkm::UInt8, 1>,
                                                    vtkm::UInt8 shape,
                                                    FieldType isoValue,
                                                    const FieldVecType& fieldIn)
{
  if ((shape == vtkm::CELL_SHAPE_LINE) || (shape == vtkm::CELL_SHAPE_POLY_LINE))
  {
    const vtkm::IdComponent numPoints = fieldIn.GetNumberOfComponents();
    vtkm::IdComponent numCrossings = 0;
    bool lastOver = (fieldIn[0] > isoValue);
    for (vtkm::IdComponent point = 1; point < numPoints; ++point)
    {
      bool nextOver = (fieldIn[point] > isoValue);
      if (lastOver != nextOver)
      {
        ++numCrossings;
      }
      lastOver = nextOver;
    }
    return numCrossings;
  }
  else
  {
    return 0;
  }
}

// ---------------------------------------------------------------------------
template <vtkm::UInt8 Dims, typename T>
class ClassifyCell : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(WholeArrayIn isovalues,
                                FieldInPoint fieldIn,
                                CellSetIn cellSet,
                                FieldOutCell outNumTriangles);
  using ExecutionSignature = void(CellShape, _1, _2, _4);
  using InputDomain = _3;

  template <typename CellShapeType, typename IsoValuesType, typename FieldInType>
  VTKM_EXEC void operator()(CellShapeType shape,
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn,
                            vtkm::IdComponent& numTriangles) const
  {
    vtkm::IdComponent sum = 0;
    vtkm::IdComponent numIsoValues = static_cast<vtkm::IdComponent>(isovalues.GetNumberOfValues());

    for (vtkm::Id i = 0; i < numIsoValues; ++i)
    {
      sum += NumOutCellsSpecialCases(
        std::integral_constant<vtkm::UInt8, Dims>{}, shape.Id, isovalues.Get(i), fieldIn);
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
    ExecObject(vtkm::UInt8 numPointsPerOutCell,
               vtkm::Id size,
               vtkm::cont::ArrayHandle<vtkm::FloatDefault>& interpWeights,
               vtkm::cont::ArrayHandle<vtkm::Id2>& interpIds,
               vtkm::cont::ArrayHandle<vtkm::Id>& interpCellIds,
               vtkm::cont::ArrayHandle<vtkm::UInt8>& interpContourId,
               vtkm::cont::DeviceAdapterId device,
               vtkm::cont::Token& token)
      : InterpWeightsPortal(
          interpWeights.PrepareForOutput(numPointsPerOutCell * size, device, token))
      , InterpIdPortal(interpIds.PrepareForOutput(numPointsPerOutCell * size, device, token))
      , InterpCellIdPortal(
          interpCellIds.PrepareForOutput(numPointsPerOutCell * size, device, token))
      , InterpContourPortal(
          interpContourId.PrepareForOutput(numPointsPerOutCell * size, device, token))
    {
      // Interp needs to be scaled as they are per point of the output cell
    }
    WritePortalType<vtkm::FloatDefault> InterpWeightsPortal;
    WritePortalType<vtkm::Id2> InterpIdPortal;
    WritePortalType<vtkm::Id> InterpCellIdPortal;
    WritePortalType<vtkm::UInt8> InterpContourPortal;
  };

  VTKM_CONT
  EdgeWeightGenerateMetaData(vtkm::UInt8 inCellDimension,
                             vtkm::Id size,
                             vtkm::cont::ArrayHandle<vtkm::FloatDefault>& interpWeights,
                             vtkm::cont::ArrayHandle<vtkm::Id2>& interpIds,
                             vtkm::cont::ArrayHandle<vtkm::Id>& interpCellIds,
                             vtkm::cont::ArrayHandle<vtkm::UInt8>& interpContourId)
    : NumPointsPerOutCell(inCellDimension)
    , Size(size)
    , InterpWeights(interpWeights)
    , InterpIds(interpIds)
    , InterpCellIds(interpCellIds)
    , InterpContourId(interpContourId)
  {
  }

  VTKM_CONT ExecObject PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                           vtkm::cont::Token& token)
  {
    return ExecObject(this->NumPointsPerOutCell,
                      this->Size,
                      this->InterpWeights,
                      this->InterpIds,
                      this->InterpCellIds,
                      this->InterpContourId,
                      device,
                      token);
  }

private:
  vtkm::UInt8 NumPointsPerOutCell;
  vtkm::Id Size;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> InterpWeights;
  vtkm::cont::ArrayHandle<vtkm::Id2> InterpIds;
  vtkm::cont::ArrayHandle<vtkm::Id> InterpCellIds;
  vtkm::cont::ArrayHandle<vtkm::UInt8> InterpContourId;
};

// -----------------------------------------------------------------------------
template <vtkm::UInt8 Dims, typename IsoValuesType, typename FieldVecType>
VTKM_EXEC const vtkm::UInt8* TableCellEdges(vtkm::UInt8 shape,
                                            const IsoValuesType& isoValues,
                                            const FieldVecType& fieldIn,
                                            vtkm::IdComponent visitIndex,
                                            vtkm::IdComponent& contourIndex)
{
  const vtkm::IdComponent numPoints = fieldIn.GetNumberOfComponents();
  // Compute the Marching Cubes case number for this cell. We need to iterate
  // the isovalues until the sum >= our visit index. But we need to make
  // sure the caseNumber is correct before stopping
  vtkm::IdComponent caseNumber = 0;
  vtkm::IdComponent sum = 0;
  vtkm::IdComponent numIsoValues = static_cast<vtkm::IdComponent>(isoValues.GetNumberOfValues());

  for (contourIndex = 0; contourIndex < numIsoValues; ++contourIndex)
  {
    const auto value = isoValues.Get(contourIndex);
    caseNumber = 0;
    for (vtkm::IdComponent point = 0; point < numPoints; ++point)
    {
      caseNumber |= (fieldIn[point] > value) << point;
    }

    sum += vtkm::worklet::marching_cells::GetNumOutCells<Dims>(shape, caseNumber);
    if (sum > visitIndex)
    {
      break;
    }
  }

  VTKM_ASSERT(contourIndex < numIsoValues);

  visitIndex = sum - visitIndex - 1;

  return vtkm::worklet::marching_cells::GetCellEdges<Dims>(shape, caseNumber, visitIndex);
}

template <typename IsoValuesType, typename FieldVecType>
VTKM_EXEC const vtkm::UInt8* CellEdgesSpecialCases(std::integral_constant<vtkm::UInt8, 3>,
                                                   vtkm::UInt8 shape,
                                                   const IsoValuesType& isoValues,
                                                   const FieldVecType& fieldIn,
                                                   vtkm::IdComponent visitIndex,
                                                   vtkm::IdComponent& contourIndex,
                                                   vtkm::Vec2ui_8& vtkmNotUsed(edgeBuffer))
{
  return TableCellEdges<3>(shape, isoValues, fieldIn, visitIndex, contourIndex);
}

template <typename IsoValuesType, typename FieldVecType>
VTKM_EXEC const vtkm::UInt8* CellEdgesSpecialCases(std::integral_constant<vtkm::UInt8, 2>,
                                                   vtkm::UInt8 shape,
                                                   const IsoValuesType& isoValues,
                                                   const FieldVecType& fieldIn,
                                                   vtkm::IdComponent visitIndex,
                                                   vtkm::IdComponent& contourIndex,
                                                   vtkm::Vec2ui_8& edgeBuffer)
{
  if (shape == vtkm::CELL_SHAPE_POLYGON)
  {
    vtkm::IdComponent numCrossings = 0;
    vtkm::IdComponent numIsoValues = static_cast<vtkm::IdComponent>(isoValues.GetNumberOfValues());
    const vtkm::IdComponent numPoints = fieldIn.GetNumberOfComponents();
    for (contourIndex = 0; contourIndex < numIsoValues; ++contourIndex)
    {
      auto isoValue = isoValues.Get(contourIndex);
      bool lastOver = (fieldIn[0] > isoValue);
      for (vtkm::IdComponent point = 1; point <= numPoints; ++point)
      {
        bool nextOver = (fieldIn[point % numPoints] > isoValue);
        if (lastOver != nextOver)
        {
          // Check to see if we hit the target edge.
          if (visitIndex == (numCrossings / 2))
          {
            if ((numCrossings % 2) == 0)
            {
              // Record first point.
              edgeBuffer[0] = point - 1;
            }
            else
            {
              // Record second (and final) point.
              edgeBuffer[1] = point - 1;
              return &edgeBuffer[0];
            }
          }
          ++numCrossings;
        }
        lastOver = nextOver;
      }
      VTKM_ASSERT((numCrossings % 2) == 0);
    }
    VTKM_ASSERT(0 && "Sanity check fail.");
    edgeBuffer[0] = edgeBuffer[1] = 0;
    return &edgeBuffer[0];
  }
  else
  {
    return TableCellEdges<2>(shape, isoValues, fieldIn, visitIndex, contourIndex);
  }
}

template <typename IsoValuesType, typename FieldVecType>
VTKM_EXEC const vtkm::UInt8* CellEdgesSpecialCases(std::integral_constant<vtkm::UInt8, 1>,
                                                   vtkm::UInt8 shape,
                                                   const IsoValuesType& isoValues,
                                                   const FieldVecType& fieldIn,
                                                   vtkm::IdComponent visitIndex,
                                                   vtkm::IdComponent& contourIndex,
                                                   vtkm::Vec2ui_8& edgeBuffer)
{
  VTKM_ASSERT((shape == vtkm::CELL_SHAPE_LINE) || (shape == vtkm::CELL_SHAPE_POLY_LINE));
  (void)shape;
  vtkm::IdComponent numCrossings = 0;
  vtkm::IdComponent numIsoValues = static_cast<vtkm::IdComponent>(isoValues.GetNumberOfValues());
  const vtkm::IdComponent numPoints = fieldIn.GetNumberOfComponents();
  for (contourIndex = 0; contourIndex < numIsoValues; ++contourIndex)
  {
    auto isoValue = isoValues.Get(contourIndex);
    bool lastOver = (fieldIn[0] > isoValue);
    for (vtkm::IdComponent point = 1; point < numPoints; ++point)
    {
      bool nextOver = (fieldIn[point] > isoValue);
      if (lastOver != nextOver)
      {
        if (visitIndex == numCrossings)
        {
          edgeBuffer[0] = point - 1;
          return &edgeBuffer[0];
        }
        ++numCrossings;
      }
      lastOver = nextOver;
    }
  }
  VTKM_ASSERT(0 && "Sanity check fail.");
  edgeBuffer[0] = 0;
  return &edgeBuffer[0];
}

/// \brief Compute the weights for each edge that is used to generate
/// a point in the resulting iso-surface
// -----------------------------------------------------------------------------
template <vtkm::UInt8 Dims>
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
                                ExecObject metaData); // Metadata for edge weight generation
  using ExecutionSignature =
    void(CellShape, PointCount, _2, _3, _4, InputIndex, WorkIndex, VisitIndex, PointIndices);

  using InputDomain = _1;

  template <typename CellShape,
            typename IsoValuesType,
            typename FieldInType, // Vec-like, one per input point
            typename IndicesVecType>
  VTKM_EXEC void operator()(const CellShape shape,
                            vtkm::IdComponent numVertices,
                            const IsoValuesType& isovalues,
                            const FieldInType& fieldIn, // Input point field defining the contour
                            const EdgeWeightGenerateMetaData::ExecObject& metaData,
                            vtkm::Id inputCellId,
                            vtkm::Id outputCellId,
                            vtkm::IdComponent visitIndex,
                            const IndicesVecType& indices) const
  {
    const vtkm::Id outputPointId = OutCellTraits<Dims>::NUM_POINTS * outputCellId;
    using FieldType = typename vtkm::VecTraits<FieldInType>::ComponentType;

    // Interpolate for vertex positions and associated scalar values
    vtkm::IdComponent contourIndex;
    vtkm::Vec2ui_8 edgeBuffer;
    const vtkm::UInt8* edges = CellEdgesSpecialCases(std::integral_constant<vtkm::UInt8, Dims>{},
                                                     shape.Id,
                                                     isovalues,
                                                     fieldIn,
                                                     visitIndex,
                                                     contourIndex,
                                                     edgeBuffer);
    for (vtkm::IdComponent triVertex = 0; triVertex < OutCellTraits<Dims>::NUM_POINTS; triVertex++)
    {
      vtkm::IdComponent2 edgeVertices;
      vtkm::Vec<FieldType, 2> fieldValues;
      for (vtkm::IdComponent edgePointId = 0; edgePointId < 2; ++edgePointId)
      {
        vtkm::ErrorCode errorCode = this->CrossingLocalIndex(
          numVertices, edgePointId, edges[triVertex], shape, edgeVertices[edgePointId]);
        if (errorCode != vtkm::ErrorCode::Success)
        {
          this->RaiseError(vtkm::ErrorString(errorCode));
          return;
        }
        fieldValues[edgePointId] = fieldIn[edgeVertices[edgePointId]];
      }

      // Store the input cell id so that we can properly generate the normals
      // in a subsequent call, after we have merged duplicate points
      metaData.InterpCellIdPortal.Set(outputPointId + triVertex, inputCellId);

      metaData.InterpContourPortal.Set(outputPointId + triVertex,
                                       static_cast<vtkm::UInt8>(contourIndex));

      metaData.InterpIdPortal.Set(outputPointId + triVertex,
                                  vtkm::Id2(indices[edgeVertices[0]], indices[edgeVertices[1]]));

      vtkm::FloatDefault interpolant =
        static_cast<vtkm::FloatDefault>(isovalues.Get(contourIndex) - fieldValues[0]) /
        static_cast<vtkm::FloatDefault>(fieldValues[1] - fieldValues[0]);

      metaData.InterpWeightsPortal.Set(outputPointId + triVertex, interpolant);
    }
  }

  template <typename CellShapeTag>
  static inline VTKM_EXEC vtkm::ErrorCode CrossingLocalIndex(vtkm::IdComponent numPoints,
                                                             vtkm::IdComponent pointIndex,
                                                             vtkm::IdComponent edgeIndex,
                                                             CellShapeTag shape,
                                                             vtkm::IdComponent& result);
};

template <>
template <typename CellShapeTag>
VTKM_EXEC vtkm::ErrorCode EdgeWeightGenerate<1>::CrossingLocalIndex(vtkm::IdComponent numPoints,
                                                                    vtkm::IdComponent pointIndex,
                                                                    vtkm::IdComponent edgeIndex,
                                                                    CellShapeTag shape,
                                                                    vtkm::IdComponent& result)
{
  VTKM_ASSERT((shape.Id == vtkm::CELL_SHAPE_LINE) || (shape.Id == vtkm::CELL_SHAPE_POLY_LINE));
  (void)shape;
  if ((pointIndex < 0) || (pointIndex > 1))
  {
    result = -1;
    return vtkm::ErrorCode::InvalidPointId;
  }
  if ((edgeIndex < 0) || (edgeIndex >= (numPoints - 1)))
  {
    result = -1;
    return vtkm::ErrorCode::InvalidEdgeId;
  }
  result = edgeIndex + pointIndex;
  return vtkm::ErrorCode::Success;
}

template <vtkm::UInt8 Dims>
template <typename CellShapeTag>
VTKM_EXEC vtkm::ErrorCode EdgeWeightGenerate<Dims>::CrossingLocalIndex(vtkm::IdComponent numPoints,
                                                                       vtkm::IdComponent pointIndex,
                                                                       vtkm::IdComponent edgeIndex,
                                                                       CellShapeTag shape,
                                                                       vtkm::IdComponent& result)
{
  return vtkm::exec::CellEdgeLocalIndex(numPoints, pointIndex, edgeIndex, shape, result);
}

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
    vtkm::exec::FieldNeighborhood<WholeCoordinatesIn> points(pointCoordinates, boundary);
    vtkm::exec::FieldNeighborhood<WholeFieldIn> field(inputField, boundary);

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
    vtkm::exec::FieldNeighborhood<WholeCoordinatesIn> points(pointCoordinates, boundary);
    vtkm::exec::FieldNeighborhood<WholeFieldIn> field(inputField, boundary);

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
template <vtkm::UInt8 Dims,
          typename CellSetType,
          typename CoordinateSystem,
          typename ValueType,
          typename StorageTagField>
vtkm::cont::CellSetSingleType<> execute(
  const CellSetType& cells,
  const CoordinateSystem& coordinateSystem,
  const std::vector<ValueType>& isovalues,
  const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& vertices,
  vtkm::cont::ArrayHandle<vtkm::Vec3f>& normals,
  vtkm::worklet::contour::CommonState& sharedState)
{
  using vtkm::worklet::contour::MapPointField;
  using vtkm::worklet::marching_cells::ClassifyCell;
  using vtkm::worklet::marching_cells::EdgeWeightGenerate;
  using vtkm::worklet::marching_cells::EdgeWeightGenerateMetaData;

  // Setup the invoker
  vtkm::cont::Invoker invoker;

  vtkm::cont::ArrayHandle<ValueType> isoValuesHandle =
    vtkm::cont::make_ArrayHandle(isovalues, vtkm::CopyFlag::Off);

  // Call the ClassifyCell functor to compute the Marching Cubes case numbers
  // for each cell, and the number of vertices to be generated
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutputTrisPerCell;
  {
    marching_cells::ClassifyCell<Dims, ValueType> classifyCell;
    invoker(classifyCell, isoValuesHandle, inputField, cells, numOutputTrisPerCell);
  }

  //Pass 2 Generate the edges
  vtkm::cont::ArrayHandle<vtkm::UInt8> contourIds;
  vtkm::cont::ArrayHandle<vtkm::Id> originalCellIdsForPoints;
  {
    auto scatter = EdgeWeightGenerate<Dims>::MakeScatter(numOutputTrisPerCell);

    // Maps output cells to input cells. Store this for cell field mapping.
    sharedState.CellIdMap = scatter.GetOutputToInputMap();

    EdgeWeightGenerateMetaData metaData(
      Dims,
      scatter.GetOutputRange(numOutputTrisPerCell.GetNumberOfValues()),
      sharedState.InterpolationWeights,
      sharedState.InterpolationEdgeIds,
      originalCellIdsForPoints,
      contourIds);

    invoker(EdgeWeightGenerate<Dims>{},
            scatter,
            cells,
            //cast to a scalar field if not one, as cellderivative only works on those
            isoValuesHandle,
            inputField,
            metaData);
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
  outputCells.Fill(vertices.GetNumberOfValues(),
                   OutCellTraits<Dims>::CELL_SHAPE,
                   OutCellTraits<Dims>::NUM_POINTS,
                   connectivity);

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
