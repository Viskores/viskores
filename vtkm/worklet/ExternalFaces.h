//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_ExternalFaces_h
#define vtk_m_worklet_ExternalFaces_h

#include <vtkm/CellShape.h>
#include <vtkm/Math.h>

#include <vtkm/exec/CellFace.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConcatenate.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

// #define __VTKM_EXTERNAL_FACES_BENCHMARK

namespace vtkm
{
namespace worklet
{

struct ExternalFaces
{
  vtkm::cont::ArrayHandle<vtkm::Id> CellIdMap;
  bool PassPolyData;

  //Unary predicate operator
  //Returns True if the argument is equal to 1; False otherwise.
  struct IsUnity
  {
    template <typename T>
    VTKM_EXEC_CONT bool operator()(const T& x) const
    {
      return x == T(1);
    }
  };

  //Returns True if the first vector argument is less than the second
  //vector argument; otherwise, False
  struct Id3LessThan
  {
    template <typename T>
    VTKM_EXEC_CONT bool operator()(const vtkm::Vec<T, 3>& a, const vtkm::Vec<T, 3>& b) const
    {
      bool isLessThan = false;
      if (a[0] < b[0])
      {
        isLessThan = true;
      }
      else if (a[0] == b[0])
      {
        if (a[1] < b[1])
        {
          isLessThan = true;
        }
        else if (a[1] == b[1])
        {
          if (a[2] < b[2])
          {
            isLessThan = true;
          }
        }
      }
      return isLessThan;
    }
  };

  //Worklet that returns the number of external faces for each structured cell
  class NumExternalFacesPerStructuredCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn inCellSet,
                                  FieldOut<> numFacesInCell,
                                  FieldInPoint<Vec3> pointCoordinates);
    typedef _2 ExecutionSignature(CellShape, _3);
    typedef _1 InputDomain;

    VTKM_CONT
    NumExternalFacesPerStructuredCell(const vtkm::Vec<vtkm::Float32, 3>& min_point,
                                      const vtkm::Vec<vtkm::Float32, 3>& max_point)
      : MinPoint(min_point)
      , MaxPoint(max_point)
    {
    }

    VTKM_EXEC
    inline vtkm::IdComponent CountExternalFacesOnDimension(vtkm::Float32 grid_min,
                                                           vtkm::Float32 grid_max,
                                                           vtkm::Float32 cell_min,
                                                           vtkm::Float32 cell_max) const
    {
      vtkm::IdComponent count = 0;

      bool cell_min_at_grid_boundary = cell_min <= grid_min;
      bool cell_max_at_grid_boundary = cell_max >= grid_max;

      if (cell_min_at_grid_boundary && !cell_max_at_grid_boundary)
        count++;
      else if (!cell_min_at_grid_boundary && cell_max_at_grid_boundary)
        count++;
      else if (cell_min_at_grid_boundary && cell_max_at_grid_boundary)
        count += 2;

      return count;
    }

    template <typename CellShapeTag, typename PointCoordVecType>
    VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag shape,
                                           const PointCoordVecType& pointCoordinates) const
    {
      VTKM_ASSERT(shape.Id == CELL_SHAPE_HEXAHEDRON);

      vtkm::IdComponent count = 0;

      count += CountExternalFacesOnDimension(
        MinPoint[0], MaxPoint[0], pointCoordinates[0][0], pointCoordinates[1][0]);

      count += CountExternalFacesOnDimension(
        MinPoint[1], MaxPoint[1], pointCoordinates[0][1], pointCoordinates[3][1]);

      count += CountExternalFacesOnDimension(
        MinPoint[2], MaxPoint[2], pointCoordinates[0][2], pointCoordinates[4][2]);

      return count;
    }

