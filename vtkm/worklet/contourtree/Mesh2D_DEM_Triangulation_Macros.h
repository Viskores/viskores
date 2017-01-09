//=======================================================================================
// 
// Second Attempt to Compute Contour Tree in Data-Parallel Mode
//
// Started August 19, 2015
//
// Copyright Hamish Carr, University of Leeds
//
// Mesh2D_DEM_Macros.h - macros for working with 2D DEM triangulations
//
//=======================================================================================
//
// COMMENTS:
//
//	Essentially, a vector of data values. BUT we will want them sorted to simplify 
//	processing - i.e. it's the robust way of handling simulation of simplicity
//
//	On the other hand, once we have them sorted, we can discard the original data since
//	only the sort order matters
//
//=======================================================================================

// macro definitions
#define N_EDGE_TYPES 3
#define EDGE_TYPE_HORIZONTAL 0
#define EDGE_TYPE_VERTICAL 1
#define EDGE_TYPE_DIAGONAL 2

#define N_INCIDENT_EDGES 6
#define MAX_OUTDEGREE 3

// vertex row - integer divide by columns
#define VERTEX_ROW(V,NCOLS) ((V)/(NCOLS))

// vertex column - integer modulus by columns
#define VERTEX_COL(V,NCOLS) ((V)%(NCOLS))

// vertex ID - row * ncols + col
#define VERTEX_ID(R,C,NCOLS) ((R)*(NCOLS)+(C))

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
