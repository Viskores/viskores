//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/exec/CellEdge.h>
#include <vtkm/exec/CellFace.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

struct ExtractEdges
{
  ////
  //// BEGIN-EXAMPLE CellEdge
  ////
  struct EdgesCount : vtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell numEdgesInCell);
    using ExecutionSignature = void(CellShape, PointCount, _2);
    using InputDomain = _1;

    template<typename CellShapeTag>
    VTKM_EXEC void operator()(CellShapeTag cellShape,
                              vtkm::IdComponent numPointsInCell,
                              vtkm::IdComponent& numEdges) const
    {
      vtkm::ErrorCode status =
        vtkm::exec::CellEdgeNumberOfEdges(numPointsInCell, cellShape, numEdges);
      if (status != vtkm::ErrorCode::Success)
      {
        this->RaiseError(vtkm::ErrorString(status));
      }
    }
  };

  ////
  //// BEGIN-EXAMPLE ComplexWorklet
  ////
  struct EdgesExtract : vtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell edgeIndices);
    using ExecutionSignature = void(CellShape, PointIndices, VisitIndex, _2);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template<typename CellShapeTag,
             typename PointIndexVecType,
             typename EdgeIndexVecType>
    VTKM_EXEC void operator()(CellShapeTag cellShape,
                              const PointIndexVecType& globalPointIndicesForCell,
                              vtkm::IdComponent edgeIndex,
                              EdgeIndexVecType& edgeIndices) const
    {
      ////
      //// END-EXAMPLE ComplexWorklet
      ////
      vtkm::IdComponent numPointsInCell =
        globalPointIndicesForCell.GetNumberOfComponents();

      vtkm::ErrorCode error;

      vtkm::IdComponent pointInCellIndex0;
      error = vtkm::exec::CellEdgeLocalIndex(
        numPointsInCell, 0, edgeIndex, cellShape, pointInCellIndex0);
      if (error != vtkm::ErrorCode::Success)
      {
        this->RaiseError(vtkm::ErrorString(error));
        return;
      }

      vtkm::IdComponent pointInCellIndex1;
      error = vtkm::exec::CellEdgeLocalIndex(
        numPointsInCell, 1, edgeIndex, cellShape, pointInCellIndex1);
      if (error != vtkm::ErrorCode::Success)
      {
        this->RaiseError(vtkm::ErrorString(error));
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
  VTKM_CONT vtkm::cont::CellSetSingleType<> Run(const CellSetInType& cellSetIn)
  {
    // Count how many edges each cell has
    vtkm::cont::ArrayHandle<vtkm::IdComponent> edgeCounts;
    vtkm::worklet::DispatcherMapTopology<EdgesCount> countDispatcher;
    countDispatcher.Invoke(cellSetIn, edgeCounts);

    // Set up a "scatter" to create an output entry for each edge in the input
    vtkm::worklet::ScatterCounting scatter(edgeCounts);

    // Get the cell index array for all the edges
    vtkm::cont::ArrayHandle<vtkm::Id> edgeIndices;
    vtkm::worklet::DispatcherMapTopology<EdgesExtract> extractDispatcher(scatter);
    extractDispatcher.Invoke(cellSetIn,
                             vtkm::cont::make_ArrayHandleGroupVec<2>(edgeIndices));

    // Construct the resulting cell set and return
    vtkm::cont::CellSetSingleType<> cellSetOut;
    cellSetOut.Fill(
      cellSetIn.GetNumberOfPoints(), vtkm::CELL_SHAPE_LINE, 2, edgeIndices);
    return cellSetOut;
  }
};

void TryExtractEdges()
{
  std::cout << "Trying extract edges worklets." << std::endl;

  vtkm::cont::DataSet dataSet =
    vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

  ExtractEdges extractEdges;
  vtkm::cont::CellSetSingleType<> edgeCells = extractEdges.Run(dataSet.GetCellSet());

  VTKM_TEST_ASSERT(edgeCells.GetNumberOfPoints() == 11,
                   "Output has wrong number of points");
  VTKM_TEST_ASSERT(edgeCells.GetNumberOfCells() == 35,
                   "Output has wrong number of cells");
}

struct ExtractFaces
{
  ////
  //// BEGIN-EXAMPLE CellFace
  ////
  struct FacesCount : vtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell numFacesInCell);
    using ExecutionSignature = void(CellShape, _2);
    using InputDomain = _1;

    template<typename CellShapeTag>
    VTKM_EXEC void operator()(CellShapeTag cellShape, vtkm::IdComponent& numFaces) const
    {
      vtkm::ErrorCode status = vtkm::exec::CellFaceNumberOfFaces(cellShape, numFaces);
      if (status != vtkm::ErrorCode::Success)
      {
        this->RaiseError(vtkm::ErrorString(status));
      }
    }
  };

  struct FacesCountPoints : vtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn,
                                  FieldOutCell numPointsInFace,
                                  FieldOutCell faceShape);
    using ExecutionSignature = void(CellShape, VisitIndex, _2, _3);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template<typename CellShapeTag>
    VTKM_EXEC void operator()(CellShapeTag cellShape,
                              vtkm::IdComponent faceIndex,
                              vtkm::IdComponent& numPointsInFace,
                              vtkm::UInt8& faceShape) const
    {
      vtkm::exec::CellFaceNumberOfPoints(faceIndex, cellShape, numPointsInFace);
      switch (numPointsInFace)
      {
        case 3:
          faceShape = vtkm::CELL_SHAPE_TRIANGLE;
          break;
        case 4:
          faceShape = vtkm::CELL_SHAPE_QUAD;
          break;
        default:
          faceShape = vtkm::CELL_SHAPE_POLYGON;
          break;
      }
    }
  };

  struct FacesExtract : vtkm::worklet::WorkletVisitCellsWithPoints
  {
    using ControlSignature = void(CellSetIn, FieldOutCell faceIndices);
    using ExecutionSignature = void(CellShape, PointIndices, VisitIndex, _2);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template<typename CellShapeTag,
             typename PointIndexVecType,
             typename FaceIndexVecType>
    VTKM_EXEC void operator()(CellShapeTag cellShape,
                              const PointIndexVecType& globalPointIndicesForCell,
                              vtkm::IdComponent faceIndex,
                              FaceIndexVecType& faceIndices) const
    {
      vtkm::IdComponent numPointsInFace = faceIndices.GetNumberOfComponents();
      for (vtkm::IdComponent pointInFaceIndex = 0; pointInFaceIndex < numPointsInFace;
           pointInFaceIndex++)
      {
        vtkm::IdComponent pointInCellIndex;
        vtkm::ErrorCode error = vtkm::exec::CellFaceLocalIndex(
          pointInFaceIndex, faceIndex, cellShape, pointInCellIndex);
        if (error != vtkm::ErrorCode::Success)
        {
          this->RaiseError(vtkm::ErrorString(error));
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
  VTKM_CONT vtkm::cont::CellSetExplicit<> Run(const CellSetInType& cellSetIn)
  {
    // Count how many faces each cell has
    vtkm::cont::ArrayHandle<vtkm::IdComponent> faceCounts;
    vtkm::worklet::DispatcherMapTopology<FacesCount> countDispatcher;
    countDispatcher.Invoke(cellSetIn, faceCounts);

    // Set up a "scatter" to create an output entry for each face in the input
    vtkm::worklet::ScatterCounting scatter(faceCounts);

    // Count how many points each face has. Also get the shape of each face.
    vtkm::cont::ArrayHandle<vtkm::IdComponent> pointsPerFace;
    vtkm::cont::ArrayHandle<vtkm::UInt8> faceShapes;
    vtkm::worklet::DispatcherMapTopology<FacesCountPoints> countPointsDispatcher(
      scatter);
    countPointsDispatcher.Invoke(cellSetIn, pointsPerFace, faceShapes);

    // To construct an ArrayHandleGroupVecVariable, we need to convert
    // pointsPerFace to an array of offsets
    vtkm::Id faceIndicesSize;
    vtkm::cont::ArrayHandle<vtkm::Id> faceIndexOffsets =
      vtkm::cont::ConvertNumComponentsToOffsets(pointsPerFace, faceIndicesSize);

    // We need to preallocate the array for faceIndices (because that is the
    // way ArrayHandleGroupVecVariable works). We use the value previously
    // returned from ConvertNumComponentsToOffsets.
    vtkm::cont::ArrayHandle<vtkm::Id> faceIndices;
    faceIndices.Allocate(faceIndicesSize);

    // Get the cell index array for all the faces
    vtkm::worklet::DispatcherMapTopology<FacesExtract> extractDispatcher(scatter);
    extractDispatcher.Invoke(
      cellSetIn,
      vtkm::cont::make_ArrayHandleGroupVecVariable(faceIndices, faceIndexOffsets));

    // Construct the resulting cell set and return
    vtkm::cont::CellSetExplicit<> cellSetOut;
    cellSetOut.Fill(
      cellSetIn.GetNumberOfPoints(), faceShapes, faceIndices, faceIndexOffsets);
    return cellSetOut;
  }
};

void TryExtractFaces()
{
  std::cout << "Trying extract faces worklets." << std::endl;

  vtkm::cont::DataSet dataSet =
    vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet5();

  ExtractFaces extractFaces;
  vtkm::cont::CellSetExplicit<> faceCells = extractFaces.Run(dataSet.GetCellSet());

  VTKM_TEST_ASSERT(faceCells.GetNumberOfPoints() == 11,
                   "Output has wrong number of points");
  VTKM_TEST_ASSERT(faceCells.GetNumberOfCells() == 20,
                   "Output has wrong number of cells");

  VTKM_TEST_ASSERT(faceCells.GetCellShape(0) == vtkm::CELL_SHAPE_QUAD, "Face wrong");
  vtkm::Id4 quadIndices;
  faceCells.GetIndices(0, quadIndices);
  VTKM_TEST_ASSERT(test_equal(quadIndices, vtkm::Id4(0, 3, 7, 4)), "Face wrong");

  VTKM_TEST_ASSERT(faceCells.GetCellShape(12) == vtkm::CELL_SHAPE_TRIANGLE,
                   "Face wrong");
  vtkm::Id3 triIndices;
  faceCells.GetIndices(12, triIndices);
  VTKM_TEST_ASSERT(test_equal(triIndices, vtkm::Id3(8, 10, 6)), "Face wrong");
}

void Run()
{
  TryExtractEdges();
  TryExtractFaces();
}

} // anonymous namespace

int GuideExampleCellEdgesFaces(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
