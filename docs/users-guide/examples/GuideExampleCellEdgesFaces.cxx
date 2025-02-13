//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/exec/CellEdge.h>
#include <viskores/exec/CellFace.h>

#include <viskores/worklet/DispatcherMapTopology.h>
#include <viskores/worklet/ScatterCounting.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/ArrayHandleGroupVecVariable.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/ConvertNumComponentsToOffsets.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

struct ExtractEdges
{
  ////
  //// BEGIN-EXAMPLE CellEdge
  ////
  struct EdgesCount : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell numEdgesInCell);
    using ExecutionSignature = void(CellShape, PointCount, _2);
    using InputDomain = _1;

    template<typename CellShapeTag>
    VISKORES_EXEC void operator()(CellShapeTag cellShape,
                                  viskores::IdComponent numPointsInCell,
                                  viskores::IdComponent& numEdges) const
    {
      viskores::ErrorCode status =
        viskores::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, numEdges);
      if (status != viskores::ErrorCode::Success)
      {
        this->RaiseError(viskores::ErrorString(status));
      }
    }
  };

  ////
  //// BEGIN-EXAMPLE ComplexWorklet
  ////
  struct EdgesExtract : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell edgeIndices);
    using ExecutionSignature = void(CellShape, PointIndices, VisitIndex, _2);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;

    template<typename CellShapeTag,
             typename PointIndexVecType,
             typename EdgeIndexVecType>
    VISKORES_EXEC void operator()(CellShapeTag cellShape,
                                  const PointIndexVecType& globalPointIndicesForCell,
                                  viskores::IdComponent edgeIndex,
                                  EdgeIndexVecType& edgeIndices) const
    {
      ////
      //// END-EXAMPLE ComplexWorklet
      ////
      viskores::IdComponent numPointsInCell =
        globalPointIndicesForCell.GetNumberOfComponents();

      viskores::ErrorCode error;

      viskores::IdComponent pointInCellIndex0;
      error = viskores::exec::CellEdgeLocalIndex(
        numPointsInCell, 0, edgeIndex, cellShape, pointInCellIndex0);
      if (error != viskores::ErrorCode::Success)
      {
        this->RaiseError(viskores::ErrorString(error));
        return;
      }

      viskores::IdComponent pointInCellIndex1;
      error = viskores::exec::CellEdgeLocalIndex(
        numPointsInCell, 1, edgeIndex, cellShape, pointInCellIndex1);
      if (error != viskores::ErrorCode::Success)
      {
        this->RaiseError(viskores::ErrorString(error));
        return;
      }

      edgeIndices[0] = globalPointIndicesForCell[pointInCellIndex0];
      edgeIndices[1] = globalPointIndicesForCell[pointInCellIndex1];
    }
  };
  ////
  //// END-EXAMPLE CellEdge
  ////

  template<typename CellSetInType>
  VISKORES_CONT viskores::cont::CellSetSingleType<> Run(const CellSetInType& cellSetIn)
  {
    // Count how many edges each cell has
    viskores::cont::ArrayHandle<viskores::IdComponent> edgeCounts;
    viskores::worklet::DispatcherMapTopology<EdgesCount> countDispatcher;
    countDispatcher.Invoke(cellSetIn, edgeCounts);

    // Set up a "scatter" to create an output entry for each edge in the input
    viskores::worklet::ScatterCounting scatter(edgeCounts);

    // Get the cell index array for all the edges
    viskores::cont::ArrayHandle<viskores::Id> edgeIndices;
    viskores::worklet::DispatcherMapTopology<EdgesExtract> extractDispatcher(scatter);
    extractDispatcher.Invoke(cellSetIn,
                             viskores::cont::make_ArrayHandleGroupVec<2>(edgeIndices));

    // Construct the resulting cell set and return
    viskores::cont::CellSetSingleType<> cellSetOut;
    cellSetOut.Fill(
      cellSetIn.GetNumberOfPoints(), viskores::CELL_SHAPE_LINE, 2, edgeIndices);
    return cellSetOut;
  }
};

void TryExtractEdges()
{
  std::cout << "Trying extract edges worklets." << std::endl;

  viskores::cont::DataSet dataSet =
    viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

  ExtractEdges extractEdges;
  viskores::cont::CellSetSingleType<> edgeCells = extractEdges.Run(dataSet.GetCellSet());

  VISKORES_TEST_ASSERT(edgeCells.GetNumberOfPoints() == 11,
                       "Output has wrong number of points");
  VISKORES_TEST_ASSERT(edgeCells.GetNumberOfCells() == 35,
                       "Output has wrong number of cells");
}

