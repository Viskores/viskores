//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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

#ifndef viskores_worklet_contourtree_augmented_process_contourtree_inc_superarc_to_edge_worklet_h
#define viskores_worklet_contourtree_augmented_process_contourtree_inc_superarc_to_edge_worklet_h

#include <viskores/Pair.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{

// Emits the saddle-peak edge for a single superarc of a ContourTreeAugmented output DataSet,
// where the Supernodes field already holds mesh vertex IDs (no sort order needed). For each
// supernode it produces the oriented (low, high) endpoint pair and a keep flag; supernodes with
// no outgoing superarc (the root) and the duplicated top/bottom edge are flagged with keep = 0 so
// they can be removed with a subsequent stream compaction.
class SuperarcToEdge : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn supernodeIndex,  // (input) supernode id (ArrayHandleIndex)
                                WholeArrayIn supernodes, // (input) mesh vertex id per supernode
                                WholeArrayIn superarcs,  // (input) masked target supernode per arc
                                FieldOut edge,           // (output) oriented (low, high) mesh ids
                                FieldOut keepFlag);      // (output) 1 if edge is real, else 0
  using ExecutionSignature = void(_1, _2, _3, _4, _5);
  using InputDomain = _1;

  VISKORES_EXEC_CONT
  SuperarcToEdge() {}

  template <typename SupernodesPortalType, typename SuperarcsPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& supernode,
                                const SupernodesPortalType& supernodesPortal,
                                const SuperarcsPortalType& superarcsPortal,
                                viskores::Pair<viskores::Id, viskores::Id>& edge,
                                viskores::IdComponent& keepFlag) const
  {
    viskores::Id regularID = supernodesPortal.Get(supernode);
    viskores::Id superTo = superarcsPortal.Get(supernode);

    // last pruned vertex has no superarc -> not an edge
    if (NoSuchElement(superTo))
    {
      edge = viskores::Pair<viskores::Id, viskores::Id>(0, 0);
      keepFlag = 0;
      return;
    }

    superTo = MaskedIndex(superTo);
    viskores::Id regularTo = supernodesPortal.Get(superTo);

    if (regularID < regularTo)
    { // from is lower
      // extra test to catch the duplicate edge at the far end
      if (superarcsPortal.Get(superTo) != supernode)
      {
        edge = viskores::Pair<viskores::Id, viskores::Id>(regularID, regularTo);
        keepFlag = 1;
      }
      else
      {
        edge = viskores::Pair<viskores::Id, viskores::Id>(0, 0);
        keepFlag = 0;
      }
    } // from is lower
    else
    {
      edge = viskores::Pair<viskores::Id, viskores::Id>(regularTo, regularID);
      keepFlag = 1;
    }
  }
};

} // namespace process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace viskores

#endif
