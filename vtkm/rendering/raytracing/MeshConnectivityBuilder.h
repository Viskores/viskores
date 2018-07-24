//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_MeshConnectivityBuilder_h
#define vtk_m_rendering_raytracing_MeshConnectivityBuilder_h

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/rendering/raytracing/CellTables.h>
#include <vtkm/rendering/raytracing/Logger.h>
#include <vtkm/rendering/raytracing/MortonCodes.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>
#include <vtkm/rendering/raytracing/Worklets.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

struct IsExternal
{
  VTKM_EXEC_CONT
  inline bool operator()(const vtkm::Id& x) const { return (x < 0); }
}; //struct IsExternal

class CountFaces : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  CountFaces() {}
  using ControlSignature = void(WholeArrayIn<>, FieldOut<>);
  using ExecutionSignature = void(_1, _2, WorkIndex);
  template <typename ShapePortalType>
  VTKM_EXEC inline void operator()(const ShapePortalType& shapes,
                                   vtkm::Id& faces,
                                   const vtkm::Id& index) const
  {
    BOUNDS_CHECK(shapes, index);
    vtkm::UInt8 shapeType = shapes.Get(index);
    if (shapeType == vtkm::CELL_SHAPE_TETRA)
    {
      faces = 4;
    }
    else if (shapeType == vtkm::CELL_SHAPE_HEXAHEDRON)
    {
      faces = 6;
    }
    else if (shapeType == vtkm::CELL_SHAPE_WEDGE)
    {
      faces = 5;
    }
    else if (shapeType == vtkm::CELL_SHAPE_PYRAMID)
    {
      faces = 5;
    }
    else
    {
      faces = 0;
    }
  }
}; //class CountFaces

