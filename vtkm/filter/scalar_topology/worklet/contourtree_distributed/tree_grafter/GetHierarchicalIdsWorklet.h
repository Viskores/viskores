//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

#ifndef vtk_m_worklet_contourtree_distributed_tree_grafter_get_hierarchical_ids_worklet_h
#define vtk_m_worklet_contourtree_distributed_tree_grafter_get_hierarchical_ids_worklet_h

#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{
namespace tree_grafter
{

// Worklet for retrieving correct Ids from hierarchical tree
class GetHierarchicalIdsWorklet : public vtkm::worklet::WorkletMapField
{
public:
  // TODO: Need to decide for which we can use FieldIn/FieldOut instead of requiring transfer of the WholeArrayIn/Out
  using ControlSignature = void(
    // input (read-only) array
    FieldIn supernodes,

    // reference (read-only) arrays
    FieldIn supernodeGlobalId,
    WholeArrayIn sortOrder,
    WholeArrayIn dataValue,
    FieldIn necessary,
    FieldIn above,
    FieldIn below,
    WholeArrayIn superparents,
    WholeArrayIn hyperparents,
    WholeArrayIn regular2Supernode,
    WholeArrayIn super2Hypernode,
    // Execution object to use the FindRegularByGlobal and FindSuperArcForUnknownNode for the hierarchical tree.
    ExecObject findRegularByGlobal,
    ExecObject findSuperArcForUnknownNode,

    // output (write-only) arrays
    // NOTE: not all fileds are always updated in the operator.
    //       We, therfore, need to use FieldInOut to make sure the original value (typical NO_SUCH_ELEMTENT)
    //       is not being overwritten (when just using FieldOut VTKm sets the value to 0)
    // TODO: We could potentially avoid the need for FieldInOut by always setting hierarchicalSuperId and hierarchicalHyperId to NO_SUCH_ELEMENT at the beginning
    FieldInOut hierarchicalRegularId,
    FieldInOut hierarchicalSuperId,
    FieldInOut hierarchicalHyperId,
    FieldInOut hierarchicalSuperparent,
    FieldInOut hierarchicalHyperparent);

  using ExecutionSignature =
    void(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18);
  using InputDomain = _1;

  // Default Constructor
  VTKM_EXEC_CONT
  GetHierarchicalIdsWorklet() {}

