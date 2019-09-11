//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_MortonCodes_h
#define vtk_m_rendering_raytracing_MortonCodes_h

#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/rendering/raytracing/CellTables.h>
#include <vtkm/rendering/raytracing/RayTracingTypeDefs.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{


//Note: if this takes a long time. we could use a lookup table
//expands 10-bit unsigned int into 30 bits
VTKM_EXEC inline vtkm::UInt32 ExpandBits32(vtkm::UInt32 x32)
{
  x32 = (x32 | (x32 << 16)) & 0x030000FF;
  x32 = (x32 | (x32 << 8)) & 0x0300F00F;
  x32 = (x32 | (x32 << 4)) & 0x030C30C3;
  x32 = (x32 | (x32 << 2)) & 0x09249249;
  return x32;
}

VTKM_EXEC inline vtkm::UInt64 ExpandBits64(vtkm::UInt32 x)
{
  vtkm::UInt64 x64 = x & 0x1FFFFF;
  x64 = (x64 | x64 << 32) & 0x1F00000000FFFF;
  x64 = (x64 | x64 << 16) & 0x1F0000FF0000FF;
  x64 = (x64 | x64 << 8) & 0x100F00F00F00F00F;
  x64 = (x64 | x64 << 4) & 0x10c30c30c30c30c3;
  x64 = (x64 | x64 << 2) & 0x1249249249249249;

  return x64;
}

//Returns 30 bit morton code for coordinates for
//coordinates in the unit cude
VTKM_EXEC inline vtkm::UInt32 Morton3D(vtkm::Float32& x, vtkm::Float32& y, vtkm::Float32& z)
{
  //take the first 10 bits
  x = vtkm::Min(vtkm::Max(x * 1024.0f, 0.0f), 1023.0f);
  y = vtkm::Min(vtkm::Max(y * 1024.0f, 0.0f), 1023.0f);
  z = vtkm::Min(vtkm::Max(z * 1024.0f, 0.0f), 1023.0f);
  //expand the 10 bits to 30
  vtkm::UInt32 xx = ExpandBits32((vtkm::UInt32)x);
  vtkm::UInt32 yy = ExpandBits32((vtkm::UInt32)y);
  vtkm::UInt32 zz = ExpandBits32((vtkm::UInt32)z);
  //interleave coordinates
  return (zz << 2 | yy << 1 | xx);
}

//Returns 30 bit morton code for coordinates for
//coordinates in the unit cude
VTKM_EXEC inline vtkm::UInt64 Morton3D64(vtkm::Float32& x, vtkm::Float32& y, vtkm::Float32& z)
{
  //take the first 21 bits
  x = vtkm::Min(vtkm::Max(x * 2097152.0f, 0.0f), 2097151.0f);
  y = vtkm::Min(vtkm::Max(y * 2097152.0f, 0.0f), 2097151.0f);
  z = vtkm::Min(vtkm::Max(z * 2097152.0f, 0.0f), 2097151.0f);
  //expand the 10 bits to 30
  vtkm::UInt64 xx = ExpandBits64((vtkm::UInt32)x);
  vtkm::UInt64 yy = ExpandBits64((vtkm::UInt32)y);
  vtkm::UInt64 zz = ExpandBits64((vtkm::UInt32)z);
  //interleave coordinates
  return (zz << 2 | yy << 1 | xx);
}

class MortonCodeFace : public vtkm::worklet::WorkletVisitCellsWithPoints
{
private:
  // (1.f / dx),(1.f / dy), (1.f, / dz)
  vtkm::Vec3f_32 InverseExtent;
  vtkm::Vec3f_32 MinCoordinate;

  VTKM_EXEC inline void Normalize(vtkm::Vec3f_32& point) const
  {
    point = (point - MinCoordinate) * InverseExtent;
  }

  VTKM_EXEC inline void Sort4(vtkm::Id4& indices) const
  {
    if (indices[0] < indices[1])
    {
      vtkm::Id temp = indices[1];
      indices[1] = indices[0];
      indices[0] = temp;
    }
    if (indices[2] < indices[3])
    {
      vtkm::Id temp = indices[3];
      indices[3] = indices[2];
      indices[2] = temp;
    }
    if (indices[0] < indices[2])
    {
      vtkm::Id temp = indices[2];
      indices[2] = indices[0];
      indices[0] = temp;
    }
    if (indices[1] < indices[3])
    {
      vtkm::Id temp = indices[3];
      indices[3] = indices[1];
      indices[1] = temp;
    }
    if (indices[1] < indices[2])
    {
      vtkm::Id temp = indices[2];
      indices[2] = indices[1];
      indices[1] = temp;
    }
  }

public:
  VTKM_CONT
  MortonCodeFace(const vtkm::Vec3f_32& inverseExtent, const vtkm::Vec3f_32& minCoordinate)
    : InverseExtent(inverseExtent)
    , MinCoordinate(minCoordinate)
  {
  }

  using ControlSignature =
    void(CellSetIn cellset, WholeArrayIn, FieldInCell, WholeArrayOut, WholeArrayOut);

  using ExecutionSignature = void(CellShape, IncidentElementIndices, WorkIndex, _2, _3, _4, _5);