class MortonNeighbor : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  MortonNeighbor() {}
  using ControlSignature = void(WholeArrayIn<>,
                                WholeArrayInOut<Id3Type>,
                                WholeArrayIn<>,
                                WholeArrayIn<>,
                                WholeArrayIn<>,
                                WholeArrayOut<>);
  using ExecutionSignature = void(_1, _2, WorkIndex, _3, _4, _5, _6);

  VTKM_EXEC
  inline vtkm::Int32 GetShapeOffset(const vtkm::UInt8& shapeType) const
  {

    CellTables tables;
    //TODO: This might be better as if if if
    vtkm::Int32 tableOffset = 0;
    if (shapeType == vtkm::CELL_SHAPE_TETRA)
    {
      tableOffset = tables.FaceLookUp(1, 0);
    }
    else if (shapeType == vtkm::CELL_SHAPE_HEXAHEDRON)
    {
      tableOffset = tables.FaceLookUp(0, 0);
    }
    else if (shapeType == vtkm::CELL_SHAPE_WEDGE)
    {
      tableOffset = tables.FaceLookUp(2, 0);
    }
    else if (shapeType == vtkm::CELL_SHAPE_PYRAMID)
    {
      tableOffset = tables.FaceLookUp(3, 0);
    }
    else
      printf("Error shape not recognized %d\n", (int)shapeType);

    return tableOffset;
  }

  VTKM_EXEC
  inline bool IsIn(const vtkm::Id& needle,
                   const vtkm::Vec<vtkm::Id, 4>& heystack,
                   const vtkm::Int32& numIndices) const
  {
    bool isIn = false;
    for (vtkm::Int32 i = 0; i < numIndices; ++i)
    {
      if (needle == heystack[i])
        isIn = true;
    }
    return isIn;
  }


  template <typename MortonPortalType,
            typename FaceIdPairsPortalType,
            typename ConnPortalType,
            typename ShapePortalType,
            typename OffsetPortalType,
            typename ExternalFaceFlagType>
  VTKM_EXEC inline void operator()(const MortonPortalType& mortonCodes,
                                   FaceIdPairsPortalType& faceIdPairs,
                                   const vtkm::Id& index,
                                   const ConnPortalType& connectivity,
                                   const ShapePortalType& shapes,
                                   const OffsetPortalType& offsets,
                                   ExternalFaceFlagType& flags) const
  {
    if (index == 0)
    {
      return;
    }

    vtkm::Id currentIndex = index - 1;
    BOUNDS_CHECK(mortonCodes, index);
    vtkm::UInt32 myCode = mortonCodes.Get(index);
    BOUNDS_CHECK(mortonCodes, currentIndex);
    vtkm::UInt32 myNeighbor = mortonCodes.Get(currentIndex);
    bool isInternal = false;
    vtkm::Id connectedCell = -1;

    CellTables tables;
    while (currentIndex > -1 && myCode == myNeighbor)
    {
      myNeighbor = mortonCodes.Get(currentIndex);
      // just because the codes are equal does not mean that
      // they are the same face. We need to double check.
      if (myCode == myNeighbor)
      {
        //get the index of the shape face in the table.
        BOUNDS_CHECK(faceIdPairs, index);
        vtkm::Id cellId1 = faceIdPairs.Get(index)[0];
        BOUNDS_CHECK(faceIdPairs, currentIndex);
        vtkm::Id cellId2 = faceIdPairs.Get(currentIndex)[0];
        BOUNDS_CHECK(shapes, cellId1);
        BOUNDS_CHECK(shapes, cellId2);
        vtkm::Int32 shape1Offset =
          GetShapeOffset(shapes.Get(cellId1)) + static_cast<vtkm::Int32>(faceIdPairs.Get(index)[1]);
        vtkm::Int32 shape2Offset = GetShapeOffset(shapes.Get(cellId2)) +
          static_cast<vtkm::Int32>(faceIdPairs.Get(currentIndex)[1]);

        const vtkm::Int32 icount1 = tables.ShapesFaceList(shape1Offset, 0);
        const vtkm::Int32 icount2 = tables.ShapesFaceList(shape2Offset, 0);
        //Check to see if we have the same number of idices
        if (icount1 != icount2)
          continue;


        //TODO: we can do better than this
        vtkm::Vec<vtkm::Id, 4> indices1;
        vtkm::Vec<vtkm::Id, 4> indices2;

        const auto faceLength = tables.ShapesFaceList(shape1Offset, 0);
        for (vtkm::Int32 i = 1; i <= faceLength; ++i)
        {
          BOUNDS_CHECK(offsets, cellId1);
          BOUNDS_CHECK(offsets, cellId2);
          BOUNDS_CHECK(connectivity,
                       (offsets.Get(cellId1) + tables.ShapesFaceList(shape1Offset, i)));
          BOUNDS_CHECK(connectivity,
                       (offsets.Get(cellId2) + tables.ShapesFaceList(shape2Offset, i)));
          indices1[i - 1] =
            connectivity.Get(offsets.Get(cellId1) + tables.ShapesFaceList(shape1Offset, i));
          indices2[i - 1] =
            connectivity.Get(offsets.Get(cellId2) + tables.ShapesFaceList(shape2Offset, i));
        }

        bool isEqual = true;
        for (vtkm::Int32 i = 0; i < faceLength; ++i)
        {
          if (!IsIn(indices1[i], indices2, faceLength))
            isEqual = false;
        }

        if (isEqual)
        {
          isInternal = true;

          connectedCell = cellId2;

          break;
        }
      }
      currentIndex--;
    }

    //this means that this cell is responsible for both itself and the other cell
    //set the connection for the other cell
    if (isInternal)
    {
      BOUNDS_CHECK(faceIdPairs, index);
      vtkm::Vec<vtkm::Id, 3> facePair = faceIdPairs.Get(index);
      vtkm::Id myCell = facePair[0];
      facePair[2] = connectedCell;
      BOUNDS_CHECK(faceIdPairs, index);
      faceIdPairs.Set(index, facePair);

      BOUNDS_CHECK(faceIdPairs, currentIndex);
      facePair = faceIdPairs.Get(currentIndex);
      facePair[2] = myCell;
      BOUNDS_CHECK(faceIdPairs, currentIndex);
      faceIdPairs.Set(currentIndex, facePair);
      BOUNDS_CHECK(flags, currentIndex);
      flags.Set(currentIndex, myCell);
      BOUNDS_CHECK(flags, index);
      flags.Set(index, connectedCell);
    }
  }
}; //class Neighbor

