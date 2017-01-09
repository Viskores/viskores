//=======================================================================================
// 
// Second Attempt to Compute Contour Tree in Data-Parallel Mode
//
// Started August 19, 2015
//
// Copyright Hamish Carr, University of Leeds
//
// SaddleAscentFunctor.h - functor that counts & identifies active saddle edges
//
//=======================================================================================
//
// COMMENTS:
//
// Any vector needed by the functor for lookup purposes will be passed as a parameter to
// the constructor and saved, with the actual function call being the operator ()
//
// Vectors marked I/O are intrinsically risky unless there is an algorithmic guarantee
// that the read/writes are completely independent - which for our case actually occurs
// The I/O vectors should therefore be justified in comments both here & in caller
//
//=======================================================================================

#ifndef vtkm_worklet_contourtree_saddle_ascent_functor_h
#define vtkm_worklet_contourtree_saddle_ascent_functor_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/worklet/contourtree/Types.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

// Worklet for setting initial chain maximum value
class SaddleAscentFunctor : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<IdType> vertexID,             // (input) index into active vertices
                                WholeArrayIn<IdType> firstEdge,       // (input) first edge for each active vertex
                                WholeArrayIn<IdType> outdegree,       // (input) updegree of vertex
                                WholeArrayIn<IdType> activeEdges,     // (input) active edges
                                WholeArrayIn<IdType> chainExtemum,    // (input) chain extemum for vertices
                                WholeArrayInOut<IdType> edgeFar,      // (input) high ends of edges
                                FieldOut<IdType> newOutdegree);       // (output) new updegree of vertex
  typedef _7 ExecutionSignature(_1, _2, _3, _4, _5, _6);
  typedef _1 InputDomain;

  // Constructor
  VTKM_EXEC_CONT
  SaddleAscentFunctor() {}

  template <typename InFieldPortalType, typename InOutFieldPortalType>
  VTKM_EXEC
  vtkm::Id operator()(const vtkm::Id& vertexID,
                      const InFieldPortalType& firstEdge,
                      const InFieldPortalType& outdegree,
                      const InFieldPortalType& activeEdges,
                      const InFieldPortalType& chainExtremum,
                      const InOutFieldPortalType& edgeFar) const
  {
    vtkm::Id newOutdegree;

    // first ascent found
    vtkm::Id firstMax = NO_VERTEX_ASSIGNED;
    bool isGenuineSaddle = false;

    // loop through edges
    for (vtkm::Id edge = 0; edge < outdegree.Get(vertexID); edge++)
    {
      // retrieve the edge ID and the high end of the edge
      vtkm::Id edgeID = activeEdges.Get(firstEdge.Get(vertexID) + edge);
      vtkm::Id nbrHigh = chainExtremum.Get(edgeFar.Get(edgeID));
      edgeFar.Set(edgeID, nbrHigh);

      // test for first one found                       
      if (firstMax == NO_VERTEX_ASSIGNED)
        firstMax = nbrHigh;
      else // otherwise, check for whether we have an actual join saddle
        if (firstMax != nbrHigh)
          { // first non-matching
            isGenuineSaddle = true;
          } // first non-matching
    } // per edge

    // if it's not a genuine saddle, ignore the edges by setting updegree to 0
    if (!isGenuineSaddle)
      newOutdegree = 0;
    else
      newOutdegree = outdegree.Get(vertexID);
    return newOutdegree;
  }
}; // SaddleAscentFunctor

}
}
}

#endif
