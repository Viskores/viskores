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

#ifndef vtkm_worklet_contourtree_mesh3d_dem_triangulation_macros_h
#define vtkm_worklet_contourtree_mesh3d_dem_triangulation_macros_h

// macro definitions
#define N_EDGE_TYPES 3
#define EDGE_TYPE_HORIZONTAL 0
#define EDGE_TYPE_VERTICAL 1
#define EDGE_TYPE_DIAGONAL 2

#define N_INCIDENT_EDGES_3D 14
#define MAX_OUTDEGREE_3D 6

// vertex row
#define VERTEX_ROW_3D(V,NROWS,NCOLS) (((V) % (NROWS * NCOLS)) / NCOLS)

// vertex column
#define VERTEX_COL_3D(V,NROWS,NCOLS) ((V) % (NCOLS))

// vertex slice
#define VERTEX_SLICE_3D(V,NROWS,NCOLS) ((V) / (NROWS * NCOLS))

// vertex ID - row * ncols + col
#define VERTEX_ID_3D(S,R,C,NROWS,NCOLS) (((S) * NROWS + (R)) * (NCOLS)+(C))

// edge row - edge / (ncols * nEdgeTypes)
#define EDGE_ROW(E,NCOLS) ((E)/((NCOLS)*(N_EDGE_TYPES)))
// edge col - (edge / nEdgeTypes) % nCols
#define EDGE_COL(E,NCOLS) (((E)/(N_EDGE_TYPES))%(NCOLS))
// edge which - edge % nEdgeTypes
#define EDGE_WHICH(E) ((E)%(N_EDGE_TYPES))
// edge ID - (row * ncols + col) * nEdgeTypes + which
#define EDGE_ID(R,C,W,NCOLS) ((((R)*(NCOLS)+(C))*(N_EDGE_TYPES))+(W))
// edge from - vertex with same row & col
#define EDGE_FROM(E,NCOLS) VERTEX_ID(EDGE_ROW(E,NCOLS),EDGE_COL(E,NCOLS),NCOLS)
// edge to - edge from +1 col if not vertical, +1 row if not horizontal
#define EDGE_TO(E,NCOLS) VERTEX_ID(EDGE_ROW(E,NCOLS)+((EDGE_WHICH(E)==EDGE_TYPE_HORIZONTAL)?0:1),EDGE_COL(E,NCOLS)+((EDGE_WHICH(E)==EDGE_TYPE_VERTICAL)?0:1),NCOLS)

#endif