class ExternalTriangles : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  ExternalTriangles() {}
  using ControlSignature =
    void(FieldIn<>, WholeArrayIn<>, WholeArrayIn<>, WholeArrayIn<>, WholeArrayOut<>, FieldIn<>);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);
  template <typename ShapePortalType,
            typename InIndicesPortalType,
            typename OutIndicesPortalType,
            typename ShapeOffsetsPortal>
  VTKM_EXEC inline void operator()(const vtkm::Vec<vtkm::Id, 3>& faceIdPair,
                                   const ShapePortalType& shapes,
                                   const ShapeOffsetsPortal& shapeOffsets,
                                   const InIndicesPortalType& indices,
                                   const OutIndicesPortalType& outputIndices,
                                   const vtkm::Id& outputOffset) const
  {
    CellTables tables;

    vtkm::Id cellId = faceIdPair[0];
    BOUNDS_CHECK(shapeOffsets, cellId);
    vtkm::Id offset = shapeOffsets.Get(cellId);
    BOUNDS_CHECK(shapes, cellId);
    vtkm::Int32 shapeId = static_cast<vtkm::Int32>(shapes.Get(cellId));
    vtkm::Int32 shapesFaceOffset = tables.FaceLookUp(tables.CellTypeLookUp(shapeId), 0);
    if (shapesFaceOffset == -1)
    {
      printf("Unsupported Shape Type %d\n", shapeId);
      return;
    }

    vtkm::Vec<vtkm::Id, 4> faceIndices(-1, -1, -1, -1);
    vtkm::Int32 tableIndex = static_cast<vtkm::Int32>(shapesFaceOffset + faceIdPair[1]);
    const vtkm::Int32 numIndices = tables.ShapesFaceList(tableIndex, 0);

    for (vtkm::Int32 i = 1; i <= numIndices; ++i)
    {
      BOUNDS_CHECK(indices, offset + tables.ShapesFaceList(tableIndex, i));
      faceIndices[i - 1] = indices.Get(offset + tables.ShapesFaceList(tableIndex, i));
    }
    vtkm::Vec<vtkm::Id, 4> triangle;
    triangle[0] = cellId;
    triangle[1] = faceIndices[0];
    triangle[2] = faceIndices[1];
    triangle[3] = faceIndices[2];
    BOUNDS_CHECK(outputIndices, outputOffset);
    outputIndices.Set(outputOffset, triangle);
    if (numIndices == 4)
    {
      triangle[1] = faceIndices[0];
      triangle[2] = faceIndices[2];
      triangle[3] = faceIndices[3];

      BOUNDS_CHECK(outputIndices, outputOffset + 1);
      outputIndices.Set(outputOffset + 1, triangle);
    }
  }
}; //class External Triangles

// Face conn was originally used for filtering out internal
// faces and was sorted with faces. To make it usable,
// we need to scatter back the connectivity into the original
// cell order, i.e., conn for cell 0 at 0,1,2,3,4,5
class WriteFaceConn : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn<>, WholeArrayIn<>, WholeArrayOut<IdType>);
  using ExecutionSignature = void(_1, _2, _3);

  VTKM_CONT
  WriteFaceConn() {}

  template <typename FaceOffsetsPortalType, typename FaceConnectivityPortalType>
  VTKM_EXEC inline void operator()(const vtkm::Vec<vtkm::Id, 3>& faceIdPair,
                                   const FaceOffsetsPortalType& faceOffsets,
                                   FaceConnectivityPortalType& faceConn) const
  {
    vtkm::Id cellId = faceIdPair[0];
    BOUNDS_CHECK(faceOffsets, cellId);
    vtkm::Id faceOffset = faceOffsets.Get(cellId) + faceIdPair[1]; // cellOffset ++ faceId
    BOUNDS_CHECK(faceConn, faceOffset);
    faceConn.Set(faceOffset, faceIdPair[2]);
  }
}; //class WriteFaceConn