struct ExtractFaces
{
  ////
  //// BEGIN-EXAMPLE CellFace
  ////
  struct FacesCount : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell numFacesInCell);
    using ExecutionSignature = void(CellShape, _2);
    using InputDomain = _1;

    template<typename CellShapeTag>
    VISKORES_EXEC void operator()(CellShapeTag cellShape,
                                  viskores::IdComponent& numFaces) const
    {
      viskores::ErrorCode status =
        viskores::exec::CellFaceNumberOfFaces(cellShape, numFaces);
      if (status != viskores::ErrorCode::Success)
      {
        this->RaiseError(viskores::ErrorString(status));
      }
    }
  };

  struct FacesCountPoints : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn,
                                  FieldOutCell numPointsInFace,
                                  FieldOutCell faceShape);
    using ExecutionSignature = void(CellShape, VisitIndex, _2, _3);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;

    template<typename CellShapeTag>
    VISKORES_EXEC void operator()(CellShapeTag cellShape,
                                  viskores::IdComponent faceIndex,
                                  viskores::IdComponent& numPointsInFace,
                                  viskores::UInt8& faceShape) const
    {
      viskores::exec::CellFaceNumberOfPoints(faceIndex, cellShape, numPointsInFace);
      switch (numPointsInFace)
      {
        case 3:
          faceShape = viskores::CELL_SHAPE_TRIANGLE;
          break;
        case 4:
          faceShape = viskores::CELL_SHAPE_QUAD;
          break;
        default:
          faceShape = viskores::CELL_SHAPE_POLYGON;
          break;
      }
    }
  };

  struct FacesExtract : viskores::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell faceIndices);
    using ExecutionSignature = void(CellShape, PointIndices, VisitIndex, _2);
    using InputDomain = _1;

    using ScatterType = viskores::worklet::ScatterCounting;

    template<typename CellShapeTag,
             typename PointIndexVecType,
             typename FaceIndexVecType>
    VISKORES_EXEC void operator()(CellShapeTag cellShape,
                                  const PointIndexVecType& globalPointIndicesForCell,
                                  viskores::IdComponent faceIndex,
                                  FaceIndexVecType& faceIndices) const
    {
      viskores::IdComponent numPointsInFace = faceIndices.GetNumberOfComponents();
      for (viskores::IdComponent pointInFaceIndex = 0;
           pointInFaceIndex < numPointsInFace;
           pointInFaceIndex++)
      {
        viskores::IdComponent pointInCellIndex;
        viskores::ErrorCode error = viskores::exec::CellFaceLocalIndex(
          pointInFaceIndex, faceIndex, cellShape, pointInCellIndex);
        if (error != viskores::ErrorCode::Success)
        {
          this->RaiseError(viskores::ErrorString(error));
          return;
        }
        faceIndices[pointInFaceIndex] = globalPointIndicesForCell[pointInCellIndex];
      }
    }
  };
  ////
  //// END-EXAMPLE CellFace
  ////

  template<typename CellSetInType>
  VISKORES_CONT viskores::cont::CellSetExplicit<> Run(const CellSetInType& cellSetIn)
  {
    // Count how many faces each cell has
    viskores::cont::ArrayHandle<viskores::IdComponent> faceCounts;
    viskores::worklet::DispatcherMapTopology<FacesCount> countDispatcher;
    countDispatcher.Invoke(cellSetIn, faceCounts);

    // Set up a "scatter" to create an output entry for each face in the input
    viskores::worklet::ScatterCounting scatter(faceCounts);

    // Count how many points each face has. Also get the shape of each face.
    viskores::cont::ArrayHandle<viskores::IdComponent> pointsPerFace;
    viskores::cont::ArrayHandle<viskores::UInt8> faceShapes;
    viskores::worklet::DispatcherMapTopology<FacesCountPoints> countPointsDispatcher(
      scatter);
    countPointsDispatcher.Invoke(cellSetIn, pointsPerFace, faceShapes);

    // To construct an ArrayHandleGroupVecVariable, we need to convert
    // pointsPerFace to an array of offsets
    viskores::Id faceIndicesSize;
    viskores::cont::ArrayHandle<viskores::Id> faceIndexOffsets =
      viskores::cont::ConvertNumComponentsToOffsets(pointsPerFace, faceIndicesSize);

    // We need to preallocate the array for faceIndices (because that is the
    // way ArrayHandleGroupVecVariable works). We use the value previously
    // returned from ConvertNumComponentsToOffsets.
    viskores::cont::ArrayHandle<viskores::Id> faceIndices;
    faceIndices.Allocate(faceIndicesSize);

    // Get the cell index array for all the faces
    viskores::worklet::DispatcherMapTopology<FacesExtract> extractDispatcher(scatter);
    extractDispatcher.Invoke(
      cellSetIn,
      viskores::cont::make_ArrayHandleGroupVecVariable(faceIndices, faceIndexOffsets));

    // Construct the resulting cell set and return
    viskores::cont::CellSetExplicit<> cellSetOut;
    cellSetOut.Fill(
      cellSetIn.GetNumberOfPoints(), faceShapes, faceIndices, faceIndexOffsets);
    return cellSetOut;
  }
};

void TryExtractFaces()
{
  std::cout << "Trying extract faces worklets." << std::endl;

  viskores::cont::DataSet dataSet =
    viskores::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

  ExtractFaces extractFaces;
  viskores::cont::CellSetExplicit<> faceCells = extractFaces.Run(dataSet.GetCellSet());

  VISKORES_TEST_ASSERT(faceCells.GetNumberOfPoints() == 11,
                       "Output has wrong number of points");
  VISKORES_TEST_ASSERT(faceCells.GetNumberOfCells() == 20,
                       "Output has wrong number of cells");

  VISKORES_TEST_ASSERT(faceCells.GetCellShape(0) == viskores::CELL_SHAPE_QUAD,
                       "Face wrong");
  viskores::Id4 quadIndices;
  faceCells.GetIndices(0, quadIndices);
  VISKORES_TEST_ASSERT(test_equal(quadIndices, viskores::Id4(0, 3, 7, 4)), "Face wrong");

  VISKORES_TEST_ASSERT(faceCells.GetCellShape(12) == viskores::CELL_SHAPE_TRIANGLE,
                       "Face wrong");
  viskores::Id3 triIndices;
  faceCells.GetIndices(12, triIndices);
  VISKORES_TEST_ASSERT(test_equal(triIndices, viskores::Id3(8, 10, 6)), "Face wrong");
}

void Run()
{
  TryExtractEdges();
  TryExtractFaces();
}

} // anonymous namespace

int GuideExampleCellEdgesFaces(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
