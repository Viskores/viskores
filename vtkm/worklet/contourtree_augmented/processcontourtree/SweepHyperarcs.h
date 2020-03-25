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

#ifndef vtk_m_worklet_contourtree_augmented_process_contourtree_sweep_hyperarcs_h
#define vtk_m_worklet_contourtree_augmented_process_contourtree_sweep_hyperarcs_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

/*
 *
* This code is written by Petar Hristov in 03.2020
*
* It does pointer doubling on an array.
*
*
*/
namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
namespace process_contourtree_inc
{
template <class BinaryFunctor>
class SweepHyperarcs : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(WholeArrayIn hypernodesPortal,
                                WholeArrayIn howManyUsedPortal,
                                WholeArrayInOut minMaxIndexPortal);

  typedef void ExecutionSignature(InputIndex, _1, _2, _3);
  using InputDomain = _1;

  BinaryFunctor operation;

  // Default Constructor
  VTKM_EXEC_CONT SweepHyperarcs(BinaryFunctor _operation)
    : operation(_operation)
  {
  }

  template <typename HypernodesPortalType,
            typename HowManyUsedPortalType,
            typename MinMaxIndexPortalType>
  VTKM_EXEC void operator()(const vtkm::Id hypernode,
                            const HypernodesPortalType& hypernodesPortal,
                            const HowManyUsedPortalType& howManyUsedPortal,
                            const MinMaxIndexPortalType& minMaxIndexPortal) const
  {
    // If it's the last hyperacs (there's nothing to do it's just the root)
    //if (hypernode >= hypernodesPortal.GetNumberOfValues() - 1) { return; }

    vtkm::Id firstSupernode = MaskedIndex(hypernodesPortal.Get(hypernode));
    vtkm::Id lastSupernode =
      MaskedIndex(hypernodesPortal.Get(hypernode + 1)) - howManyUsedPortal.Get(hypernode);

    //auto subarray = vtkm::cont::make_ArrayHandleView(minMaxIndex, firstSupernode, lastSupernode - firstSupernode);
    //vtkm::cont::Algorithm::ScanInclusive(subarray, subarray, operation);

    //
    // Prefix scan along the hyperarc chain. Parallel prefix scan.
    //
    for (Id j = firstSupernode + 1; j < lastSupernode; j++)
    {
      Id vertex = j - 1;
      Id parent = j;

      Id vertexValue = minMaxIndexPortal.Get(vertex);
      Id parentValue = minMaxIndexPortal.Get(parent);

      minMaxIndexPortal.Set(parent, operation(vertexValue, parentValue));
    }
  }
}; // ComputeMinMaxValues
} // process_contourtree_inc
} // namespace contourtree_augmented
} // namespace worklet
} // namespace vtkm


#endif
