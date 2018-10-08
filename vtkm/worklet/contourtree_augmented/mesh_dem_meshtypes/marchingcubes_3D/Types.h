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
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#ifndef vtkm_worklet_contourtree_augmented_mesh_dem_triangulation_3D_marchingcubes_types_h
#define vtkm_worklet_contourtree_augmented_mesh_dem_triangulation_3D_marchingcubes_types_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace mesh_dem_3d_marchingcubes_inc
{

// Constants and case tables
static const vtkm::Int8 frontBit = 1 << 4;
static const vtkm::Int8 backBit = 1 << 5;
static const vtkm::Int8 topBit = 1 << 2;
static const vtkm::Int8 bottomBit = 1 << 3;
static const vtkm::Int8 leftBit = 1 << 0;
static const vtkm::Int8 rightBit = 1 << 1;

static const vtkm::IdComponent N_EDGE_NEIGHBOURS = 6;
static const vtkm::IdComponent N_FACE_NEIGHBOURS = 18;
static const vtkm::IdComponent N_ALL_NEIGHBOURS = 26;

// edgeBoundaryDetectionMasks
static const vtkm::Int8 edgeBoundaryDetectionMasks[N_ALL_NEIGHBOURS] = {
  frontBit,
  topBit,
  leftBit,
  rightBit,
  bottomBit,
  backBit,
  frontBit | topBit,
  frontBit | leftBit,
  frontBit | rightBit,
  frontBit | bottomBit,
  topBit | leftBit,
  topBit | rightBit,
  bottomBit | leftBit,
  bottomBit | rightBit,
  backBit | topBit,
  backBit | leftBit,
  backBit | rightBit,
  backBit | bottomBit,
  frontBit | topBit | leftBit,
  frontBit | topBit | rightBit,
  frontBit | bottomBit | leftBit,
  frontBit | bottomBit | rightBit,
  backBit | topBit | leftBit,
  backBit | topBit | rightBit,
  backBit | bottomBit | leftBit,
  backBit | bottomBit | rightBit
};
// VTK-M type for the edgeBoundaryDetectionMasks
typedef typename vtkm::cont::ArrayHandle<vtkm::Int8> edgeBoundaryDetectionMasksType;


// Number of permutation vectors in cubeVertexPermutations
static const vtkm::UInt8 cubeVertexPermutations_NumPermutations = 8;
// Length of a single permutation vector in the cubeVertexPermutations array
static const vtkm::UInt8 cubeVertexPermutations_PermVecLength = 7;
// VTK-M type for the cubeVertexPermutations
typedef typename vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<vtkm::IdComponent>,
                                                 cubeVertexPermutations_PermVecLength>
  cubeVertexPermutationsType;
/* cubeVertexPermutations will be used as a 2D array of [8, 7]
   * The array is flattened here to ease conversion in vtk-m
   */
static const vtkm::IdComponent cubeVertexPermutations[cubeVertexPermutations_NumPermutations *
                                                      cubeVertexPermutations_PermVecLength] = {
  3, 4, 5, 13, 16, 17, 25, 3, 4, 0, 13, 8, 9, 21, 3, 1, 5, 11, 16, 14, 23, 2, 4, 5, 12, 15, 17, 24,
  3, 1, 0, 11, 8,  6,  19, 2, 4, 0, 12, 7, 9, 20, 2, 1, 5, 10, 15, 14, 22, 2, 1, 0, 10, 7,  6,  18
};


// number of vertex connection pairs contained in linkVertexConnectionsSix
static const vtkm::UInt8 linkVertexConnectionsSix_NumPairs = 3;
// number of components defining a vertex connection
static const vtkm::UInt8 vertexConnections_VecLength = 2;
// VTKM-M type for the linkVertexConnectionsEighteen and linkVertexConnectionsSix
typedef typename vtkm::cont::ArrayHandleGroupVec<vtkm::cont::ArrayHandle<vtkm::IdComponent>,
                                                 vertexConnections_VecLength>
  linkVertexConnectionsType;
/* linkVertexConnectionsSix[ will be used as a 2D array of [3, 3]
   * The array is flattened here to ease conversion in vtk-m
   */
static const vtkm::IdComponent linkVertexConnectionsSix[linkVertexConnectionsSix_NumPairs *
                                                        vertexConnections_VecLength] = { 0, 1, 0,
                                                                                         2, 1, 2 };

// number of vertex connection pairs contained in linkVertexConnectionsSix
static const vtkm::UInt8 linkVertexConnectionsEighteen_NumPairs = 15;
/* linkVertexConnectionsEighteen[ will be used as a 2D array of [3, 3]
   * The array is flattened here to ease conversion in vtk-m
   */
static const vtkm::IdComponent
  linkVertexConnectionsEighteen[linkVertexConnectionsEighteen_NumPairs *
                                vertexConnections_VecLength] = { 0, 1, 0, 2, 0, 3, 0, 4, 0, 5,
                                                                 1, 2, 1, 3, 1, 4, 1, 5, 2, 3,
                                                                 2, 4, 2, 5, 3, 4, 3, 5, 4, 5 };

// VTKM-M type for the inCubeConnectionsEighteen and inCubeConnectionsSix
typedef typename vtkm::cont::ArrayHandle<vtkm::UInt32> inCubeConnectionsType;
static const vtkm::UInt8 inCubeConnectionsSix_NumElements = 128;
static const vtkm::UInt32 inCubeConnectionsSix[inCubeConnectionsSix_NumElements] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 1, 0, 2, 0, 7,
  0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 1, 0, 0, 4, 7, 0, 0, 0, 0, 0, 2, 4, 7, 0, 0, 0, 1, 0, 2, 4, 7,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 2, 0, 2, 0, 0, 0, 1, 0, 2, 4, 7,
  0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 1, 0, 2, 4, 7, 0, 0, 0, 1, 0, 2, 4, 7, 0, 0, 0, 1, 0, 2, 4, 7
};

static const vtkm::UInt8 inCubeConnectionsEighteen_NumElements = 128;
static const vtkm::UInt32 inCubeConnectionsEighteen[inCubeConnectionsEighteen_NumElements] = {
  0,     0,     0,     1,     0,     2,     32,    35,    0,     4,     64,    69,    0,
  518,   608,   615,   0,     8,     0,     137,   1024,  1034,  1184,  1195,  4096,  4108,
  4288,  4301,  5632,  5646,  5856,  5871,  0,     0,     256,   273,   2048,  2066,  2336,
  2355,  8192,  8212,  8512,  8533,  10752, 10774, 11104, 11127, 16384, 16408, 16768, 16793,
  19456, 19482, 19872, 19899, 28672, 28700, 29120, 29149, 32256, 32286, 32736, 32767, 0,
  0,     0,     1,     0,     2,     32,    35,    0,     4,     64,    69,    512,   518,
  608,   615,   0,     8,     128,   137,   1024,  1034,  1184,  1195,  4096,  4108,  4288,
  4301,  5632,  5646,  5856,  5871,  0,     16,    256,   273,   2048,  2066,  2336,  2355,
  8192,  8212,  8512,  8533,  10752, 10774, 11104, 11127, 16384, 16408, 16768, 16793, 19456,
  19482, 19872, 19899, 28672, 28700, 29120, 29149, 32256, 32286, 32736, 32767
};




} // mesh_dem_types_2d_freudenthal
} // contourtree_augmented
} // worklet
} // vtkm

#endif