  private:
    vtkm::Vec<vtkm::Float32, 3> MinPoint;
    vtkm::Vec<vtkm::Float32, 3> MaxPoint;
  };


  //Worklet that finds face connectivity for each structured cell
  class BuildConnectivityStructured : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn inCellSet,
                                  WholeCellSetIn<> inputCell,
                                  FieldOut<> faceShapes,
                                  FieldOut<> facePointCount,
                                  FieldOut<> faceConnectivity,
                                  FieldInPoint<Vec3> pointCoordinates);
    typedef void ExecutionSignature(CellShape, VisitIndex, InputIndex, _2, _3, _4, _5, _6);
    typedef _1 InputDomain;

    using ScatterType = vtkm::worklet::ScatterCounting;

    VTKM_CONT
    ScatterType GetScatter() const { return this->Scatter; }

    template <typename CountArrayType, typename Device>
    VTKM_CONT BuildConnectivityStructured(const vtkm::Vec<vtkm::Float32, 3>& min_point,
                                          const vtkm::Vec<vtkm::Float32, 3>& max_point,
                                          const CountArrayType& countArray,
                                          Device)
      : MinPoint(min_point)
      , MaxPoint(max_point)
      , Scatter(countArray, Device())
    {
      VTKM_IS_ARRAY_HANDLE(CountArrayType);
    }

    VTKM_CONT
    BuildConnectivityStructured(const vtkm::Vec<vtkm::Float32, 3>& min_point,
                                const vtkm::Vec<vtkm::Float32, 3>& max_point,
                                const ScatterType& scatter)
      : MinPoint(min_point)
      , MaxPoint(max_point)
      , Scatter(scatter)
    {
    }

    enum FaceType
    {
      FACE_GRID_MIN,
      FACE_GRID_MAX,
      FACE_GRID_MIN_AND_MAX,
      FACE_NONE
    };

    VTKM_EXEC
    inline bool FoundFaceOnDimension(vtkm::Float32 grid_min,
                                     vtkm::Float32 grid_max,
                                     vtkm::Float32 cell_min,
                                     vtkm::Float32 cell_max,
                                     vtkm::IdComponent& faceIndex,
                                     vtkm::IdComponent& count,
                                     vtkm::IdComponent dimensionFaceOffset,
                                     vtkm::IdComponent visitIndex) const
    {
      bool cell_min_at_grid_boundary = cell_min <= grid_min;
      bool cell_max_at_grid_boundary = cell_max >= grid_max;

      FaceType Faces = FaceType::FACE_NONE;

      if (cell_min_at_grid_boundary && !cell_max_at_grid_boundary)
        Faces = FaceType::FACE_GRID_MIN;
      else if (!cell_min_at_grid_boundary && cell_max_at_grid_boundary)
        Faces = FaceType::FACE_GRID_MAX;
      else if (cell_min_at_grid_boundary && cell_max_at_grid_boundary)
        Faces = FaceType::FACE_GRID_MIN_AND_MAX;

      if (Faces == FaceType::FACE_NONE)
        return false;

      if (Faces == FaceType::FACE_GRID_MIN)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset;
          return true;
        }
        else
          count++;
      }
      else if (Faces == FaceType::FACE_GRID_MAX)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset + 1;
          return true;
        }
        else
          count++;
      }
      else if (Faces == FaceType::FACE_GRID_MIN_AND_MAX)
      {
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset;
          return true;
        }
        count++;
        if (visitIndex == count)
        {
          faceIndex = dimensionFaceOffset + 1;
          return true;
        }
        count++;
      }

      return false;
    }

    template <typename PointCoordVecType>
    VTKM_EXEC inline vtkm::IdComponent FindFaceIndexForVisit(
      vtkm::IdComponent visitIndex,
      const PointCoordVecType& pointCoordinates) const
    {
      vtkm::IdComponent count = 0;
      vtkm::IdComponent faceIndex = 0;
      // Search X dimension
      if (!FoundFaceOnDimension(MinPoint[0],
                                MaxPoint[0],
                                pointCoordinates[0][0],
                                pointCoordinates[1][0],
                                faceIndex,
                                count,
                                0,
                                visitIndex))
      {
        // Search Y dimension
        if (!FoundFaceOnDimension(MinPoint[1],
                                  MaxPoint[1],
                                  pointCoordinates[0][1],
                                  pointCoordinates[3][1],
                                  faceIndex,
                                  count,
                                  2,
                                  visitIndex))
        {
          // Search Z dimension
          FoundFaceOnDimension(MinPoint[2],
                               MaxPoint[2],
                               pointCoordinates[0][2],
                               pointCoordinates[4][2],
                               faceIndex,
                               count,
                               4,
                               visitIndex);
        }
      }
      return faceIndex;
    }

    template <typename CellShapeTag,
              typename CellSetType,
              typename PointCoordVecType,
              typename ConnectivityType>
    VTKM_EXEC void operator()(CellShapeTag shape,
                              vtkm::IdComponent visitIndex,
                              vtkm::Id inputIndex,
                              const CellSetType& cellSet,
                              vtkm::UInt8& shapeOut,
                              vtkm::IdComponent& numFacePointsOut,
                              ConnectivityType& faceConnectivity,
                              const PointCoordVecType& pointCoordinates) const
    {
      VTKM_ASSERT(shape.Id == CELL_SHAPE_HEXAHEDRON);

      vtkm::IdComponent faceIndex = FindFaceIndexForVisit(visitIndex, pointCoordinates);

      vtkm::VecCConst<vtkm::IdComponent> localFaceIndices =
        vtkm::exec::CellFaceLocalIndices(faceIndex, shape, *this);
      vtkm::IdComponent numFacePoints = localFaceIndices.GetNumberOfComponents();
      VTKM_ASSERT(numFacePoints == faceConnectivity.GetNumberOfComponents());

      typename CellSetType::IndicesType inCellIndices = cellSet.GetIndices(inputIndex);

      shapeOut = vtkm::CELL_SHAPE_QUAD;
      numFacePointsOut = 4;

      for (vtkm::IdComponent facePointIndex = 0; facePointIndex < numFacePoints; facePointIndex++)
      {
        faceConnectivity[facePointIndex] = inCellIndices[localFaceIndices[facePointIndex]];
      }
    }

  private:
    vtkm::Vec<vtkm::Float32, 3> MinPoint;
    vtkm::Vec<vtkm::Float32, 3> MaxPoint;
    ScatterType Scatter;
  };

  //Worklet that returns the number of faces for each cell/shape
  class NumFacesPerCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn inCellSet, FieldOut<> numFacesInCell);
    typedef _2 ExecutionSignature(CellShape);
    typedef _1 InputDomain;

    template <typename CellShapeTag>
    VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag shape) const
    {
      return vtkm::exec::CellFaceNumberOfFaces(shape, *this);
    }
  };

  //Worklet that identifies a cell face by 3 cononical points
  class FaceHash : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldOut<> faceHashes,
                                  FieldOut<> originCells,
                                  FieldOut<> originFaces);
    typedef void ExecutionSignature(_2, _3, _4, CellShape, FromIndices, InputIndex, VisitIndex);
    typedef _1 InputDomain;

    using ScatterType = vtkm::worklet::ScatterCounting;

    VTKM_CONT
    ScatterType GetScatter() const { return this->Scatter; }

    template <typename CountArrayType, typename Device>
    VTKM_CONT FaceHash(const CountArrayType& countArray, Device)
      : Scatter(countArray, Device())
    {
      VTKM_IS_ARRAY_HANDLE(CountArrayType);
    }

    VTKM_CONT
    FaceHash(const ScatterType& scatter)
      : Scatter(scatter)
    {
    }

    template <typename CellShapeTag, typename CellNodeVecType>
    VTKM_EXEC void operator()(vtkm::Id3& faceHash,
                              vtkm::Id& cellIndex,
                              vtkm::IdComponent& faceIndex,
                              CellShapeTag shape,
                              const CellNodeVecType& cellNodeIds,
                              vtkm::Id inputIndex,
                              vtkm::IdComponent visitIndex) const
    {
      vtkm::VecCConst<vtkm::IdComponent> localFaceIndices =
        vtkm::exec::CellFaceLocalIndices(visitIndex, shape, *this);

      VTKM_ASSERT(localFaceIndices.GetNumberOfComponents() >= 3);

      //Assign cell points/nodes to this face
      vtkm::Id faceP1 = cellNodeIds[localFaceIndices[0]];
      vtkm::Id faceP2 = cellNodeIds[localFaceIndices[1]];
      vtkm::Id faceP3 = cellNodeIds[localFaceIndices[2]];

      //Sort the first 3 face points/nodes in ascending order
      vtkm::Id sorted[3] = { faceP1, faceP2, faceP3 };
      vtkm::Id temp;
      if (sorted[0] > sorted[2])
      {
        temp = sorted[0];
        sorted[0] = sorted[2];
        sorted[2] = temp;
      }
      if (sorted[0] > sorted[1])
      {
        temp = sorted[0];
        sorted[0] = sorted[1];
        sorted[1] = temp;
      }
      if (sorted[1] > sorted[2])
      {
        temp = sorted[1];
        sorted[1] = sorted[2];
        sorted[2] = temp;
      }

      // Check the rest of the points to see if they are in the lowest 3
      vtkm::IdComponent numPointsInFace = localFaceIndices.GetNumberOfComponents();
      for (vtkm::IdComponent pointIndex = 3; pointIndex < numPointsInFace; pointIndex++)
      {
        vtkm::Id nextPoint = cellNodeIds[localFaceIndices[pointIndex]];
        if (nextPoint < sorted[2])
        {
          if (nextPoint < sorted[1])
          {
            sorted[2] = sorted[1];
            if (nextPoint < sorted[0])
            {
              sorted[1] = sorted[0];
              sorted[0] = nextPoint;
            }
            else // nextPoint > P0, nextPoint < P1
            {
              sorted[1] = nextPoint;
            }
          }
          else // nextPoint > P1, nextPoint < P2
          {
            sorted[2] = nextPoint;
          }
        }
        else // nextPoint > P2
        {
          // Do nothing. nextPoint not in top 3.
        }
      }

      faceHash[0] = sorted[0];
      faceHash[1] = sorted[1];
      faceHash[2] = sorted[2];

      cellIndex = inputIndex;
      faceIndex = visitIndex;
    }

  private:
    ScatterType Scatter;
  };

  // Worklet that identifies the number of cells written out per face, which
  // is 1 for faces that belong to only one cell (external face) or 0 for
  // faces that belong to more than one cell (internal face).
  class FaceCounts : public vtkm::worklet::WorkletReduceByKey
  {
  public:
    typedef void ControlSignature(KeysIn keys, ReducedValuesOut<> numOutputCells);
    typedef _2 ExecutionSignature(ValueCount);
    using InputDomain = _1;

    VTKM_EXEC
    vtkm::IdComponent operator()(vtkm::IdComponent numCellsOnFace) const
    {
      if (numCellsOnFace == 1)
      {
        return 1;
      }
      else
      {
        return 0;
      }
    }
  };

  // Worklet that returns the number of points for each outputted face
  class NumPointsPerFace : public vtkm::worklet::WorkletReduceByKey
  {
  public:
    typedef void ControlSignature(KeysIn keys,
                                  WholeCellSetIn<> inputCells,
                                  ValuesIn<> originCells,
                                  ValuesIn<> originFaces,
                                  ReducedValuesOut<> numPointsInFace);
    typedef _5 ExecutionSignature(_2, _3, _4);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    VTKM_CONT
    ScatterType GetScatter() const { return this->Scatter; }

    template <typename CountArrayType, typename Device>
    VTKM_CONT NumPointsPerFace(const CountArrayType& countArray, Device)
      : Scatter(countArray, Device())
    {
      VTKM_IS_ARRAY_HANDLE(CountArrayType);
    }

    VTKM_CONT
    NumPointsPerFace(const ScatterType& scatter)
      : Scatter(scatter)
    {
    }

    template <typename CellSetType, typename OriginCellsType, typename OriginFacesType>
    VTKM_EXEC vtkm::IdComponent operator()(const CellSetType& cellSet,
                                           const OriginCellsType& originCells,
                                           const OriginFacesType& originFaces) const
    {
      VTKM_ASSERT(originCells.GetNumberOfComponents() == 1);
      VTKM_ASSERT(originFaces.GetNumberOfComponents() == 1);
      return vtkm::exec::CellFaceNumberOfPoints(
        originFaces[0], cellSet.GetCellShape(originCells[0]), *this);
    }

  private:
    ScatterType Scatter;
  };

  // Worklet that returns the shape and connectivity for each external face
  class BuildConnectivity : public vtkm::worklet::WorkletReduceByKey
  {
  public:
    typedef void ControlSignature(KeysIn keys,
                                  WholeCellSetIn<> inputCells,
                                  ValuesIn<> originCells,
                                  ValuesIn<> originFaces,
                                  ReducedValuesOut<> shapesOut,
                                  ReducedValuesOut<> connectivityOut,
                                  ReducedValuesOut<> cellIdMapOut);
    typedef void ExecutionSignature(_2, _3, _4, _5, _6, _7);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    VTKM_CONT
    ScatterType GetScatter() const { return this->Scatter; }

    template <typename CountArrayType, typename Device>
    VTKM_CONT BuildConnectivity(const CountArrayType& countArray, Device)
      : Scatter(countArray, Device())
    {
      VTKM_IS_ARRAY_HANDLE(CountArrayType);
    }

    VTKM_CONT
    BuildConnectivity(const ScatterType& scatter)
      : Scatter(scatter)
    {
    }

    template <typename CellSetType,
              typename OriginCellsType,
              typename OriginFacesType,
              typename ConnectivityType>
    VTKM_EXEC void operator()(const CellSetType& cellSet,
                              const OriginCellsType& originCells,
                              const OriginFacesType& originFaces,
                              vtkm::UInt8& shapeOut,
                              ConnectivityType& connectivityOut,
                              vtkm::Id& cellIdMapOut) const
    {
      VTKM_ASSERT(originCells.GetNumberOfComponents() == 1);
      VTKM_ASSERT(originFaces.GetNumberOfComponents() == 1);

      typename CellSetType::CellShapeTag shapeIn = cellSet.GetCellShape(originCells[0]);
      shapeOut = vtkm::exec::CellFaceShape(originFaces[0], shapeIn, *this);
      cellIdMapOut = originCells[0];

      vtkm::VecCConst<vtkm::IdComponent> localFaceIndices =
        vtkm::exec::CellFaceLocalIndices(originFaces[0], shapeIn, *this);
      vtkm::IdComponent numFacePoints = localFaceIndices.GetNumberOfComponents();
      VTKM_ASSERT(numFacePoints == connectivityOut.GetNumberOfComponents());

      typename CellSetType::IndicesType inCellIndices = cellSet.GetIndices(originCells[0]);

      for (vtkm::IdComponent facePointIndex = 0; facePointIndex < numFacePoints; facePointIndex++)
      {
        connectivityOut[facePointIndex] = inCellIndices[localFaceIndices[facePointIndex]];
      }
    }

  private:
    ScatterType Scatter;
  };

  class IsPolyDataCell : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn inCellSet, FieldOut<> isPolyDataCell);
    typedef _2 ExecutionSignature(CellShape);
    typedef _1 InputDomain;

    template <typename CellShapeTag>
    VTKM_EXEC vtkm::IdComponent operator()(CellShapeTag shape) const
    {
      return !vtkm::exec::CellFaceNumberOfFaces(shape, *this);
    }
  };

  class CountPolyDataCellPoints : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    using ScatterType = vtkm::worklet::ScatterCounting;

    VTKM_CONT
    ScatterType GetScatter() const { return this->Scatter; }

    template <typename CountArrayType, typename Device>
    VTKM_CONT CountPolyDataCellPoints(const CountArrayType& countArray, Device)
      : Scatter(countArray, Device())
    {
      VTKM_IS_ARRAY_HANDLE(CountArrayType);
    }

    VTKM_CONT
    CountPolyDataCellPoints(const ScatterType& scatter)
      : Scatter(scatter)
    {
    }

    typedef void ControlSignature(CellSetIn inCellSet, FieldOut<> numPoints);
    typedef _2 ExecutionSignature(PointCount);
    typedef _1 InputDomain;

    VTKM_EXEC vtkm::Id operator()(vtkm::Id count) const { return count; }
  private:
    ScatterType Scatter;
  };

  class PassPolyDataCells : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    using ScatterType = vtkm::worklet::ScatterCounting;

    VTKM_CONT
    ScatterType GetScatter() const { return this->Scatter; }

    template <typename CountArrayType, typename Device>
    VTKM_CONT PassPolyDataCells(const CountArrayType& countArray, Device)
      : Scatter(countArray, Device())
    {
      VTKM_IS_ARRAY_HANDLE(CountArrayType);
    }

    VTKM_CONT
    PassPolyDataCells(const ScatterType& scatter)
      : Scatter(scatter)
    {
    }

    typedef void ControlSignature(CellSetIn inputTopology,
                                  FieldOut<> shapes,
                                  FieldOut<> pointIndices,
                                  FieldOut<> cellIdMapOut);
    typedef void ExecutionSignature(CellShape, PointIndices, InputIndex, VisitIndex, _2, _3, _4);

    template <typename CellShape, typename InPointIndexType, typename OutPointIndexType>
    VTKM_EXEC void operator()(const CellShape& inShape,
                              const InPointIndexType& inPoints,
                              vtkm::Id inputIndex,
                              vtkm::IdComponent visitIndex,
                              vtkm::UInt8& outShape,
                              OutPointIndexType& outPoints,
                              vtkm::Id& cellIdMapOut) const
    {
      cellIdMapOut = inputIndex;
      outShape = inShape.Id;

      vtkm::IdComponent numPoints = inPoints.GetNumberOfComponents();
      VTKM_ASSERT(numPoints == outPoints.GetNumberOfComponents());
      for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
      {
        outPoints[pointIndex] = inPoints[pointIndex];
      }
    }

  private:
    ScatterType Scatter;
  };

  template <typename T>
  struct BiasFunctor
  {
    VTKM_EXEC_CONT
    BiasFunctor(T bias = T(0))
      : Bias(bias)
    {
    }

    VTKM_EXEC_CONT
    T operator()(T x) const { return x + this->Bias; }

    T Bias;
  };

