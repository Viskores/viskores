//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
//  Copyright (c) 2016, Los Alamos National Security, LLC
//  All rights reserved.
//
//  Copyright 2016. Los Alamos National Security, LLC.
//  This software was produced under U.S. Government contract DE-AC52-06NA25396
//  for Los Alamos National Laboratory (LANL), which is operated by
//  Los Alamos National Security, LLC for the U.S. Department of Energy.
//  The U.S. Government has rights to use, reproduce, and distribute this
//  software.  NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC
//  MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE
//  USE OF THIS SOFTWARE.  If software is modified to produce derivative works,
//  such modified software should be clearly marked, so as not to confuse it
//  with the version available from LANL.
//
//  Additionally, redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the following conditions
//  are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//  3. Neither the name of Los Alamos National Security, LLC, Los Alamos
//     National Laboratory, LANL, the U.S. Government, nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND
//  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
//  BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS
//  NATIONAL SECURITY, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
//  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
//  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//============================================================================

//  This code is based on the algorithm presented in the paper:
//  “Parallel Peak Pruning for Scalable SMP Contour Tree Computation.”
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.

#ifndef viskores_worklet_contourtree_skip_vertex_h
#define viskores_worklet_contourtree_skip_vertex_h

#include <viskores/filter/scalar_topology/worklet/contourtree/Types.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace worklet
{
namespace contourtree
{

// Worklet for doing regular to candidate
class SkipVertex : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn superID,            // (input) index into supernodes
                                WholeArrayIn superarcs,     // (input)
                                WholeArrayInOut joinArcs,   // (i/o)
                                WholeArrayInOut splitArcs); // (i/o)
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  // Constructor
  VISKORES_EXEC_CONT
  SkipVertex() {}

  template <typename InFieldPortalType, typename OutFieldPortalType>
  VISKORES_EXEC void operator()(const viskores::Id& superID,
                            const InFieldPortalType& superarcs,
                            const OutFieldPortalType& joinArcs,
                            const OutFieldPortalType& splitArcs) const
  {
    //  retrieve it's join neighbour j
    viskores::Id joinNeighbour = joinArcs.Get(superID);

    // if v has a join neighbour (i.e. j == -1) and j has a contour arc
    if ((joinNeighbour != NO_VERTEX_ASSIGNED) &&
        (superarcs.Get(joinNeighbour) != NO_VERTEX_ASSIGNED))
      // reset the vertex' join neighbour
      joinArcs.Set(superID, joinArcs.Get(joinNeighbour));

    // retrieve it's split neighbour s
    viskores::Id splitNeighbour = splitArcs.Get(superID);

    // if v has a split neighbour (i.e. s == -1) and s has a contour arc
    if ((splitNeighbour != NO_VERTEX_ASSIGNED) &&
        (superarcs.Get(splitNeighbour) != NO_VERTEX_ASSIGNED))
      // reset the vertex' split neighbour
      splitArcs.Set(superID, splitArcs.Get(splitNeighbour));
  }
}; // SkipVertex
}
}
}

#endif