class StructuredExternalTriangles : public vtkm::worklet::WorkletMapField
{
protected:
  using ConnType = vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                                      vtkm::TopologyElementTagCell,
                                                      3>;
  ConnType Connectivity;
  vtkm::Id Segments[7];
  vtkm::Id3 CellDims;

public:
  VTKM_CONT
  StructuredExternalTriangles(const ConnType& connectivity)
    : Connectivity(connectivity)
  {
    vtkm::Id3 cellDims = Connectivity.GetPointDimensions();
    cellDims[0] -= 1;
    cellDims[1] -= 1;
    cellDims[2] -= 1;
    CellDims = cellDims;

    //We have 6 segments for each of the six faces.
    Segments[0] = 0;
    // 0-1 = the two faces parallel to the x-z plane
    Segments[1] = cellDims[0] * cellDims[2];
    Segments[2] = Segments[1] + Segments[1];
    // 2-3 parallel to the y-z plane
    Segments[3] = Segments[2] + cellDims[1] * cellDims[2];
    Segments[4] = Segments[3] + cellDims[1] * cellDims[2];
    // 4-5 parallel to the x-y plane
    Segments[5] = Segments[4] + cellDims[1] * cellDims[0];
    Segments[6] = Segments[5] + cellDims[1] * cellDims[0];
  }
  using ControlSignature = void(FieldIn<>, WholeArrayOut<>);
  using ExecutionSignature = void(_1, _2);
  template <typename TrianglePortalType>
  VTKM_EXEC inline void operator()(const vtkm::Id& index, TrianglePortalType& triangles) const
  {
    // Each one of size segments will process
    // one face of the hex and domain
    VTKM_STATIC_CONSTEXPR_ARRAY vtkm::Int32 SegmentToFace[6] = { 0, 2, 1, 3, 4, 5 };

    // Each face/segment has 2 varying dimensions
    VTKM_STATIC_CONSTEXPR_ARRAY vtkm::Int32 SegmentDirections[6][2] = {
      { 0, 2 },           //face 0 and 2 have the same directions
      { 0, 2 }, { 1, 2 }, //1 and 3
      { 1, 2 }, { 0, 1 }, // 4 and 5
      { 0, 1 }
    };

    //
    // We get one index per extenal face
    //

    //
    // Cell face to extract which is also the domain
    // face in this segment
    //

    vtkm::Int32 segment;

    for (segment = 0; index >= Segments[segment + 1]; ++segment)
      ;

    if (segment >= 6)
    {
      printf("OUT OF BOUDNS %d", (int)index);
    }
    vtkm::Int32 cellFace = SegmentToFace[segment];

    // Face relative directions of the
    // 2 varying coordinates.
    vtkm::Int32 dir1, dir2;
    dir1 = SegmentDirections[segment][0];
    dir2 = SegmentDirections[segment][1];

    // For each face, we will have a relative offset to
    // the "bottom corner" of the face. Three are at the
    // origin and we have to adjust for the other faces.
    vtkm::Id3 cellIndex(0, 0, 0);
    if (cellFace == 1)
      cellIndex[0] = CellDims[0] - 1;
    if (cellFace == 2)
      cellIndex[1] = CellDims[1] - 1;
    if (cellFace == 5)
      cellIndex[2] = CellDims[2] - 1;


    // index is the global index of all external faces
    // the offset is the relative index of the cell
    // on the current 2d face

    vtkm::Id offset = index - Segments[segment];
    vtkm::Id dir1Offset = offset % CellDims[dir1];
    vtkm::Id dir2Offset = offset / CellDims[dir1];

    cellIndex[dir1] = cellIndex[dir1] + dir1Offset;
    cellIndex[dir2] = cellIndex[dir2] + dir2Offset;
    vtkm::Id cellId = Connectivity.LogicalToFlatToIndex(cellIndex);
    vtkm::VecVariable<vtkm::Id, 8> cellIndices = Connectivity.GetIndices(cellId);

    // Look up the offset into the face list for each cell type
    // This should always be zero, but in case this table changes I don't
    // want to break anything.
    CellTables tables;
    vtkm::Int32 shapesFaceOffset =
      tables.FaceLookUp(tables.CellTypeLookUp(CELL_SHAPE_HEXAHEDRON), 0);

    vtkm::Vec<vtkm::Id, 4> faceIndices;
    vtkm::Int32 tableIndex = shapesFaceOffset + cellFace;

    // Load the face
    for (vtkm::Int32 i = 1; i <= 4; ++i)
    {
      faceIndices[i - 1] = cellIndices[tables.ShapesFaceList(tableIndex, i)];
    }
    const vtkm::Id outputOffset = index * 2;
    vtkm::Vec<vtkm::Id, 4> triangle;
    triangle[0] = cellId;
    triangle[1] = faceIndices[0];
    triangle[2] = faceIndices[1];
    triangle[3] = faceIndices[2];
    BOUNDS_CHECK(triangles, outputOffset);
    triangles.Set(outputOffset, triangle);
    triangle[1] = faceIndices[0];
    triangle[2] = faceIndices[2];
    triangle[3] = faceIndices[3];
    BOUNDS_CHECK(triangles, outputOffset);
    triangles.Set(outputOffset + 1, triangle);
  }
}; //class  StructuredExternalTriangles

