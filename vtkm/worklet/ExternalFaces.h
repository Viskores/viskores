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
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
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
                                  ReducedValuesOut<> connectivityOut);
    typedef void ExecutionSignature(_2, _3, _4, _5, _6);
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
                              ConnectivityType& connectivityOut) const
    {
      VTKM_ASSERT(originCells.GetNumberOfComponents() == 1);
      VTKM_ASSERT(originFaces.GetNumberOfComponents() == 1);

      typename CellSetType::CellShapeTag shapeIn = cellSet.GetCellShape(originCells[0]);
      shapeOut = vtkm::exec::CellFaceShape(originFaces[0], shapeIn, *this);

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

public:
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

    if (scatterCellToFace.GetOutputRange(inCellSet.GetNumberOfCells()) == 0)
    {
      // Data has no faces. Output is empty.
      outCellSet.PrepareToAddCells(0, 0);
      outCellSet.CompleteAddingCells(inCellSet.GetNumberOfPoints());
      return;
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

    vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorage> facePointCount;
    vtkm::worklet::DispatcherReduceByKey<NumPointsPerFace, DeviceAdapter> pointsPerFaceDispatcher(
      scatterCullInternalFaces);

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    pointsPerFaceDispatcher.Invoke(faceKeys, inCellSet, originCells, originFaces, facePointCount);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "PointsPerFaceCount_Worklet," << timer.GetElapsedTime() << "\n";
#endif

    vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorage> faceShapes;

    vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorage> faceOffsets;
    vtkm::Id connectivitySize;
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    vtkm::cont::ConvertNumComponentsToOffsets(facePointCount, faceOffsets, connectivitySize);
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "FacePointCount_ScanExclusive," << timer.GetElapsedTime() << "\n";
#endif

    vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorage> faceConnectivity;
    // Must pre allocate because worklet invocation will not have enough
    // information to.
    faceConnectivity.Allocate(connectivitySize);

    vtkm::worklet::DispatcherReduceByKey<BuildConnectivity, DeviceAdapter>
      buildConnectivityDispatcher(scatterCullInternalFaces);

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    timer.Reset();
#endif
    buildConnectivityDispatcher.Invoke(
      faceKeys,
      inCellSet,
      originCells,
      originFaces,
      faceShapes,
      vtkm::cont::make_ArrayHandleGroupVecVariable(faceConnectivity, faceOffsets));
#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "BuildConnectivity_Worklet," << timer.GetElapsedTime() << "\n";
#endif

    outCellSet.Fill(
      inCellSet.GetNumberOfPoints(), faceShapes, facePointCount, faceConnectivity, faceOffsets);

#ifdef __VTKM_EXTERNAL_FACES_BENCHMARK
    std::cout << "Total External Faces = " << outCellSet.GetNumberOfCells() << std::endl;
#endif
  }

}; //struct ExternalFaces
}
} //namespace vtkm::worklet

#endif //vtk_m_worklet_ExternalFaces_h