  template <typename CellShape,
            typename CellNodeVecType,
            typename PointPortalType,
            typename MortonPortalType,
            typename CellFaceIdsPortalType>
  VTKM_EXEC inline void operator()(const CellShape& cellShape,
                                   const CellNodeVecType& cellIndices,
                                   const vtkm::Id& cellId,
                                   const PointPortalType& points,
                                   const vtkm::Id& offset,
                                   MortonPortalType& mortonCodes,
                                   CellFaceIdsPortalType& cellFaceIds) const
  {
    CellTables tables;
    vtkm::Int32 faceCount;
    vtkm::Int32 tableOffset;

    if (cellShape.Id == vtkm::CELL_SHAPE_TETRA)
    {
      faceCount = tables.FaceLookUp(1, 1);
      tableOffset = tables.FaceLookUp(1, 0);
    }
    else if (cellShape.Id == vtkm::CELL_SHAPE_HEXAHEDRON)
    {
      faceCount = tables.FaceLookUp(0, 1);
      tableOffset = tables.FaceLookUp(0, 0);
    }
    else if (cellShape.Id == vtkm::CELL_SHAPE_WEDGE)
    {
      faceCount = tables.FaceLookUp(2, 1);
      tableOffset = tables.FaceLookUp(2, 0);
    }
    else if (cellShape.Id == vtkm::CELL_SHAPE_PYRAMID)
    {
      faceCount = tables.FaceLookUp(3, 1);
      tableOffset = tables.FaceLookUp(3, 0);
    }
    else
    {
      printf("Unknown shape type %d\n", (int)cellShape.Id);
      return;
    }

    //calc the morton code at the center of each face
    for (vtkm::Int32 i = 0; i < faceCount; ++i)
    {
      vtkm::Vec3f_32 center;
      vtkm::UInt32 code;
      vtkm::Id3 cellFace;
      cellFace[0] = cellId;

      // We must be sure that this calculation is the same for all faces. If we didn't
      // then it is possible for the same face to end up in multiple morton "buckets" due to
      // the wonders of floating point math. This is bad. If we calculate in the same order
      // for all faces, then at worst, two different faces can enter the same bucket, which
      // we currently check for.
      vtkm::Id4 faceIndices(-1);
      //Number of indices this face has
      const vtkm::Int32 indiceCount = tables.ShapesFaceList(tableOffset + i, 0);
      for (vtkm::Int32 j = 1; j <= indiceCount; j++)
      {
        faceIndices[j - 1] = cellIndices[tables.ShapesFaceList(tableOffset + i, j)];
      }
      //sort the indices in descending order
      Sort4(faceIndices);

      vtkm::Int32 count = 1;
      BOUNDS_CHECK(points, faceIndices[0]);
      center = points.Get(faceIndices[0]);
      for (int idx = 1; idx < indiceCount; ++idx)
      {
        BOUNDS_CHECK(points, faceIndices[idx]);
        center = center + points.Get(faceIndices[idx]);
        count++;
      }
      //TODO: we could make this a recipical, but this is not a bottleneck.
      center[0] = center[0] / vtkm::Float32(count);
      center[1] = center[1] / vtkm::Float32(count);
      center[2] = center[2] / vtkm::Float32(count);
      Normalize(center);
      code = Morton3D(center[0], center[1], center[2]);
      BOUNDS_CHECK(mortonCodes, offset + i);
      mortonCodes.Set(offset + i, code);
      cellFace[1] = i;
      cellFace[2] = -1; //Need to initialize this for the  next step
      BOUNDS_CHECK(cellFaceIds, offset + i);
      cellFaceIds.Set(offset + i, cellFace);
    }
  }
}; // class MortonCodeFace

class MortonCodeAABB : public vtkm::worklet::WorkletMapField
{
private:
  // (1.f / dx),(1.f / dy), (1.f, / dz)
  vtkm::Vec3f_32 InverseExtent;
  vtkm::Vec3f_32 MinCoordinate;

public:
  VTKM_CONT
  MortonCodeAABB(const vtkm::Vec3f_32& inverseExtent, const vtkm::Vec3f_32& minCoordinate)
    : InverseExtent(inverseExtent)
    , MinCoordinate(minCoordinate)
  {
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6, _7);
  typedef _7 InputDomain;

  VTKM_EXEC
  void operator()(const vtkm::Float32& xmin,
                  const vtkm::Float32& ymin,
                  const vtkm::Float32& zmin,
                  const vtkm::Float32& xmax,
                  const vtkm::Float32& ymax,
                  const vtkm::Float32& zmax,
                  vtkm::UInt32& mortonCode) const
  {
    vtkm::Vec3f_32 direction(xmax - xmin, ymax - ymin, zmax - zmin);
    vtkm::Float32 halfDistance = sqrtf(vtkm::Dot(direction, direction)) * 0.5f;
    vtkm::Normalize(direction);
    vtkm::Float32 centroidx = xmin + halfDistance * direction[0] - MinCoordinate[0];
    vtkm::Float32 centroidy = ymin + halfDistance * direction[1] - MinCoordinate[1];
    vtkm::Float32 centroidz = zmin + halfDistance * direction[2] - MinCoordinate[2];
    //normalize the centroid tp 10 bits
    centroidx *= InverseExtent[0];
    centroidy *= InverseExtent[1];
    centroidz *= InverseExtent[2];
    mortonCode = Morton3D(centroidx, centroidy, centroidz);
  }
}; // class MortonCodeAABB
}
}
} //namespace vtkm::rendering::raytracing
#endif