//If with output faces or triangles, we still have to calculate the size of the output
//array. TODO: switch to faces only
class CountExternalTriangles : public vtkm::worklet::WorkletMapField
{
public:
  VTKM_CONT
  CountExternalTriangles() {}
  using ControlSignature = void(FieldIn<>, WholeArrayIn<>, FieldOut<>);
  using ExecutionSignature = void(_1, _2, _3);
  template <typename ShapePortalType>
  VTKM_EXEC inline void operator()(const vtkm::Vec<vtkm::Id, 3>& faceIdPair,
                                   const ShapePortalType& shapes,
                                   vtkm::Id& triangleCount) const
  {
    CellTables tables;
    vtkm::Id cellId = faceIdPair[0];
    vtkm::Id cellFace = faceIdPair[1];
    vtkm::Int32 shapeType = static_cast<vtkm::Int32>(shapes.Get(cellId));
    vtkm::Int32 faceStartIndex = tables.FaceLookUp(tables.CellTypeLookUp(shapeType), 0);
    if (faceStartIndex == -1)
    {
      //Unsupported Shape Type this should never happen
      triangleCount = 0;
      return;
    }
    vtkm::Int32 faceType =
      tables.ShapesFaceList(faceStartIndex + static_cast<vtkm::Int32>(cellFace), 0);
    // The face will either have 4 or 3 indices, so quad or tri
    triangleCount = (faceType == 4) ? 2 : 1;

    //faceConn.Set(faceOffset, faceIdPair[2]);
  }
}; //class WriteFaceConn


template <typename DeviceAdapter>
class MeshConnectivityBuilder
{
public:
  MeshConnectivityBuilder() {}
  ~MeshConnectivityBuilder() {}


  VTKM_CONT
  void BuildConnectivity(vtkm::cont::CellSetSingleType<>& cellSetUnstructured,
                         const vtkm::cont::ArrayHandleVirtualCoordinates& coordinates,
                         vtkm::Bounds coordsBounds)
  {
    Logger* logger = Logger::GetInstance();
    logger->OpenLogEntry("mesh_conn");
    logger->AddLogData("device", GetDeviceString(DeviceAdapter()));
    vtkm::cont::Timer<DeviceAdapter> timer;

    vtkm::Float32 BoundingBox[6];
    BoundingBox[0] = vtkm::Float32(coordsBounds.X.Min);
    BoundingBox[1] = vtkm::Float32(coordsBounds.X.Max);
    BoundingBox[2] = vtkm::Float32(coordsBounds.Y.Min);
    BoundingBox[3] = vtkm::Float32(coordsBounds.Y.Max);
    BoundingBox[4] = vtkm::Float32(coordsBounds.Z.Min);
    BoundingBox[5] = vtkm::Float32(coordsBounds.Z.Max);

    const vtkm::cont::ArrayHandleConstant<vtkm::UInt8> shapes = cellSetUnstructured.GetShapesArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    const vtkm::cont::ArrayHandle<vtkm::Id> conn = cellSetUnstructured.GetConnectivityArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    const vtkm::cont::ArrayHandleCounting<vtkm::Id> shapeOffsets =
      cellSetUnstructured.GetIndexOffsetArray(vtkm::TopologyElementTagPoint(),
                                              vtkm::TopologyElementTagCell());

    vtkm::cont::ArrayHandle<vtkm::Id> faceConnectivity;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 3>> cellFaceId;

    this->GenerateFaceConnnectivity(cellSetUnstructured,
                                    shapes,
                                    conn,
                                    shapeOffsets,
                                    coordinates,
                                    faceConnectivity,
                                    cellFaceId,
                                    BoundingBox);

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> externalTriangles;
    //Extenal Faces

    externalTriangles =
      this->ExtractExternalFaces(cellFaceId, faceConnectivity, shapes, conn, shapeOffsets);


    // scatter the coonectivity into the original order
    vtkm::worklet::DispatcherMapField<WriteFaceConn, DeviceAdapter>(WriteFaceConn())
      .Invoke(cellFaceId, this->FaceOffsets, faceConnectivity);


    FaceConnectivity = faceConnectivity;
    OutsideTriangles = externalTriangles;

    vtkm::Float64 time = timer.GetElapsedTime();
    logger->CloseLogEntry(time);
  }