public:
  VTKM_CONT
  ExternalFaces()
    : PassPolyData(true)
  {
  }

  VTKM_CONT
  void SetPassPolyData(bool flag) { this->PassPolyData = flag; }

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

  void ReleaseCellMapArrays() { this->CellIdMap.ReleaseResources(); }


  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry for regular grids.
  ///
  /// Faster Run() method for uniform and rectilinear grid types.
  /// Uses grid extents to find cells on the boundaries of the grid.
  template <typename ShapeStorage,
            typename NumIndicesStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage,
            typename DeviceAdapter>
  VTKM_CONT void Run(const vtkm::cont::CellSetStructured<3>& inCellSet,
                     const vtkm::cont::CoordinateSystem& coord,
                     vtkm::cont::CellSetExplicit<ShapeStorage,
                                                 NumIndicesStorage,
                                                 ConnectivityStorage,
                                                 OffsetsStorage>& outCellSet,
                     DeviceAdapter)
  {
    vtkm::Vec<vtkm::Float32, 3> MinPoint;
    vtkm::Vec<vtkm::Float32, 3> MaxPoint;
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                       vtkm::TopologyElementTagCell,
                                       3>
      Conn;

    Conn = inCellSet.PrepareForInput(
      DeviceAdapter(), vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    vtkm::Id3 PointDimensions = Conn.GetPointDimensions();
    typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> DefaultHandle;
    typedef vtkm::cont::ArrayHandleCartesianProduct<DefaultHandle, DefaultHandle, DefaultHandle>
      CartesianArrayHandle;

    if (coord.GetData().IsSameType(CartesianArrayHandle()))
    {
      CartesianArrayHandle vertices;
      vertices = coord.GetData().Cast<CartesianArrayHandle>();

      MinPoint[0] =
        static_cast<vtkm::Float32>(vertices.GetPortalConstControl().GetFirstPortal().Get(0));
      MinPoint[1] =
        static_cast<vtkm::Float32>(vertices.GetPortalConstControl().GetSecondPortal().Get(0));
      MinPoint[2] =
        static_cast<vtkm::Float32>(vertices.GetPortalConstControl().GetThirdPortal().Get(0));

      MaxPoint[0] = static_cast<vtkm::Float32>(
        vertices.GetPortalConstControl().GetFirstPortal().Get(PointDimensions[0] - 1));
      MaxPoint[1] = static_cast<vtkm::Float32>(
        vertices.GetPortalConstControl().GetSecondPortal().Get(PointDimensions[1] - 1));
      MaxPoint[2] = static_cast<vtkm::Float32>(
        vertices.GetPortalConstControl().GetThirdPortal().Get(PointDimensions[2] - 1));
    }
    else
    {
      vtkm::cont::ArrayHandleUniformPointCoordinates vertices;
      vertices = coord.GetData().Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>();
      typedef typename vtkm::cont::ArrayHandleUniformPointCoordinates UniformArrayHandle;
      typedef
        typename UniformArrayHandle::ExecutionTypes<DeviceAdapter>::PortalConst UniformConstPortal;
      UniformConstPortal Coordinates = vertices.PrepareForInput(DeviceAdapter());

      MinPoint = Coordinates.GetOrigin();
      vtkm::Vec<vtkm::Float32, 3> spacing = Coordinates.GetSpacing();

      vtkm::Vec<vtkm::Float32, 3> unitLength;
      unitLength[0] = static_cast<vtkm::Float32>(PointDimensions[0] - 1);
      unitLength[1] = static_cast<vtkm::Float32>(PointDimensions[1] - 1);
      unitLength[2] = static_cast<vtkm::Float32>(PointDimensions[2] - 1);
      MaxPoint = MinPoint + spacing * unitLength;
    }

    // Create a worklet to count the number of external faces on each cell
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numExternalFaces;
    vtkm::worklet::DispatcherMapTopology<NumExternalFacesPerStructuredCell>
      numExternalFacesDispatcher((NumExternalFacesPerStructuredCell(MinPoint, MaxPoint)));

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    vtkm::cont::Timer<DeviceAdapter> timer;
#endif
    numExternalFacesDispatcher.Invoke(inCellSet, numExternalFaces, coord.GetData());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "NumExternalFacesPerStructuredCell_Worklet," << timer.GetElapsedTime() << "\n";
#endif

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;
    vtkm::Id numberOfExternalFaces = DeviceAlgorithms::Reduce(numExternalFaces, 0, vtkm::Sum());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "numberOfExternalFaces_Reduce," << timer.GetElapsedTime() << "\n";
#endif

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    vtkm::worklet::ScatterCounting scatterCellToExternalFace(numExternalFaces, DeviceAdapter());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "numExternalFaces_ScatterCounting," << timer.GetElapsedTime() << "\n";
#endif

    // Maps output cells to input cells. Store this for cell field mapping.
    this->CellIdMap = scatterCellToExternalFace.GetOutputToInputMap();

    numExternalFaces.ReleaseResources();

    vtkm::Id connectivitySize = 4 * numberOfExternalFaces;
    vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorage> faceConnectivity;
    vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorage> faceShapes;
    vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorage> facePointCount;
    // Must pre allocate because worklet invocation will not have enough
    // information to.
    faceConnectivity.Allocate(connectivitySize);

    vtkm::worklet::DispatcherMapTopology<BuildConnectivityStructured>
      buildConnectivityStructuredDispatcher(
        (BuildConnectivityStructured(MinPoint, MaxPoint, scatterCellToExternalFace)));

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    buildConnectivityStructuredDispatcher.Invoke(
      inCellSet,
      inCellSet,
      faceShapes,
      facePointCount,
      vtkm::cont::make_ArrayHandleGroupVec<4>(faceConnectivity),
      coord.GetData());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "BuildConnectivityStructured_Worklet," << timer.GetElapsedTime() << "\n";
#endif
    outCellSet.Fill(inCellSet.GetNumberOfPoints(), faceShapes, facePointCount, faceConnectivity);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "Total External Faces = " << outCellSet.GetNumberOfCells() << std::endl;
#endif
  }

  ///////////////////////////////////////////////////
  /// \brief ExternalFaces: Extract Faces on outside of geometry
  template <typename InCellSetType,
            typename ShapeStorage,
            typename NumIndicesStorage,
            typename ConnectivityStorage,
            typename OffsetsStorage,
            typename DeviceAdapter>
  VTKM_CONT void Run(const InCellSetType& inCellSet,
                     vtkm::cont::CellSetExplicit<ShapeStorage,
                                                 NumIndicesStorage,
                                                 ConnectivityStorage,
                                                 OffsetsStorage>& outCellSet,
                     DeviceAdapter)
  {
    typedef vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorage> PointCountArrayType;
    typedef vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorage> ShapeArrayType;
    typedef vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage> OffsetsArrayType;
    typedef vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorage> ConnectivityArrayType;
    typedef vtkm::cont::ArrayHandle<vtkm::Id> CellIdArrayType;

    //Create a worklet to map the number of faces to each cell
    vtkm::cont::ArrayHandle<vtkm::IdComponent> facesPerCell;
    vtkm::worklet::DispatcherMapTopology<NumFacesPerCell> numFacesDispatcher;

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    vtkm::cont::Timer<DeviceAdapter> timer;
#endif
    numFacesDispatcher.Invoke(inCellSet, facesPerCell);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "NumFacesPerCell_Worklet," << timer.GetElapsedTime() << "\n";
#endif

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    vtkm::worklet::ScatterCounting scatterCellToFace(facesPerCell, DeviceAdapter());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "FaceInputCount_ScatterCounting," << timer.GetElapsedTime() << "\n";
#endif
    facesPerCell.ReleaseResources();

    PointCountArrayType polyDataPointCount;
    ShapeArrayType polyDataShapes;
    OffsetsArrayType polyDataOffsets;
    ConnectivityArrayType polyDataConnectivity;
    CellIdArrayType polyDataCellIdMap;
    vtkm::Id polyDataConnectivitySize = 0;
    if (this->PassPolyData)
    {
      vtkm::cont::ArrayHandle<vtkm::IdComponent> isPolyDataCell;
      vtkm::worklet::DispatcherMapTopology<IsPolyDataCell> isPolyDataCellDispatcher;

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
      timer.Reset();
#endif
      isPolyDataCellDispatcher.Invoke(inCellSet, isPolyDataCell);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
      std::cout << "IsPolyDataCell_Worklet," << timer.GetElapsedTime() << "\n";
#endif

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
      timer.Reset();
#endif
      vtkm::worklet::ScatterCounting scatterPolyDataCells(isPolyDataCell, DeviceAdapter());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
      std::cout << "scatterPolyDataCells_ScatterCounting," << timer.GetElapsedTime() << "\n";
#endif

      isPolyDataCell.ReleaseResources();

      if (scatterPolyDataCells.GetOutputRange(inCellSet.GetNumberOfCells()) != 0)
      {
        vtkm::worklet::DispatcherMapTopology<CountPolyDataCellPoints, DeviceAdapter>
          countPolyDataCellPointsDispatcher((CountPolyDataCellPoints(scatterPolyDataCells)));

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        timer.Reset();
#endif
        countPolyDataCellPointsDispatcher.Invoke(inCellSet, polyDataPointCount);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "CountPolyDataCellPoints_Worklet" << timer.GetElapsedTime() << "\n";
#endif

        vtkm::cont::ConvertNumComponentsToOffsets(
          polyDataPointCount, polyDataOffsets, polyDataConnectivitySize);

        vtkm::worklet::DispatcherMapTopology<PassPolyDataCells, DeviceAdapter>
          passPolyDataCellsDispatcher((PassPolyDataCells(scatterPolyDataCells)));

        polyDataConnectivity.Allocate(polyDataConnectivitySize);

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        timer.Reset();
#endif
        passPolyDataCellsDispatcher.Invoke(
          inCellSet,
          polyDataShapes,
          vtkm::cont::make_ArrayHandleGroupVecVariable(polyDataConnectivity, polyDataOffsets),
          polyDataCellIdMap);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
        std::cout << "PassPolyDataCells_Worklet," << timer.GetElapsedTime() << "\n";
#endif
      }
    }

    if (scatterCellToFace.GetOutputRange(inCellSet.GetNumberOfCells()) == 0)
    {
      if (!polyDataConnectivitySize)
      {
        // Data has no faces. Output is empty.
        outCellSet.PrepareToAddCells(0, 0);
        outCellSet.CompleteAddingCells(inCellSet.GetNumberOfPoints());
        return;
      }
      else
      {
        // Pass only input poly data to output
        outCellSet.Fill(inCellSet.GetNumberOfPoints(),
                        polyDataShapes,
                        polyDataPointCount,
                        polyDataConnectivity,
                        polyDataOffsets);
        this->CellIdMap = polyDataCellIdMap;
        return;
      }
    }

    vtkm::cont::ArrayHandle<vtkm::Id3> faceHashes;
    vtkm::cont::ArrayHandle<vtkm::Id> originCells;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> originFaces;
    vtkm::worklet::DispatcherMapTopology<FaceHash, DeviceAdapter> faceHashDispatcher(
      (FaceHash(scatterCellToFace)));

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    faceHashDispatcher.Invoke(inCellSet, faceHashes, originCells, originFaces);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "FaceHash_Worklet," << timer.GetElapsedTime() << "\n";
#endif

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    vtkm::worklet::Keys<vtkm::Id3> faceKeys(faceHashes, DeviceAdapter());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "Keys_BuildArrays," << timer.GetElapsedTime() << "\n";
#endif

    vtkm::cont::ArrayHandle<vtkm::IdComponent> faceOutputCount;
    vtkm::worklet::DispatcherReduceByKey<FaceCounts, DeviceAdapter> faceCountDispatcher;

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    faceCountDispatcher.Invoke(faceKeys, faceOutputCount);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "FaceCount_Worklet," << timer.GetElapsedTime() << "\n";
#endif

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    vtkm::worklet::ScatterCounting scatterCullInternalFaces(faceOutputCount, DeviceAdapter());
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "FaceOutputCount_ScatterCounting," << timer.GetElapsedTime() << "\n";
#endif

    PointCountArrayType facePointCount;
    vtkm::worklet::DispatcherReduceByKey<NumPointsPerFace, DeviceAdapter> pointsPerFaceDispatcher(
      scatterCullInternalFaces);

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    pointsPerFaceDispatcher.Invoke(faceKeys, inCellSet, originCells, originFaces, facePointCount);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "PointsPerFaceCount_Worklet," << timer.GetElapsedTime() << "\n";
#endif

    ShapeArrayType faceShapes;

    OffsetsArrayType faceOffsets;
    vtkm::Id connectivitySize;
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    vtkm::cont::ConvertNumComponentsToOffsets(facePointCount, faceOffsets, connectivitySize);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "FacePointCount_ScanExclusive," << timer.GetElapsedTime() << "\n";
#endif

    ConnectivityArrayType faceConnectivity;
    // Must pre allocate because worklet invocation will not have enough
    // information to.
    faceConnectivity.Allocate(connectivitySize);

    vtkm::worklet::DispatcherReduceByKey<BuildConnectivity, DeviceAdapter>
      buildConnectivityDispatcher(scatterCullInternalFaces);

    CellIdArrayType faceToCellIdMap;

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    buildConnectivityDispatcher.Invoke(
      faceKeys,
      inCellSet,
      originCells,
      originFaces,
      faceShapes,
      vtkm::cont::make_ArrayHandleGroupVecVariable(faceConnectivity, faceOffsets),
      faceToCellIdMap);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "BuildConnectivity_Worklet," << timer.GetElapsedTime() << "\n";
#endif

    if (!polyDataConnectivitySize)
    {
      outCellSet.Fill(
        inCellSet.GetNumberOfPoints(), faceShapes, facePointCount, faceConnectivity, faceOffsets);
      this->CellIdMap = faceToCellIdMap;
    }
    else
    {
      // Join poly data to face data output
      typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

      vtkm::cont::ArrayHandleConcatenate<ShapeArrayType, ShapeArrayType> faceShapesArray(
        faceShapes, polyDataShapes);
      ShapeArrayType joinedShapesArray;
      DeviceAlgorithm::Copy(faceShapesArray, joinedShapesArray);

      vtkm::cont::ArrayHandleConcatenate<PointCountArrayType, PointCountArrayType> pointCountArray(
        facePointCount, polyDataPointCount);
      PointCountArrayType joinedPointCountArray;
      DeviceAlgorithm::Copy(pointCountArray, joinedPointCountArray);

      vtkm::cont::ArrayHandleConcatenate<ConnectivityArrayType, ConnectivityArrayType>
        connectivityArray(faceConnectivity, polyDataConnectivity);
      ConnectivityArrayType joinedConnectivity;
      DeviceAlgorithm::Copy(connectivityArray, joinedConnectivity);

      // Adjust poly data offsets array with face connectivity size before join
      typedef vtkm::cont::ArrayHandleTransform<OffsetsArrayType, BiasFunctor<vtkm::Id>>
        TransformBiasArrayType;
      TransformBiasArrayType adjustedPolyDataOffsets =
        vtkm::cont::make_ArrayHandleTransform<OffsetsArrayType>(
          polyDataOffsets, BiasFunctor<vtkm::Id>(faceConnectivity.GetNumberOfValues()));
      vtkm::cont::ArrayHandleConcatenate<OffsetsArrayType, TransformBiasArrayType> offsetsArray(
        faceOffsets, adjustedPolyDataOffsets);
      OffsetsArrayType joinedOffsets;
      DeviceAlgorithm::Copy(offsetsArray, joinedOffsets);

      vtkm::cont::ArrayHandleConcatenate<CellIdArrayType, CellIdArrayType> cellIdMapArray(
        faceToCellIdMap, polyDataCellIdMap);
      CellIdArrayType joinedCellIdMap;
      DeviceAlgorithm::Copy(cellIdMapArray, joinedCellIdMap);

      outCellSet.Fill(inCellSet.GetNumberOfPoints(),
                      joinedShapesArray,
                      joinedPointCountArray,
                      joinedConnectivity,
                      joinedOffsets);
      this->CellIdMap = joinedCellIdMap;
    }

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "Total External Faces = " << outCellSet.GetNumberOfCells() << std::endl;
#endif
  }

}; //struct ExternalFaces
}
} //namespace vtkm::worklet

#endif //vtk_m_worklet_ExternalFaces_h
