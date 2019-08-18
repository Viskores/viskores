//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_ContourTable_h
#define vtk_m_ContourTable_h

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace worklet
{
namespace internal
{

// clang-format off
VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent NumVerticesPerCellTable[] = {
  0, //  CELL_SHAPE_EMPTY = 0,
  0, //  CELL_SHAPE_VERTEX = 1,
  0, //  CELL_SHAPE_POLY_VERTEX = 2,
  0, //  CELL_SHAPE_LINE = 3,
  0, //  CELL_SHAPE_POLY_LINE = 4,
  3, //  CELL_SHAPE_TRIANGLE = 5,
  0, //  CELL_SHAPE_TRIANGLE_STRIP   = 6,
  0, //  CELL_SHAPE_POLYGON = 7,
  0, //  CELL_SHAPE_PIXEL = 8,
  4, //  CELL_SHAPE_QUAD = 9,
  4, //  CELL_SHAPE_TETRA = 10,
  0, //  CELL_SHAPE_VOXEL = 11,
  8, //  CELL_SHAPE_HEXAHEDRON = 12,
  6, //  CELL_SHAPE_WEDGE = 13,
  5  //  CELL_SHAPE_PYRAMID = 14,
};

VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent NumTrianglesTableOffset[] = {
  0, //  CELL_SHAPE_EMPTY = 0,
  0, //  CELL_SHAPE_VERTEX = 1,
  0, //  CELL_SHAPE_POLY_VERTEX = 2,
  0, //  CELL_SHAPE_LINE = 3,
  0, //  CELL_SHAPE_POLY_LINE = 4,
  0, //  CELL_SHAPE_TRIANGLE = 5,
  0, //  CELL_SHAPE_TRIANGLE_STRIP   = 6,
  0, //  CELL_SHAPE_POLYGON = 7,
  0, //  CELL_SHAPE_PIXEL = 8,
  0, //  CELL_SHAPE_QUAD = 9,
  0, //  CELL_SHAPE_TETRA = 10,
  0, //  CELL_SHAPE_VOXEL = 11,
  16, //  CELL_SHAPE_HEXAHEDRON = 12,
  16+256, //  CELL_SHAPE_WEDGE = 13,
  16+256+64  //  CELL_SHAPE_PYRAMID = 14,
};

VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent NumTrianglesTable[] = {
  // CELL_SHAPE_TETRA, case 0 - 15
  0, 1, 1, 2, 1, 2, 2, 1, 1, 2, 2, 1, 2, 1, 1, 0,
  // CELL_SHAPE_HEXAHEDRON, case 0 - 255
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 2,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
  2, 3, 3, 2, 3, 4, 4, 3, 3, 4, 4, 3, 4, 5, 5, 2,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4,
  2, 3, 3, 4, 3, 4, 2, 3, 3, 4, 4, 5, 4, 5, 3, 2,
  3, 4, 4, 3, 4, 5, 3, 2, 4, 5, 5, 4, 5, 2, 4, 1,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 2, 4, 3, 4, 3, 5, 2,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4,
  3, 4, 4, 3, 4, 5, 5, 4, 4, 3, 5, 2, 5, 4, 2, 1,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 2, 3, 3, 2,
  3, 4, 4, 5, 4, 5, 5, 2, 4, 3, 5, 4, 3, 2, 4, 1,
  3, 4, 4, 5, 4, 5, 3, 4, 4, 5, 5, 2, 3, 4, 2, 1,
  2, 3, 3, 2, 3, 4, 2, 1, 3, 2, 4, 1, 2, 1, 1, 0,
  // CELL_SHAPE_WEDGE, case 0 - 63
  0, 1, 1, 2, 1, 2, 1, 1, 1, 2, 2, 3, 2, 3, 3, 2,
  1, 2, 2, 3, 2, 3, 3, 2, 2, 3, 3, 2, 3, 4, 4, 1,
  1, 2, 2, 3, 2, 3, 3, 2, 2, 3, 3, 4, 3, 2, 4, 1,
  2, 3, 3, 4, 3, 4, 2, 1, 1, 2, 2, 1, 2, 1, 1, 0,
  // CELL_SHAPE_PYRAMID, case 0 - 31
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 2,
  2, 3, 3, 2, 3, 4, 2, 1, 3, 2, 4, 1, 2, 1, 1, 0
};

VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent EdgeTable[] = {
  // CELL_SHAPE_TETRA, 6 edge * 2 vertices/edge = 12 entries
  0, 1, // edge 0 : vertex 0 -> vertex 1
  1, 2, // edge 1 : vertex 1 -> vertex 2
  0, 2, // edge 2 : vertex 0 -> vertex 2
  0, 3, // edge 3 : vertex 0 -> vertex 3
  1, 3, // edge 4 : vertex 1 -> vertex 3
  2, 3, // edge 5 : vertex 2 -> vertex 3
  // CELL_SHAPE_HEXAHEDRON, 12 edges * 2 vertices/edge = 24 entries
  0, 1, // bottom layer
  1, 2,
  3, 2,
  0, 3,
  4, 5, // top layer
  5, 6,
  7, 6,
  4, 7,
  0, 4, // side
  1, 5,
  2, 6,
  3, 7,
  // CELL_SHAPE_WEDGE, 9 edges * 2 vertices/edge = 18 entries
  0, 1,
  1, 2,
  2, 0,
  3, 4,
  4, 5,
  5, 3,
  0, 3,
  1, 4,
  2, 5,
  // CELL_SHAPE_PYRAMID, 8 edges * 2 vertices/ede = 16 entries
  0, 1,
  1, 2,
  2, 3,
  3, 0,
  0, 4,
  1, 4,
  2, 4,
  3, 4
};

VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent EdgeTableOffset[] = {
  0, //  CELL_SHAPE_EMPTY = 0,
  0, //  CELL_SHAPE_VERTEX = 1,
  0, //  CELL_SHAPE_POLY_VERTEX = 2,
  0, //  CELL_SHAPE_LINE = 3,
  0, //  CELL_SHAPE_POLY_LINE = 4,
  0, //  CELL_SHAPE_TRIANGLE = 5,
  0, //  CELL_SHAPE_TRIANGLE_STRIP   = 6,
  0, //  CELL_SHAPE_POLYGON = 7,
  0, //  CELL_SHAPE_PIXEL = 8,
  0, //  CELL_SHAPE_QUAD = 9,
  0, //  CELL_SHAPE_TETRA = 10,
  0, //  CELL_SHAPE_VOXEL = 11,
  12, //  CELL_SHAPE_HEXAHEDRON = 12,
  12+24,    //  CELL_SHAPE_WEDGE = 13,
  12+24+18  //  CELL_SHAPE_PYRAMID = 14,
};

VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent TriangleTable[] = {
#define X -1
  // CELL_SHAPE_TETRA, 16 cases, 7 edges/cases, 112 entries total
  // FIXME, this is different winding rule than VTK
  X, X, X, X, X, X, X,
  0, 3, 2, X, X, X, X,
  0, 1, 4, X, X, X, X,
  1, 4, 2, 2, 4, 3, X,
  1, 2, 5, X, X, X, X,
  0, 3, 5, 0, 5, 1, X,
  0, 2, 5, 0, 5, 4, X,
  5, 4, 3, X, X, X, X,
  3, 4, 5, X, X, X, X,
  4, 5, 0, 5, 2, 0, X,
  1, 5, 0, 5, 3, 0, X,
  5, 2, 1, X, X, X, X,
  3, 4, 2, 2, 4, 1, X,
  4, 1, 0, X, X, X, X,
  2, 3, 0, X, X, X, X,
  X, X, X, X, X, X, X,

  // CELL_SHAPE_HEXAHEDRON, 256 cases, 16 edges/cases, 4096 entries total
  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  0,  8,  3,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  0,  1,  9,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  1,  8,  3,  9,  8,  1,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  1,  2,  10, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  0,  8,  3,  1,  2,  10, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  9,  2,  10, 0,  2,  9,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  2,  8,  3,  2,  10, 8,  10, 9,  8,  X,  X,  X,  X,  X,  X,  X,
  3,  11, 2,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  0,  11, 2,  8,  11, 0,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  1,  9,  0,  2,  3,  11, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  1,  11, 2,  1,  9,  11, 9,  8,  11, X,  X,  X,  X,  X,  X,  X,
  3,  10, 1,  11, 10, 3,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  0,  10, 1,  0,  8,  10, 8,  11, 10, X,  X,  X,  X,  X,  X,  X,
  3,  9,  0,  3,  11, 9,  11, 10, 9,  X,  X,  X,  X,  X,  X,  X,
  9,  8,  10, 10, 8,  11, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  4,  7,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  4,  3,  0,  7,  3,  4,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  0,  1,  9,  8,  4,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  4,  1,  9,  4,  7,  1,  7,  3,  1,  X,  X,  X,  X,  X,  X,  X,
  1,  2,  10, 8,  4,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  3,  4,  7,  3,  0,  4,  1,  2, 10,  X,  X,  X,  X,  X,  X,  X,
  9,  2,  10, 9,  0,  2,  8,  4,  7,  X,  X,  X,  X,  X,  X,  X,
  2,  10, 9,  2,  9,  7,  2,  7,  3,  7,  9,  4,  X,  X,  X,  X,
  8,  4,  7,  3,  11, 2,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  11, 4,  7,  11, 2,  4,  2,  0,
  4,  X,  X,  X,  X,  X,  X,  X,  9,  0,  1,  8,  4,  7,  2,  3,  11, X,  X,  X,  X,  X,  X,  X,
  4,  7,  11, 9,  4,  11, 9,  11, 2,  9,  2,  1,  X,  X,  X,  X,  3,  10, 1,  3,  11, 10, 7,  8,
  4,  X,  X,  X,  X,  X,  X,  X,  1,  11, 10, 1,  4,  11, 1,  0,  4,  7,  11, 4,  X,  X,  X,  X,
  4,  7,  8,  9,  0,  11, 9,  11, 10, 11, 0,  3,  X,  X,  X,  X,  4,  7,  11, 4,  11, 9,  9,  11,
  10, X,  X,  X,  X,  X,  X,  X,  9,  5,  4,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  9,  5,  4,  0,  8,  3,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  5,  4,  1,  5,  0,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  8,  5,  4,  8,  3,  5,  3,  1,  5,  X,  X,  X,  X,  X,  X,  X,
  1,  2,  10, 9,  5,  4,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  0,  8,  1,  2,  10, 4,  9,
  5,  X,  X,  X,  X,  X,  X,  X,  5,  2,  10, 5,  4,  2,  4,  0,  2,  X,  X,  X,  X,  X,  X,  X,
  2,  10, 5,  3,  2,  5,  3,  5,  4,  3,  4,  8,  X,  X,  X,  X,  9,  5,  4,  2,  3,  11, X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  0,  11, 2,  0,  8,  11, 4,  9,  5,  X,  X,  X,  X,  X,  X,  X,
  0,  5,  4,  0,  1,  5,  2,  3,  11, X,  X,  X,  X,  X,  X,  X,  2,  1,  5,  2,  5,  8,  2,  8,
  11, 4,  8,  5,  X,  X,  X,  X,  10, 3,  11, 10, 1,  3,  9,  5,  4,  X,  X,  X,  X,  X,  X,  X,
  4,  9,  5,  0,  8,  1,  8,  10, 1,  8,  11, 10, X,  X,  X,  X,  5,  4,  0,  5,  0,  11, 5,  11,
  10, 11, 0,  3,  X,  X,  X,  X,  5,  4,  8,  5,  8,  10, 10, 8,  11, X,  X,  X,  X,  X,  X,  X,
  9,  7,  8,  5,  7,  9,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  9,  3,  0,  9,  5,  3,  5,  7,
  3,  X,  X,  X,  X,  X,  X,  X,  0,  7,  8,  0,  1,  7,  1,  5,  7,  X,  X,  X,  X,  X,  X,  X,
  1,  5,  3,  3,  5,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  9,  7,  8,  9,  5,  7,  10, 1,
  2,  X,  X,  X,  X,  X,  X,  X,  10, 1,  2,  9,  5,  0,  5,  3,  0,  5,  7,  3,  X,  X,  X,  X,
  8,  0,  2,  8,  2,  5,  8,  5,  7,  10, 5,  2,  X,  X,  X,  X,  2,  10, 5,  2,  5,  3,  3,  5,
  7,  X,  X,  X,  X,  X,  X,  X,  7,  9,  5,  7,  8,  9,  3,  11, 2,  X,  X,  X,  X,  X,  X,  X,
  9,  5,  7,  9,  7,  2,  9,  2,  0,  2,  7,  11, X,  X,  X,  X,  2,  3,  11, 0,  1,  8,  1,  7,
  8,  1,  5,  7,  X,  X,  X,  X,  11, 2,  1,  11, 1,  7,  7,  1,  5,  X,  X,  X,  X,  X,  X,  X,
  9,  5,  8,  8,  5,  7,  10, 1,  3,  10, 3,  11, X,  X,  X,  X,  5,  7,  0,  5,  0,  9,  7,  11,
  0,  1,  0,  10, 11, 10, 0,  X,  11, 10, 0,  11, 0,  3,  10, 5,  0,  8,  0,  7,  5,  7,  0,  X,
  11, 10, 5,  7,  11, 5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  10, 6,  5,  X,  X,  X,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  0,  8,  3,  5,  10, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  9,  0,  1,  5,  10, 6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,  8,  3,  1,  9,  8,  5,  10,
  6,  X,  X,  X,  X,  X,  X,  X,  1,  6,  5,  2,  6,  1,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  1,  6,  5,  1,  2,  6,  3,  0,  8,  X,  X,  X,  X,  X,  X,  X,  9,  6,  5,  9,  0,  6,  0,  2,
  6,  X,  X,  X,  X,  X,  X,  X,  5,  9,  8,  5,  8,  2,  5,  2,  6,  3,  2,  8,  X,  X,  X,  X,
  2,  3,  11, 10, 6,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  11, 0,  8,  11, 2,  0,  10, 6,
  5,  X,  X,  X,  X,  X,  X,  X,  0,  1,  9,  2,  3,  11, 5,  10, 6,  X,  X,  X,  X,  X,  X,  X,
  5,  10, 6,  1,  9,  2,  9,  11, 2,  9,  8,  11, X,  X,  X,  X,  6,  3,  11, 6,  5,  3,  5,  1,
  3,  X,  X,  X,  X,  X,  X,  X,  0,  8,  11, 0,  11, 5,  0,  5,  1,  5,  11, 6,  X,  X,  X,  X,
  3,  11, 6,  0,  3,  6,  0,  6,  5,  0,  5,  9,  X,  X,  X,  X,  6,  5,  9,  6,  9,  11, 11, 9,
  8,  X,  X,  X,  X,  X,  X,  X,  5,  10, 6,  4,  7,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  4,  3,  0,  4,  7,  3,  6,  5,  10, X,  X,  X,  X,  X,  X,  X,  1,  9,  0,  5,  10, 6,  8,  4,
  7,  X,  X,  X,  X,  X,  X,  X,  10, 6,  5,  1,  9,  7,  1,  7,  3,  7,  9,  4,  X,  X,  X,  X,
  6,  1,  2,  6,  5,  1,  4,  7,  8,  X,  X,  X,  X,  X,  X,  X,  1,  2,  5,  5,  2,  6,  3,  0,
  4,  3,  4,  7,  X,  X,  X,  X,  8,  4,  7,  9,  0,  5,  0,  6,  5,  0,  2,  6,  X,  X,  X,  X,
  7,  3,  9,  7,  9,  4,  3,  2,  9,  5,  9,  6,  2,  6,  9,  X,  3,  11, 2,  7,  8,  4,  10, 6,
  5,  X,  X,  X,  X,  X,  X,  X,  5,  10, 6,  4,  7,  2,  4,  2,  0,  2,  7,  11, X,  X,  X,  X,
  0,  1,  9,  4,  7,  8,  2,  3,  11, 5,  10, 6,  X,  X,  X,  X,  9,  2,  1,  9,  11, 2,  9,  4,
  11, 7,  11, 4,  5,  10, 6,  X,  8,  4,  7,  3,  11, 5,  3,  5,  1,  5,  11, 6,  X,  X,  X,  X,
  5,  1,  11, 5,  11, 6,  1,  0,  11, 7,  11, 4,  0,  4,  11, X,  0,  5,  9,  0,  6,  5,  0,  3,
  6,  11, 6,  3,  8,  4,  7,  X,  6,  5,  9,  6,  9,  11, 4,  7,  9,  7,  11, 9,  X,  X,  X,  X,
  10, 4,  9,  6,  4,  10, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  4,  10, 6,  4,  9,  10, 0,  8,
  3,  X,  X,  X,  X,  X,  X,  X,  10, 0,  1,  10, 6,  0,  6,  4,  0,  X,  X,  X,  X,  X,  X,  X,
  8,  3,  1,  8,  1,  6,  8,  6,  4,  6,  1,  10, X,  X,  X,  X,  1,  4,  9,  1,  2,  4,  2,  6,
  4,  X,  X,  X,  X,  X,  X,  X,  3,  0,  8,  1,  2,  9,  2,  4,  9,  2,  6,  4,  X,  X,  X,  X,
  0,  2,  4,  4,  2,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  8,  3,  2,  8,  2,  4,  4,  2,
  6,  X,  X,  X,  X,  X,  X,  X,  10, 4,  9,  10, 6,  4,  11, 2,  3,  X,  X,  X,  X,  X,  X,  X,
  0,  8,  2,  2,  8,  11, 4,  9,  10, 4,  10, 6,  X,  X,  X,  X,  3,  11, 2,  0,  1,  6,  0,  6,
  4,  6,  1,  10, X,  X,  X,  X,  6,  4,  1,  6,  1,  10, 4,  8,  1,  2,  1,  11, 8,  11, 1,  X,
  9,  6,  4,  9,  3,  6,  9,  1,  3,  11, 6,  3,  X,  X,  X,  X,  8,  11, 1,  8,  1,  0,  11, 6,
  1,  9,  1,  4,  6,  4,  1,  X,  3,  11, 6,  3,  6,  0,  0,  6,  4,  X,  X,  X,  X,  X,  X,  X,
  6,  4,  8,  11, 6,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  7,  10, 6,  7,  8,  10, 8,  9,
  10, X,  X,  X,  X,  X,  X,  X,  0,  7,  3,  0,  10, 7,  0,  9,  10, 6,  7,  10, X,  X,  X,  X,
  10, 6,  7,  1,  10, 7,  1,  7,  8,  1,  8,  0,  X,  X,  X,  X,  10, 6,  7,  10, 7,  1,  1,  7,
  3,  X,  X,  X,  X,  X,  X,  X,  1,  2,  6,  1,  6,  8,  1,  8,  9,  8,  6,  7,  X,  X,  X,  X,
  2,  6,  9,  2,  9,  1,  6,  7,  9,  0,  9,  3,  7,  3,  9,  X,  7,  8,  0,  7,  0,  6,  6,  0,
  2,  X,  X,  X,  X,  X,  X,  X,  7,  3,  2,  6,  7,  2,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  2,  3,  11, 10, 6,  8,  10, 8,  9,  8,  6,  7,  X,  X,  X,  X,  2,  0,  7,  2,  7,  11, 0,  9,
  7,  6,  7,  10, 9,  10, 7,  X,  1,  8,  0,  1,  7,  8,  1,  10, 7,  6,  7,  10, 2,  3,  11, X,
  11, 2,  1,  11, 1,  7,  10, 6,  1,  6,  7,  1,  X,  X,  X,  X,  8,  9,  6,  8,  6,  7,  9,  1,
  6,  11, 6,  3,  1,  3,  6,  X,  0,  9,  1,  11, 6,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  7,  8,  0,  7,  0,  6,  3,  11, 0,  11, 6,  0,  X,  X,  X,  X,  7,  11, 6,  X,  X,  X,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  7,  6,  11, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  3,  0,  8,  11, 7,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  1,  9,  11, 7,  6,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  8,  1,  9,  8,  3,  1,  11, 7,  6,  X,  X,  X,  X,  X,  X,  X,
  10, 1,  2,  6,  11, 7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,  2,  10, 3,  0,  8,  6,  11,
  7,  X,  X,  X,  X,  X,  X,  X,  2,  9,  0,  2,  10, 9,  6,  11, 7,  X,  X,  X,  X,  X,  X,  X,
  6,  11, 7,  2,  10, 3,  10, 8,  3,  10, 9,  8,  X,  X,  X,  X,  7,  2,  3,  6,  2,  7,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  7,  0,  8,  7,  6,  0,  6,  2,  0,  X,  X,  X,  X,  X,  X,  X,
  2,  7,  6,  2,  3,  7,  0,  1,  9,  X,  X,  X,  X,  X,  X,  X,  1,  6,  2,  1,  8,  6,  1,  9,
  8,  8,  7,  6,  X,  X,  X,  X,  10, 7,  6,  10, 1,  7,  1,  3,  7,  X,  X,  X,  X,  X,  X,  X,
  10, 7,  6,  1,  7,  10, 1,  8,  7,  1,  0,  8,  X,  X,  X,  X,  0,  3,  7,  0,  7,  10, 0,  10,
  9,  6,  10, 7,  X,  X,  X,  X,  7,  6,  10, 7,  10, 8,  8,  10, 9,  X,  X,  X,  X,  X,  X,  X,
  6,  8,  4,  11, 8,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  6,  11, 3,  0,  6,  0,  4,
  6,  X,  X,  X,  X,  X,  X,  X,  8,  6,  11, 8,  4,  6,  9,  0,  1,  X,  X,  X,  X,  X,  X,  X,
  9,  4,  6,  9,  6,  3,  9,  3,  1,  11, 3,  6,  X,  X,  X,  X,  6,  8,  4,  6,  11, 8,  2,  10,
  1,  X,  X,  X,  X,  X,  X,  X,  1,  2,  10, 3,  0,  11, 0,  6,  11, 0,  4,  6,  X,  X,  X,  X,
  4,  11, 8,  4,  6,  11, 0,  2,  9,  2,  10, 9,  X,  X,  X,  X,  10, 9,  3,  10, 3,  2,  9,  4,
  3,  11, 3,  6,  4,  6,  3,  X,  8,  2,  3,  8,  4,  2,  4,  6,  2,  X,  X,  X,  X,  X,  X,  X,
  0,  4,  2,  4,  6,  2,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,  9,  0,  2,  3,  4,  2,  4,
  6,  4,  3,  8,  X,  X,  X,  X,  1,  9,  4,  1,  4,  2,  2,  4,  6,  X,  X,  X,  X,  X,  X,  X,
  8,  1,  3,  8,  6,  1,  8,  4,  6,  6,  10, 1,  X,  X,  X,  X,  10, 1,  0,  10, 0,  6,  6,  0,
  4,  X,  X,  X,  X,  X,  X,  X,  4,  6,  3,  4,  3,  8,  6,  10, 3,  0,  3,  9,  10, 9,  3,  X,
  10, 9,  4,  6,  10, 4,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  4,  9,  5,  7,  6,  11, X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  0,  8,  3,  4,  9,  5,  11, 7,  6,  X,  X,  X,  X,  X,  X,  X,
  5,  0,  1,  5,  4,  0,  7,  6,  11, X,  X,  X,  X,  X,  X,  X,  11, 7,  6,  8,  3,  4,  3,  5,
  4,  3,  1,  5,  X,  X,  X,  X,  9,  5,  4,  10, 1,  2,  7,  6,  11, X,  X,  X,  X,  X,  X,  X,
  6,  11, 7,  1,  2,  10, 0,  8,  3,  4,  9,  5,  X,  X,  X,  X,  7,  6,  11, 5,  4,  10, 4,  2,
  10, 4,  0,  2,  X,  X,  X,  X,  3,  4,  8,  3,  5,  4,  3,  2,  5,  10, 5,  2,  11, 7,  6,  X,
  7,  2,  3,  7,  6,  2,  5,  4,  9,  X,  X,  X,  X,  X,  X,  X,  9,  5,  4,  0,  8,  6,  0,  6,
  2,  6,  8,  7,  X,  X,  X,  X,  3,  6,  2,  3,  7,  6,  1,  5,  0,  5,  4,  0,  X,  X,  X,  X,
  6,  2,  8,  6,  8,  7,  2,  1,  8,  4,  8,  5,  1,  5,  8,  X,  9,  5,  4,  10, 1,  6,  1,  7,
  6,  1,  3,  7,  X,  X,  X,  X,  1,  6,  10, 1,  7,  6,  1,  0,  7,  8,  7,  0,  9,  5,  4,  X,
  4,  0,  10, 4,  10, 5,  0,  3,  10, 6,  10, 7,  3,  7,  10, X,  7,  6,  10, 7,  10, 8,  5,  4,
  10, 4,  8,  10, X,  X,  X,  X,  6,  9,  5,  6,  11, 9,  11, 8,  9,  X,  X,  X,  X,  X,  X,  X,
  3,  6,  11, 0,  6,  3,  0,  5,  6,  0,  9,  5,  X,  X,  X,  X,  0,  11, 8,  0,  5,  11, 0,  1,
  5,  5,  6,  11, X,  X,  X,  X,  6,  11, 3,  6,  3,  5,  5,  3,  1,  X,  X,  X,  X,  X,  X,  X,
  1,  2,  10, 9,  5,  11, 9,  11, 8,  11, 5,  6,  X,  X,  X,  X,  0,  11, 3,  0,  6,  11, 0,  9,
  6,  5,  6,  9,  1,  2,  10, X,  11, 8,  5,  11, 5,  6,  8,  0,  5,  10, 5,  2,  0,  2,  5,  X,
  6,  11, 3,  6,  3,  5,  2,  10, 3,  10, 5,  3,  X,  X,  X,  X,  5,  8,  9,  5,  2,  8,  5,  6,
  2,  3,  8,  2,  X,  X,  X,  X,  9,  5,  6,  9,  6,  0,  0,  6,  2,  X,  X,  X,  X,  X,  X,  X,
  1,  5,  8,  1,  8,  0,  5,  6,  8,  3,  8,  2,  6,  2,  8,  X,  1,  5,  6,  2,  1,  6,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  1,  3,  6,  1,  6,  10, 3,  8,  6,  5,  6,  9,  8,  9,  6,  X,
  10, 1,  0,  10, 0,  6,  9,  5,  0,  5,  6,  0,  X,  X,  X,  X,  0,  3,  8,  5,  6,  10, X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  10, 5,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  11, 5,  10, 7,  5,  11, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  11, 5,  10, 11, 7,  5,  8,  3,
  0,  X,  X,  X,  X,  X,  X,  X,  5,  11, 7,  5,  10, 11, 1,  9,  0,  X,  X,  X,  X,  X,  X,  X,
  10, 7,  5,  10, 11, 7,  9,  8,  1,  8,  3,  1,  X,  X,  X,  X,  11, 1,  2,  11, 7,  1,  7,  5,
  1,  X,  X,  X,  X,  X,  X,  X,  0,  8,  3,  1,  2,  7,  1,  7,  5,  7,  2,  11, X,  X,  X,  X,
  9,  7,  5,  9,  2,  7,  9,  0,  2,  2,  11, 7,  X,  X,  X,  X,  7,  5,  2,  7,  2,  11, 5,  9,
  2,  3,  2,  8,  9,  8,  2,  X,  2,  5,  10, 2,  3,  5,  3,  7,  5,  X,  X,  X,  X,  X,  X,  X,
  8,  2,  0,  8,  5,  2,  8,  7,  5,  10, 2,  5,  X,  X,  X,  X,  9,  0,  1,  5,  10, 3,  5,  3,
  7,  3,  10, 2,  X,  X,  X,  X,  9,  8,  2,  9,  2,  1,  8,  7,  2,  10, 2,  5,  7,  5,  2,  X,
  1,  3,  5,  3,  7,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  8,  7,  0,  7,  1,  1,  7,
  5,  X,  X,  X,  X,  X,  X,  X,  9,  0,  3,  9,  3,  5,  5,  3,  7,  X,  X,  X,  X,  X,  X,  X,
  9,  8,  7,  5,  9,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  5,  8,  4,  5,  10, 8,  10, 11,
  8,  X,  X,  X,  X,  X,  X,  X,  5,  0,  4,  5,  11, 0,  5,  10, 11, 11, 3,  0,  X,  X,  X,  X,
  0,  1,  9,  8,  4,  10, 8,  10, 11, 10, 4,  5,  X,  X,  X,  X,  10, 11, 4,  10, 4,  5,  11, 3,
  4,  9,  4,  1,  3,  1,  4,  X,  2,  5,  1,  2,  8,  5,  2,  11, 8,  4,  5,  8,  X,  X,  X,  X,
  0,  4,  11, 0,  11, 3,  4,  5,  11, 2,  11, 1,  5,  1,  11, X,  0,  2,  5,  0,  5,  9,  2,  11,
  5,  4,  5,  8,  11, 8,  5,  X,  9,  4,  5,  2,  11, 3,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  2,  5,  10, 3,  5,  2,  3,  4,  5,  3,  8,  4,  X,  X,  X,  X,  5,  10, 2,  5,  2,  4,  4,  2,
  0,  X,  X,  X,  X,  X,  X,  X,  3,  10, 2,  3,  5,  10, 3,  8,  5,  4,  5,  8,  0,  1,  9,  X,
  5,  10, 2,  5,  2,  4,  1,  9,  2,  9,  4,  2,  X,  X,  X,  X,  8,  4,  5,  8,  5,  3,  3,  5,
  1,  X,  X,  X,  X,  X,  X,  X,  0,  4,  5,  1,  0,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  8,  4,  5,  8,  5,  3,  9,  0,  5,  0,  3,  5,  X,  X,  X,  X,  9,  4,  5,  X,  X,  X,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  4,  11, 7,  4,  9,  11, 9,  10, 11, X,  X,  X,  X,  X,  X,  X,
  0,  8,  3,  4,  9,  7,  9,  11, 7,  9,  10, 11, X,  X,  X,  X,  1,  10, 11, 1,  11, 4,  1,  4,
  0,  7,  4,  11, X,  X,  X,  X,  3,  1,  4,  3,  4,  8,  1,  10, 4,  7,  4,  11, 10, 11, 4,  X,
  4,  11, 7,  9,  11, 4,  9,  2,  11, 9,  1,  2,  X,  X,  X,  X,  9,  7,  4,  9,  11, 7,  9,  1,
  11, 2,  11, 1,  0,  8,  3,  X,  11, 7,  4,  11, 4,  2,  2,  4,  0,  X,  X,  X,  X,  X,  X,  X,
  11, 7,  4,  11, 4,  2,  8,  3,  4,  3,  2,  4,  X,  X,  X,  X,  2,  9,  10, 2,  7,  9,  2,  3,
  7,  7,  4,  9,  X,  X,  X,  X,  9,  10, 7,  9,  7,  4,  10, 2,  7,  8,  7,  0,  2,  0,  7,  X,
  3,  7,  10, 3,  10, 2,  7,  4,  10, 1,  10, 0,  4,  0,  10, X,  1,  10, 2,  8,  7,  4,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  4,  9,  1,  4,  1,  7,  7,  1,  3,  X,  X,  X,  X,  X,  X,  X,
  4,  9,  1,  4,  1,  7,  0,  8,  1,  8,  7,  1,  X,  X,  X,  X,  4,  0,  3,  7,  4,  3,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  4,  8,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  9,  10, 8,  10, 11, 8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  0,  9,  3,  9,  11, 11, 9,
  10, X,  X,  X,  X,  X,  X,  X,  0,  1,  10, 0,  10, 8,  8,  10, 11, X,  X,  X,  X,  X,  X,  X,
  3,  1,  10, 11, 3,  10, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  1,  2,  11, 1,  11, 9,  9,  11,
  8,  X,  X,  X,  X,  X,  X,  X,  3,  0,  9,  3,  9,  11, 1,  2,  9,  2,  11, 9,  X,  X,  X,  X,
  0,  2,  11, 8,  0,  11, X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  3,  2,  11, X,  X,  X,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  2,  3,  8,  2,  8,  10, 10, 8,  9,  X,  X,  X,  X,  X,  X,  X,
  9,  10, 2,  0,  9,  2,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  2,  3,  8,  2,  8,  10, 0,  1,
  8,  1,  10, 8,  X,  X,  X,  X,  1,  10, 2,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  1,  3,  8,  9,  1,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  0,  9,  1,  X,  X,  X,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  0,  3,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,
  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,

  //  CELL_SHAPE_WEDGE = 13, 64 cases, 13 edges/case, 832 total entries
  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //0
  0,  6,  2,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //1
  0,  1,  7,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //2
  6,  1,  7,  6,  2,  1,  X,  X,  X,  X,  X,  X,  X, //3
  1,  2,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //4
  6,  1,  0,  6,  8,  1,  X,  X,  X,  X,  X,  X,  X, //5
  0,  2,  8,  7,  0,  8,  X,  X,  X,  X,  X,  X,  X, //6
  7,  6,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //7
  3,  5,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //8
  3,  5,  0,  5,  2,  0,  X,  X,  X,  X,  X,  X,  X, //9
  0,  1,  7,  6,  3,  5,  X,  X,  X,  X,  X,  X,  X, //10
  1,  7,  3,  1,  3,  5,  1,  5,  2,  X,  X,  X,  X, //11
  2,  8,  1,  6,  3,  5,  X,  X,  X,  X,  X,  X,  X, //12
  0,  3,  1,  1,  3,  5,  1,  5,  8,  X,  X,  X,  X, //13
  6,  3,  5,  0,  8,  7,  0,  2,  8,  X,  X,  X,  X, //14
  7,  3,  5,  7,  5,  8,  X,  X,  X,  X,  X,  X,  X, //15
  7,  4,  3,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //16
  7,  4,  3,  0,  6,  2,  X,  X,  X,  X,  X,  X,  X, //17
  0,  1,  3,  1,  4,  3,  X,  X,  X,  X,  X,  X,  X, //18
  1,  4,  3,  1,  3,  6,  1,  6,  2,  X,  X,  X,  X, //19
  7,  4,  3,  2,  8,  1,  X,  X,  X,  X,  X,  X,  X, //20
  7,  4,  3,  6,  1,  0,  6,  8,  1,  X,  X,  X,  X, //21
  0,  4,  3,  0,  8,  4,  0,  2,  8,  X,  X,  X,  X, //22
  6,  8,  3,  3,  8,  4,  X,  X,  X,  X,  X,  X,  X, //23
  6,  7,  4,  6,  4,  5,  X,  X,  X,  X,  X,  X,  X, //24
  0,  7,  5,  7,  4,  5,  2,  0,  5,  X,  X,  X,  X, //25
  1,  6,  0,  1,  5,  6,  1,  4,  5,  X,  X,  X,  X, //26
  2,  1,  5,  5,  1,  4,  X,  X,  X,  X,  X,  X,  X, //27
  2,  8,  1,  6,  7,  5,  7,  4,  5,  X,  X,  X,  X, //28
  0,  7,  5,  7,  4,  5,  0,  5,  1,  1,  5,  8,  X, //29
  0,  2,  8,  0,  8,  4,  0,  4,  5,  0,  5,  6,  X, //30
  8,  4,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //31
  4,  8,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //32
  4,  8,  5,  0,  6,  2,  X,  X,  X,  X,  X,  X,  X, //33
  4,  8,  5,  0,  1,  7,  X,  X,  X,  X,  X,  X,  X, //34
  4,  8,  5,  6,  1,  7,  6,  2,  1,  X,  X,  X,  X, //35
  1,  5,  4,  2,  5,  1,  X,  X,  X,  X,  X,  X,  X, //36
  1,  5,  4,  1,  6,  5,  1,  0,  6,  X,  X,  X,  X, //37
  5,  4,  7,  5,  7,  0,  5,  0,  2,  X,  X,  X,  X, //38
  6,  4,  7,  6,  5,  4,  X,  X,  X,  X,  X,  X,  X, //39
  6,  3,  8,  3,  4,  8,  X,  X,  X,  X,  X,  X,  X, //40
  0,  3,  4,  0,  4,  8,  0,  8,  2,  X,  X,  X,  X, //41
  7,  0,  1,  6,  3,  4,  6,  4,  8,  X,  X,  X,  X, //42
  1,  7,  3,  1,  3,  2,  2,  3,  8,  8,  3,  4,  X, //43
  2,  6,  1,  6,  3,  1,  3,  4,  1,  X,  X,  X,  X, //44
  0,  3,  1,  1,  3,  4,  X,  X,  X,  X,  X,  X,  X, //45
  7,  0,  4,  4,  0,  2,  4,  2,  3,  3,  2,  6,  X, //46
  7,  3,  4,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //47
  7,  8,  5,  7,  5,  3,  X,  X,  X,  X,  X,  X,  X, //48
  0,  6,  2,  7,  8,  5,  7,  5,  3,  X,  X,  X,  X, //49
  0,  1,  3,  1,  5,  3,  1,  8,  5,  X,  X,  X,  X, //50
  2,  1,  6,  6,  1,  3,  5,  1,  8,  3,  1,  5,  X, //51
  1,  3,  7,  1,  5,  3,  1,  2,  5,  X,  X,  X,  X, //52
  1,  0,  6,  1,  6,  5,  1,  5,  7,  7,  5,  3,  X, //53
  0,  2,  5,  0,  5,  3,  X,  X,  X,  X,  X,  X,  X, //54
  3,  6,  5,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //55
  7,  8,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //56
  0,  7,  8,  0,  8,  2,  X,  X,  X,  X,  X,  X,  X, //57
  0,  1,  6,  1,  8,  6,  X,  X,  X,  X,  X,  X,  X, //58
  2,  1,  8,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //59
  6,  7,  1,  6,  1,  2,  X,  X,  X,  X,  X,  X,  X, //60
  0,  7,  1,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //61
  0,  2,  6,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //62
  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //63

  //  CELL_SHAPE_PYRAMID = 14, 32 cases, 13 edges/case, 416 total entries
  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //0
  3,  4,  0,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //1
  5,  1,  0,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //2
  5,  1,  4,  1,  3,  4,  X,  X,  X,  X,  X,  X,  X, //3
  6,  2,  1,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //4
  3,  4,  0,  6,  2,  1,  X,  X,  X,  X,  X,  X,  X, //5
  5,  2,  0,  6,  2,  5,  X,  X,  X,  X,  X,  X,  X, //6
  2,  3,  4,  2,  4,  6,  4,  5,  6,  X,  X,  X,  X, //7
  2,  7,  3,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //8
  2,  7,  4,  4,  0,  2,  X,  X,  X,  X,  X,  X,  X, //9
  5,  1,  0,  2,  7,  3,  X,  X,  X,  X,  X,  X,  X, //10
  5,  7,  4,  1,  7,  5,  2,  7,  1,  X,  X,  X,  X, //11
  6,  3,  1,  7,  3,  6,  X,  X,  X,  X,  X,  X,  X, //12
  4,  6,  7,  0,  6,  4,  1,  6,  0,  X,  X,  X,  X, //13
  7,  5,  6,  3,  5,  7,  0,  5,  3,  X,  X,  X,  X, //14
  7,  4,  5,  7,  5,  6,  X,  X,  X,  X,  X,  X,  X, //15
  7,  5,  4,  7,  6,  5,  X,  X,  X,  X,  X,  X,  X, //16
  5,  0,  3,  6,  5,  3,  7,  6,  3,  X,  X,  X,  X, //17
  1,  0,  4,  7,  1,  4,  6,  1,  7,  X,  X,  X,  X, //18
  6,  1,  3,  7,  6,  3,  X,  X,  X,  X,  X,  X,  X, //19
  7,  5,  4,  7,  1,  5,  7,  2,  1,  X,  X,  X,  X, //20
  3,  7,  0,  7,  5,  0,  7,  2,  5,  2,  1,  5,  X, //21
  4,  2,  0,  7,  2,  4,  X,  X,  X,  X,  X,  X,  X, //22
  7,  2,  3,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //23
  2,  4,  3,  5,  4,  2,  6,  5,  2,  X,  X,  X,  X, //24
  2,  5,  0,  2,  6,  5,  X,  X,  X,  X,  X,  X,  X, //25
  6,  1,  0,  4,  6,  0,  3,  6,  4,  3,  2,  6,  X, //26
  2,  6,  1,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //27
  1,  4,  3,  1,  5,  4,  X,  X,  X,  X,  X,  X,  X, //28
  1,  5,  0,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //29
  4,  3,  0,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X, //30
  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X,  X  //31

#undef X
};

VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent TriangleTableOffset[] = {
  0, //  CELL_SHAPE_EMPTY = 0,
  0, //  CELL_SHAPE_VERTEX = 1,
  0, //  CELL_SHAPE_POLY_VERTEX = 2,
  0, //  CELL_SHAPE_LINE = 3,
  0, //  CELL_SHAPE_POLY_LINE = 4,
  0, //  CELL_SHAPE_TRIANGLE = 5,
  0, //  CELL_SHAPE_TRIANGLE_STRIP   = 6,
  0, //  CELL_SHAPE_POLYGON = 7,
  0, //  CELL_SHAPE_PIXEL = 8,
  0, //  CELL_SHAPE_QUAD = 9,
  0, //  CELL_SHAPE_TETRA = 10,
  0, //  CELL_SHAPE_VOXEL = 11,
  112, //  CELL_SHAPE_HEXAHEDRON = 12,
  112+4096, //  CELL_SHAPE_WEDGE = 13,
  112+4096+832, //  CELL_SHAPE_PYRAMID = 14,
};

// clang-format on
class CellClassifyTable : public vtkm::cont::ExecutionObjectBase
{
public:
  template <typename DeviceAdapter>
  class ExecObject
  {
  public:
    VTKM_EXEC
    vtkm::IdComponent GetNumVerticesPerCell(vtkm::Id shape) const
    {
      return this->NumVerticesPerCellPortal.Get(shape);
    }

    VTKM_EXEC
    vtkm::IdComponent GetNumTriangles(vtkm::Id shape, vtkm::IdComponent caseNumber) const
    {
      vtkm::IdComponent offset = this->NumTrianglesTableOffsetPortal.Get(shape);
      return this->NumTrianglesTablePortal.Get(offset + caseNumber);
    }

  private:
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<DeviceAdapter>::PortalConst
      NumVerticesPerCellPortal;
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<DeviceAdapter>::PortalConst
      NumTrianglesTablePortal;
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<DeviceAdapter>::PortalConst
      NumTrianglesTableOffsetPortal;

    friend class CellClassifyTable;
  };

  CellClassifyTable()
    : NumVerticesPerCellArray(
        vtkm::cont::make_ArrayHandle(NumVerticesPerCellTable, vtkm::NUMBER_OF_CELL_SHAPES))
    , NumTrianglesTableOffsetArray(
        vtkm::cont::make_ArrayHandle(NumTrianglesTableOffset, vtkm::NUMBER_OF_CELL_SHAPES))
    , NumTrianglesTableArray(
        vtkm::cont::make_ArrayHandle(NumTrianglesTable,
                                     sizeof(NumTrianglesTable) / sizeof(NumTrianglesTable[0])))
  {
  }

  template <typename DeviceAdapter>
  ExecObject<DeviceAdapter> PrepareForExecution(DeviceAdapter)
  {
    ExecObject<DeviceAdapter> execObject;
    execObject.NumVerticesPerCellPortal =
      this->NumVerticesPerCellArray.PrepareForInput(DeviceAdapter());
    execObject.NumTrianglesTableOffsetPortal =
      this->NumTrianglesTableOffsetArray.PrepareForInput(DeviceAdapter());
    execObject.NumTrianglesTablePortal =
      this->NumTrianglesTableArray.PrepareForInput(DeviceAdapter());
    return execObject;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumVerticesPerCellArray;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumTrianglesTableOffsetArray;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumTrianglesTableArray;
};

class TriangleGenerationTable : public vtkm::cont::ExecutionObjectBase
{
public:
  template <typename DeviceAdapter>
  class ExecObject
  {
  public:
    VTKM_EXEC
    vtkm::Pair<vtkm::IdComponent, vtkm::IdComponent> GetEdgeVertices(
      vtkm::Id shape,
      vtkm::IdComponent caseNumber,
      vtkm::IdComponent triangleNumber,
      vtkm::IdComponent vertexNumber) const
    {
      VTKM_STATIC_CONSTEXPR_ARRAY vtkm::IdComponent NumEntriesPerCase[] = {
        0,  //  CELL_SHAPE_EMPTY = 0,
        0,  //  CELL_SHAPE_VERTEX = 1,
        0,  //  CELL_SHAPE_POLY_VERTEX = 2,
        0,  //  CELL_SHAPE_LINE = 3,
        0,  //  CELL_SHAPE_POLY_LINE = 4,
        0,  //  CELL_SHAPE_TRIANGLE = 5,
        0,  //  CELL_SHAPE_TRIANGLE_STRIP   = 6,
        0,  //  CELL_SHAPE_POLYGON = 7,
        0,  //  CELL_SHAPE_PIXEL = 8,
        0,  //  CELL_SHAPE_QUAD = 9,
        7,  //  CELL_SHAPE_TETRA = 10,
        0,  //  CELL_SHAPE_VOXEL = 11,
        16, //  CELL_SHAPE_HEXAHEDRON = 12,
        13, //  CELL_SHAPE_WEDGE = 13,
        13, //  CELL_SHAPE_PYRAMID = 14,
      };

      vtkm::IdComponent triOffset = TriangleTableOffsetPortal.Get(shape) +
        NumEntriesPerCase[shape] * caseNumber + triangleNumber * 3;
      vtkm::IdComponent edgeIndex = TriangleTablePortal.Get(triOffset + vertexNumber);
      vtkm::IdComponent edgeOffset = EdgeTableOffsetPortal.Get(shape);

      return { EdgeTablePortal.Get(edgeOffset + edgeIndex * 2 + 0),
               EdgeTablePortal.Get(edgeOffset + edgeIndex * 2 + 1) };
    }

  private:
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<DeviceAdapter>::PortalConst
      EdgeTablePortal;
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<DeviceAdapter>::PortalConst
      EdgeTableOffsetPortal;
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<DeviceAdapter>::PortalConst
      TriangleTablePortal;
    typename vtkm::cont::ArrayHandle<vtkm::IdComponent>::ExecutionTypes<DeviceAdapter>::PortalConst
      TriangleTableOffsetPortal;
    friend class TriangleGenerationTable;
  };

  template <typename DeviceAdapter>
  ExecObject<DeviceAdapter> PrepareForExecution(DeviceAdapter)
  {
    ExecObject<DeviceAdapter> execObject;
    execObject.EdgeTablePortal = this->EdgeTableArray.PrepareForInput(DeviceAdapter());
    execObject.EdgeTableOffsetPortal = this->EdgeTableOffsetArray.PrepareForInput(DeviceAdapter());
    execObject.TriangleTablePortal = this->TriangleTableArray.PrepareForInput(DeviceAdapter());
    execObject.TriangleTableOffsetPortal =
      this->TriangleTableOffsetArray.PrepareForInput(DeviceAdapter());
    return execObject;
  }

  TriangleGenerationTable()
    : EdgeTableArray(
        vtkm::cont::make_ArrayHandle(EdgeTable, sizeof(EdgeTable) / sizeof(EdgeTable[0])))
    , EdgeTableOffsetArray(
        vtkm::cont::make_ArrayHandle(EdgeTableOffset,
                                     sizeof(EdgeTableOffset) / sizeof(EdgeTableOffset[0])))
    , TriangleTableArray(
        vtkm::cont::make_ArrayHandle(TriangleTable,
                                     sizeof(TriangleTable) / sizeof(TriangleTable[0])))
    , TriangleTableOffsetArray(
        vtkm::cont::make_ArrayHandle(TriangleTableOffset,
                                     sizeof(TriangleTableOffset) / sizeof(TriangleTableOffset[0])))
  {
  }

private:
  vtkm::cont::ArrayHandle<vtkm::IdComponent> EdgeTableArray;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> EdgeTableOffsetArray;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> TriangleTableArray;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> TriangleTableOffsetArray;
};
}
}
}
#endif // vtk_m_ContourTable_h