  VTKM_CONT
  void BuildConnectivity(vtkm::cont::CellSetExplicit<>& cellSetUnstructured,
                         const vtkm::cont::ArrayHandleVirtualCoordinates& coordinates,
                         vtkm::Bounds coordsBounds)
  {
    Logger* logger = Logger::GetInstance();
    logger->OpenLogEntry("meah_conn");
    logger->AddLogData("device", GetDeviceString(DeviceAdapter()));
    vtkm::cont::Timer<DeviceAdapter> timer;

    vtkm::Float32 BoundingBox[6];
    BoundingBox[0] = vtkm::Float32(coordsBounds.X.Min);
    BoundingBox[1] = vtkm::Float32(coordsBounds.X.Max);
    BoundingBox[2] = vtkm::Float32(coordsBounds.Y.Min);
    BoundingBox[3] = vtkm::Float32(coordsBounds.Y.Max);
    BoundingBox[4] = vtkm::Float32(coordsBounds.Z.Min);
    BoundingBox[5] = vtkm::Float32(coordsBounds.Z.Max);

    const vtkm::cont::ArrayHandle<vtkm::UInt8> shapes = cellSetUnstructured.GetShapesArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    const vtkm::cont::ArrayHandle<vtkm::Id> conn = cellSetUnstructured.GetConnectivityArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    const vtkm::cont::ArrayHandle<vtkm::Id> shapeOffsets = cellSetUnstructured.GetIndexOffsetArray(
      vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell());

    vtkm::cont::ArrayHandle<vtkm::Id> faceConnectivity;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 3>> cellFaceId;

    this->GenerateFaceConnnectivity(cellSetUnstructured,
                                    shapes,
                                    conn,
                                    shapeOffsets,
                                    coordinates,
                                    faceConnectivity,
                                    cellFaceId,
                                    BoundingBox);

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> externalTriangles;
    //
    //Extenal Faces
    externalTriangles =
      this->ExtractExternalFaces(cellFaceId, faceConnectivity, shapes, conn, shapeOffsets);

    // scatter the coonectivity into the original order
    vtkm::worklet::DispatcherMapField<WriteFaceConn, DeviceAdapter>(WriteFaceConn())
      .Invoke(cellFaceId, this->FaceOffsets, faceConnectivity);

    FaceConnectivity = faceConnectivity;
    OutsideTriangles = externalTriangles;

    vtkm::Float64 time = timer.GetElapsedTime();
    logger->CloseLogEntry(time);
  }

