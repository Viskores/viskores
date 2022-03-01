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
// COMMENTS:
//
// Input is assumed to be a sequence of lines of the form:
//	I	Global ID of branch root
//	II	Value of supernode
//	III	Global ID of supernode
//
//	All lines are assumed to have been sorted already.  Because of how the
//      Unix sort utility operates (textual sort), the most we can assume is that all
//      supernodes corresponding to a given branch root are sorted together.
//
//	We therefore do simple stream processing, identifying new branches by
//      the changes in root ID.
//
//=======================================================================================

#include <iostream>
#include <vtkm/worklet/contourtree_augmented/Types.h>

int main()
{ // main()
  // variables tracking the best & worst so far for this extent
  vtkm::Id currentBranch = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
  // this is slightly tricky, since we don't know the range of the data type
  // yet, but we can initialize to 0 for both floats and integers, then test on
  // current branch
  vtkm::Float64 highValue = 0;
  vtkm::Float64 lowValue = 0;
  vtkm::Id highEnd = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
  vtkm::Id lowEnd = vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;

  // values to read in
  vtkm::Id nextBranch;
  vtkm::Float64 nextValue;
  vtkm::Id nextSupernode;

  while (true)
  { // until stream goes bad
    // read the next triple
    std::cin >> nextBranch >> nextValue >> nextSupernode;

    // test in the middle before processing
    if (std::cin.eof())
      break;

    // test to see if the branch is different from the current one
    if (nextBranch != currentBranch)
    { // new branch
      // special test for initial one
      if (!vtkm::worklet::contourtree_augmented::NoSuchElement(currentBranch))
        printf("%12llu  %12llu\n", highEnd, lowEnd);

      // set the high & low value ends to this one
      highValue = nextValue;
      lowValue = nextValue;
      highEnd = nextSupernode;
      lowEnd = nextSupernode;

      // and reset the branch ID
      currentBranch = nextBranch;
    } // new branch
    else
    { // existing branch
      // test value with simulation of simplicity
      if ((nextValue > highValue) || ((nextValue == highValue) && (nextSupernode > highEnd)))
      { // new high end
        highEnd = nextSupernode;
        highValue = nextValue;
      } // new high end
      // test value with simulation of simplicity
      else if ((nextValue < lowValue) || ((nextValue == lowValue) && (nextSupernode < lowEnd)))
      { // new low end
        lowEnd = nextSupernode;
        lowValue = nextValue;
      } // new low end
    }   // existing branch
  }     // until stream goes bad

  printf("%12llu  %12llu\n", highEnd, lowEnd);
} // main()