  // TODO: Discuss with Oliver when it's a ID, when it's a portal and when it's a reference
  template <typename InFieldPortalType,
            typename SortOrderPortalType,
            typename InFieldDataPortalType,
            typename FindRegularExecType,
            typename FindSuperExecType>
  VTKM_EXEC void operator()(
    // const vtkm::Id&              supernode, // not used
    const vtkm::Id& oldSortId, // i.e, supernodes[supernode]
    const vtkm::Id&
      supernodeGlobalIdVal, // Depending on the mesh we may get a different fany array handle
    const SortOrderPortalType&
      sortOrder, // Depending on the mesh we may get a different fany array handle
    const InFieldDataPortalType& dataValues,
    const vtkm::Id& necessary,
    const vtkm::Id& upGlobalId, // = above[supernode]
    const vtkm::Id& dnGlobalId, // = below[supernode]
    const InFieldPortalType& superparents,
    const InFieldPortalType& hyperparents,
    const InFieldPortalType& regular2Supernode,
    const InFieldPortalType& super2Hypernode,
    const FindRegularExecType& findRegularByGlobal,
    const FindSuperExecType& findSuperArcForUnknownNode,
    vtkm::Id& hierarchicalRegularId,
    vtkm::Id& hierarchicalSuperId,
    vtkm::Id& hierarchicalHyperId,
    vtkm::Id& hierarchicalSuperparent,
    vtkm::Id& hierarchicalHyperparent) const
  { // operator ()

    // TODO: Need to check that this code is correct
    // TODO: Discuss with Oliver how to use the FindRegularByGlobal function
    vtkm::Id regularId = findRegularByGlobal.FindRegularByGlobal(supernodeGlobalIdVal);

    // save the regular Id
    hierarchicalRegularId = regularId; // set return value

    // Free supernode: if it was necessary, then it's potentially an attachment point
    if (vtkm::worklet::contourtree_augmented::NoSuchElement(regularId))
    //  Super = NSE, Regular = NSE:
    { // no such element regular or super
      // not marked as necessary: cannot be an attachment point
      if (!necessary)
      {
        return;
      }

      // it was marked as necessary, but got regularised & therefore not a regular point in the parent
      // this means we are guaranteed that we have a valid up/down neighbour pair from our BRACT construction
      // and that the up/down neighbour pair are guaranteed to be at least regular in the hierarchical tree
      // vtkm::Id oldSortId = supernodes.Get(supernode);
      vtkm::Id oldRegularId = sortOrder.Get(oldSortId);
      auto dataValue = dataValues.Get(oldRegularId);

      vtkm::Id upHierarchicalId = findRegularByGlobal.FindRegularByGlobal(upGlobalId);

      vtkm::Id dnHierarchicalId = findRegularByGlobal.FindRegularByGlobal(dnGlobalId);

      vtkm::Id superparentInHierarchy = findSuperArcForUnknownNode.FindSuperArcForUnknownNode(
        supernodeGlobalIdVal, dataValue, upHierarchicalId, dnHierarchicalId);
      hierarchicalSuperparent = superparentInHierarchy;                   // set return value
      hierarchicalHyperparent = hyperparents.Get(superparentInHierarchy); // set return value
    } // no such element regular or super
    else
    { // regular Id exists
      // we want to know if the regular node has a superId.  This is straightforward, since we have a lookup array
      vtkm::Id supernodeId = regular2Supernode.Get(regularId);
      vtkm::Id superparent = superparents.Get(regularId);

      // if there is no superId yet, we will need to set one up
      if (vtkm::worklet::contourtree_augmented::NoSuchElement(supernodeId))
      { // no supernode Id: regular but not super
        // We have a supernode in the lower level tree (which excludes boundary regular vertices from consideration)
        // This occurred AFTER regularisation, so either:
        // a) it was regularised, but is a boundary vertex and was therefore kept, or
        // b) it was not regularised, but is not critical once other blocks have been considered
        // In either case, it is now needed and must be inserted as an attachment point
        // Note that regularised interior attachment points are dealt with later
        // We assume as an invariant that all regular points in the hierarchical tree already have superparent correctly set
        // This means we know which superarc it belongs to, and therefore which hyperarc, so we take that as the insertion parent
        vtkm::Id hierSuperparent =
          vtkm::worklet::contourtree_augmented::MaskedIndex(superparents.Get(regularId));
        // this is logically redundant since we won't use it, but it'll make debug easier
        hierarchicalSuperparent = hierSuperparent;                   // set return value
        hierarchicalHyperparent = hyperparents.Get(hierSuperparent); // set return value
      } // no supernode Id: regular but not super
      else
      { // supernode Id exists
        // save the supernode Id
        hierarchicalSuperId = supernodeId; // Set return value

        // we can also set the superparent & hyperparent at this point
        hierarchicalSuperparent = superparent;                   // set return value
        hierarchicalHyperparent = hyperparents.Get(supernodeId); // set return value

        // now retrieve the hypernode Id (if any) & store it (even if it is already NO_SUCH_ELEMENT)
        hierarchicalHyperId = super2Hypernode.Get(supernodeId); // set return value
      }                                                         // supernode Id exists
    }                                                           // regular Id exists

    // In serial this worklet implements the following operation
    /*
      // down-convert to a global ID
      // search in the hierarchy to find the regular ID
      indexType regularID = hierTree.FindRegularByGlobal(supernodeGlobalID[supernode]);
      hierarchicalRegularID[supernode] = regularID;

      // Free supernode: if it was necessary, then it's potentially an attachment point
      if (noSuchElement(regularID))
        //  Super = NSE, Regular = NSE:
        { // no such element regular or super
        // not marked as necessary: cannot be an attachment point
        if (!residue->isNecessary[supernode])
          continue;

        // it was marked as necessary, but got regularised & therefore not a regular point in the parent
        // this means we are guaranteed that we have a valid up/down neighbour pair from our BRACT construction
        // and that the up/down neighbour pair are guaranteed to be at least regular in the hierarchical tree
        indexType oldSortID = contourTree->supernodes[supernode];
        indexType oldRegularID = mesh->SortOrder(oldSortID);
        dataType dataValue = mesh->DataValue(oldRegularID);

        indexType upGlobalID = residue->above[supernode];
        indexType upHierarchicalID = hierTree.FindRegularByGlobal(upGlobalID);

        indexType dnGlobalID = residue->below[supernode];
        indexType dnHierarchicalID = hierTree.FindRegularByGlobal(dnGlobalID);

        indexType superparentInHierarchy = hierTree.FindSuperArcForUnknownNode(supernodeGlobalID[supernode], dataValue, upHierarchicalID, dnHierarchicalID);
        hierarchicalSuperparent[supernode] = superparentInHierarchy;
        hierarchicalHyperparent[supernode] = hierTree.hyperparents[superparentInHierarchy];
        } // no such element regular or super
      else
        { // regular ID exists
        // we want to know if the regular node has a superID.  This is straightforward, since we have a lookup array
        indexType supernodeID = hierTree.regular2supernode[regularID];
        indexType superparent = hierTree.superparents[regularID];

        // if there is no superID yet, we will need to set one up
        if (noSuchElement(supernodeID))
          { // no supernode ID: regular but not super
          // We have a supernode in the lower level tree (which excludes boundary regular vertices from consideration)
          // This occurred AFTER regularisation, so either:
          // a) it was regularised, but is a boundary vertex and was therefore kept, or
          // b) it was not regularised, but is not critical once other blocks have been considered
          // In either case, it is now needed and must be inserted as an attachment point
          // Note that regularised interior attachment points are dealt with later
          // We assume as an invariant that all regular points in the hierarchical tree already have superparent correctly set
          // This means we know which superarc it belongs to, and therefore which hyperarc, so we take that as the insertion parent
          indexType hierSuperparent = maskedIndex(hierTree.superparents[regularID]);
          // this is logically redundant since we won't use it, but it'll make debug easier
          hierarchicalSuperparent[supernode] = hierSuperparent;
          hierarchicalHyperparent[supernode] = hierTree.hyperparents[hierSuperparent];
          } // no supernode ID: regular but not super
        else
          { // supernode ID exists
          // save the supernode ID
          hierarchicalSuperID[supernode] = supernodeID;

          // we can also set the superparent & hyperparent at this point
          hierarchicalSuperparent[supernode] = superparent;
          hierarchicalHyperparent[supernode] = hierTree.hyperparents[supernodeID];

          // now retrieve the hypernode ID (if any) & store it (even if it is already NO_SUCH_ELEMENT)
          hierarchicalHyperID[supernode] = hierTree.super2hypernode[supernodeID];
          } // supernode ID exists
        } // regular ID exists
      } // per tree supernode
  */
  } // operator ()

}; // BoundaryVerticiesPerSuperArcStepOneWorklet

} // namespace tree_grafter
} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