  // Should we just make this name BuildConnectivity?
  VTKM_CONT
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> ExternalTrianglesStructured(
    vtkm::cont::CellSetStructured<3>& cellSetStructured)
  {
    vtkm::cont::Timer<DeviceAdapter> timer;

    vtkm::Id3 cellDims = cellSetStructured.GetCellDimensions();
    vtkm::Id numFaces =
      cellDims[0] * cellDims[1] * 2 + cellDims[1] * cellDims[2] * 2 + cellDims[2] * cellDims[0] * 2;

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> triangles;
    triangles.PrepareForOutput(numFaces * 2, DeviceAdapter());
    vtkm::cont::ArrayHandleCounting<vtkm::Id> counting(0, 1, numFaces);

    vtkm::worklet::DispatcherMapField<StructuredExternalTriangles, DeviceAdapter>(
      StructuredExternalTriangles(cellSetStructured.PrepareForInput(
        DeviceAdapter(), vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell())))
      .Invoke(counting, triangles);

    vtkm::Float64 time = timer.GetElapsedTime();
    Logger::GetInstance()->AddLogData("structured_external_faces", time);

    return triangles;
  }

  vtkm::cont::ArrayHandle<vtkm::Id> GetFaceConnectivity() { return FaceConnectivity; }

  vtkm::cont::ArrayHandle<vtkm::Id> GetFaceOffsets() { return FaceOffsets; }

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> GetExternalTriangles()
  {
    return OutsideTriangles;
  }

protected:
  template <typename CellSetType,
            typename ShapeHandleType,
            typename ConnHandleType,
            typename OffsetsHandleType>
  VTKM_CONT void GenerateFaceConnnectivity(
    const CellSetType cellSet,
    const ShapeHandleType shapes,
    const ConnHandleType conn,
    const OffsetsHandleType shapeOffsets,
    const vtkm::cont::ArrayHandleVirtualCoordinates& coords,
    vtkm::cont::ArrayHandle<vtkm::Id>& faceConnectivity,
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 3>>& cellFaceId,
    vtkm::Float32 BoundingBox[6])
  {

    vtkm::cont::Timer<DeviceAdapter> timer;

    vtkm::Id numCells = shapes.GetNumberOfValues();

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> coordinates;
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy(coords, coordinates);

    /*-----------------------------------------------------------------*/

    // Count the number of total faces in the cell set
    vtkm::cont::ArrayHandle<vtkm::Id> facesPerCell;

    vtkm::worklet::DispatcherMapField<CountFaces, DeviceAdapter>(CountFaces())
      .Invoke(shapes, facesPerCell);

    vtkm::Id totalFaces = 0;
    totalFaces =
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Reduce(facesPerCell, vtkm::Id(0));

    // Calculate the offsets so each cell knows where to insert the morton code
    // for each face
    vtkm::cont::ArrayHandle<vtkm::Id> cellOffsets;
    cellOffsets.PrepareForOutput(numCells, DeviceAdapter());
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanExclusive(facesPerCell, cellOffsets);
    // cell offsets also serve as the offsets into the array that tracks connectivity.
    // For example, we have a hex with 6 faces and each face connects to another cell.
    // The connecting cells (from each face) are stored beginning at index cellOffsets[cellId]
    this->FaceOffsets = cellOffsets;

    // We are creating a spatial hash based on morton codes calculated
    // from the centriod (point average)  of each face. Each centroid is
    // calculated in way (consistent order of floating point calcs) that
    // ensures that each face maps to the same morton code. It is possbilbe
    // that two non-connecting faces map to the same morton code,  but if
    // if a face has a matching face from another cell, it will be mapped
    // to the same morton code. We check for this.

    // set up everything we need to gen morton codes
    vtkm::Vec<vtkm::Float32, 3> inverseExtent;
    inverseExtent[0] = 1.f / (BoundingBox[1] - BoundingBox[0]);
    inverseExtent[1] = 1.f / (BoundingBox[3] - BoundingBox[2]);
    inverseExtent[2] = 1.f / (BoundingBox[5] - BoundingBox[4]);
    vtkm::Vec<vtkm::Float32, 3> minPoint;
    minPoint[0] = BoundingBox[0];
    minPoint[1] = BoundingBox[2];
    minPoint[2] = BoundingBox[4];

    // Morton Codes are created for the centroid of each face.
    // cellFaceId:
    //  0) Cell that the face belongs to
    //  1) Face of the the cell (e.g., hex will have 6 faces and this is 1 of the 6)
    //  2) cell id of the cell that connects to the corresponding face (1)
    vtkm::cont::ArrayHandle<vtkm::UInt32> faceMortonCodes;
    cellFaceId.PrepareForOutput(totalFaces, DeviceAdapter());
    faceMortonCodes.PrepareForOutput(totalFaces, DeviceAdapter());
    vtkm::worklet::DispatcherMapTopology<MortonCodeFace, DeviceAdapter>(
      MortonCodeFace(inverseExtent, minPoint))
      .Invoke(cellSet, coordinates, cellOffsets, faceMortonCodes, cellFaceId);

    // Sort the "faces" (i.e., morton codes)
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::SortByKey(faceMortonCodes, cellFaceId);

    // Allocate space for the final face-to-face conectivity
    faceConnectivity.PrepareForOutput(totalFaces, DeviceAdapter());

    // Initialize All connecting faces to -1 (connects to nothing)
    vtkm::worklet::DispatcherMapField<MemSet<vtkm::Id>, DeviceAdapter>(MemSet<vtkm::Id>(-1))
      .Invoke(faceConnectivity);

    vtkm::worklet::DispatcherMapField<MortonNeighbor, DeviceAdapter>(MortonNeighbor())
      .Invoke(faceMortonCodes, cellFaceId, conn, shapes, shapeOffsets, faceConnectivity);

    vtkm::Float64 time = timer.GetElapsedTime();
    Logger::GetInstance()->AddLogData("gen_face_conn", time);
  }

