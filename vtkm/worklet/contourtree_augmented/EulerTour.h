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

#ifndef vtkm_worklet_contourtree_augmented_euler_tour_h
#define vtkm_worklet_contourtree_augmented_euler_tour_h

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/ComputeEulerTourFirstNext.h>
#include <vtkm/worklet/contourtree_augmented/processcontourtree/ComputeEulerTourList.h>


namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{
class EulerTour
{

public:
  vtkm::cont::Invoker Invoke;

  // The sequence of the edges in the tours is given by succ array
  cont::ArrayHandle<Id> succ;

  // The tour consists of the directed edges of the contour tree
  cont::ArrayHandle<Vec<Id, 2>> edges;

  // The first occurance of an vertex
  cont::ArrayHandle<Id> first;

  // Compute the Euler Tour with root 0
  void computeEulerTour(const IdArrayType::PortalConstControl superarcs)
  {
    //
    // Make a list of all directed edges in the tree
    //
    edges.Allocate(2 * (superarcs.GetNumberOfValues() - 1));

    auto edgesPortal = edges.GetPortalControl();

    int counter = 0;
    for (int i = 0; i < superarcs.GetNumberOfValues(); i++)
    {
      Id j = maskedIndex(superarcs.Get(i));

      if (j != 0)
      {
        edgesPortal.Set(counter++, { i, j });
        edgesPortal.Set(counter++, { j, i });
      }
    }

    vtkm::cont::Algorithm::Sort(edges, vtkm::SortLess());

    //
    // Initialize first and next arrays. They are used to compute the ciculer linked list that is the euler tour
    //

    //cont::ArrayHandle<Id> first;
    cont::ArrayHandle<Id> next;

    vtkm::cont::ArrayCopy(
      vtkm::cont::make_ArrayHandle(std::vector<Id>(
        static_cast<unsigned long>(superarcs.GetNumberOfValues()), NO_SUCH_ELEMENT)),
      first);
    vtkm::cont::ArrayCopy(
      vtkm::cont::make_ArrayHandle(
        std::vector<Id>(static_cast<unsigned long>(edges.GetNumberOfValues()), NO_SUCH_ELEMENT)),
      next);

    //
    // Compute First and Next arrays that are needed to compute the euler tour linked list
    //
    vtkm::worklet::contourtree_augmented::process_contourtree_inc::ComputeEulerTourFirstNext
      eulerWorklet;
    this->Invoke(eulerWorklet, edges, first, next);

    //
    // Compute the euler tour as a circular linked list from the first and next arrays
    //
    succ.Allocate(edges.GetNumberOfValues());

    vtkm::worklet::contourtree_augmented::process_contourtree_inc::ComputeEulerTourList
      eulerTourListWorklet;
    this->Invoke(eulerTourListWorklet, next, first, edges, succ);
  }

  // Reroot the euler tour at a different root (O(n) for finding the first occurence of the new root and O(1) for rerouting and O(n) for returning it as an array)
  void getTourAtRoot(const Id root, const cont::ArrayHandle<Vec<Id, 2>>::PortalControl tourEdges)
  {
    auto edgesPortal = edges.GetPortalControl();

    //
    // Reroot at the global min/max
    //
    Id i = 0;
    Id start = NO_SUCH_ELEMENT;
    do
    {
      if (edgesPortal.Get(i)[0] == root)
      {
        start = i;
        break;
      }

      i = succ.GetPortalControl().Get(i);
    } while (i != 0);

    //
    // Convert linked list to array handle array
    //
    {
      int counter = 0;
      i = start;
      do
      {
        tourEdges.Set(counter++, edgesPortal.Get(i));
        i = succ.GetPortalControl().Get(i);

      } while (i != start);
    }
  }
}; // class EulerTour
} // namespace contourtree_augmented
} // worklet
} // vtkm
#endif
