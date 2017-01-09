//=======================================================================================
// 
// Second Attempt to Compute Contour Tree in Data-Parallel Mode
//
// Started August 19, 2015
//
// Copyright Hamish Carr, University of Leeds
//
// Type.h - simple include to define types
//
//=======================================================================================
//
// COMMENTS:
//
//	We may end up templating later if this works, but we'll probably do a major rewrite
//  in that case anyway.  Instead, we will define two basic types:
//	
//	dataType:		the underlying type of the data being processed
//	indexType:		the type required for indices - should normally be signed, so we can
//					use negative deltas in a couple of places
//
//	Part of the reason for this is that we expect to migrate to 64-bit indices later
//	and it would be awkward to have to go back and rewrite
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_types_h
#define vtkm_worklet_contourtree_types_h

// constant for consistent processing
#define NO_VERTEX_ASSIGNED -1

#endif