  template <typename ShapeHandleType, typename OffsetsHandleType, typename ConnHandleType>
  VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Vec<Id, 4>> ExtractExternalFaces(
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 3>>
      cellFaceId, // Map of cell, face, and connecting cell
    vtkm::cont::ArrayHandle<vtkm::Id>
      faceConnectivity, // -1 if the face does not connect to any other face
    const ShapeHandleType& shapes,
    const ConnHandleType& conn,
    const OffsetsHandleType& shapeOffsets)
  {

    vtkm::cont::Timer<DeviceAdapter> timer;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 3>> externalFacePairs;
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::CopyIf(
      cellFaceId, faceConnectivity, externalFacePairs, IsExternal());

    // We need to count the number of triangle per external face
    // If it is a single cell type and it is a tet or hex, this is a special case
    // i.e., we do not need to calculate it. TODO
    // Otherwise, we need to check to see if the face is a quad or triangle

    vtkm::Id numExternalFaces = externalFacePairs.GetNumberOfValues();

    vtkm::cont::ArrayHandle<vtkm::Id> trianglesPerExternalFace;
    trianglesPerExternalFace.PrepareForOutput(numExternalFaces, DeviceAdapter());


    vtkm::worklet::DispatcherMapField<CountExternalTriangles, DeviceAdapter>(
      CountExternalTriangles())
      .Invoke(externalFacePairs, shapes, trianglesPerExternalFace);

    vtkm::cont::ArrayHandle<vtkm::Id> externalTriangleOffsets;
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ScanExclusive(trianglesPerExternalFace,
                                                                     externalTriangleOffsets);

    vtkm::Id totalExternalTriangles;
    totalExternalTriangles = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Reduce(
      trianglesPerExternalFace, vtkm::Id(0));
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> externalTriangles;
    externalTriangles.PrepareForOutput(totalExternalTriangles, DeviceAdapter());
    //count the number triangles in the external faces

    vtkm::worklet::DispatcherMapField<ExternalTriangles, DeviceAdapter>(ExternalTriangles())
      .Invoke(
        externalFacePairs, shapes, shapeOffsets, conn, externalTriangles, externalTriangleOffsets);


    vtkm::Float64 time = timer.GetElapsedTime();
    Logger::GetInstance()->AddLogData("external_faces", time);
    return externalTriangles;
  }


  vtkm::cont::ArrayHandle<vtkm::Id> FaceConnectivity;
  vtkm::cont::ArrayHandle<vtkm::Id> FaceOffsets;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id, 4>> OutsideTriangles;
};
}
}
} //namespace vtkm::rendering::raytracing
#endif
