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


#ifndef vtkm_worklet_contourtree_augmented_types_h
#define vtkm_worklet_contourtree_augmented_types_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

// constexpr for bit flags
// clang-format off
constexpr vtkm::Id NO_SUCH_ELEMENT = std::numeric_limits<vtkm::Id>::min();
constexpr vtkm::Id TERMINAL_ELEMENT = std::numeric_limits<vtkm::Id>::max() / 2 + 1; //0x40000000 || 0x4000000000000000
constexpr vtkm::Id IS_SUPERNODE = std::numeric_limits<vtkm::Id>::max() / 4 + 1; //0x20000000 || 0x2000000000000000
constexpr vtkm::Id IS_HYPERNODE = std::numeric_limits<vtkm::Id>::max() / 8 + 1; //0x10000000 || 0x1000000000000000
constexpr vtkm::Id IS_ASCENDING = std::numeric_limits<vtkm::Id>::max() / 16 + 1; //0x08000000 || 0x0800000000000000
constexpr vtkm::Id INDEX_MASK = std::numeric_limits<vtkm::Id>::max() / 16; //0x07FFFFFF || 0x07FFFFFFFFFFFFFF
constexpr vtkm::Id CV_OTHER_FLAG = std::numeric_limits<vtkm::Id>::max() / 8 + 1; //0x10000000 || 0x1000000000000000
// clang-format on

using IdArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;

using EdgePair = vtkm::Pair<vtkm::Id, vtkm::Id>; // here EdgePair.first=low and EdgePair.second=high
using EdgePairArray = vtkm::cont::ArrayHandle<EdgePair>; // Array of edge pairs

// inline functions for retrieving flags or index
VTKM_EXEC_CONT
inline bool noSuchElement(vtkm::Id flaggedIndex)
{ // noSuchElement()
  return ((flaggedIndex & (vtkm::Id)NO_SUCH_ELEMENT) != 0);
} // noSuchElement()

VTKM_EXEC_CONT
inline bool isTerminalElement(vtkm::Id flaggedIndex)
{ // isTerminalElement()
  return ((flaggedIndex & TERMINAL_ELEMENT) != 0);
} // isTerminalElement()

VTKM_EXEC_CONT
inline bool isSupernode(vtkm::Id flaggedIndex)
{ // isSupernode()
  return ((flaggedIndex & IS_SUPERNODE) != 0);
} // isSupernode()

VTKM_EXEC_CONT
inline bool isHypernode(vtkm::Id flaggedIndex)
{ // isHypernode()
  return ((flaggedIndex & IS_HYPERNODE) != 0);
} // isHypernode()

VTKM_EXEC_CONT
inline bool isAscending(vtkm::Id flaggedIndex)
{ // isAscending()
  return ((flaggedIndex & IS_ASCENDING) != 0);
} // isAscending()

VTKM_EXEC_CONT
inline vtkm::Id maskedIndex(vtkm::Id flaggedIndex)
{ // maskedIndex()
  return (flaggedIndex & INDEX_MASK);
} // maskedIndex()

// Used in the context of CombinedVector class used in ContourTreeMesh to merge the mesh of contour trees
VTKM_EXEC_CONT
inline bool isThis(vtkm::Id flaggedIndex)
{ // isThis
  return ((flaggedIndex & CV_OTHER_FLAG) == 0);
} // isThis

template <typename T>
struct MaskedIndexFunctor
{
  VTKM_EXEC_CONT

  MaskedIndexFunctor() {}

  VTKM_EXEC_CONT
  vtkm::Id operator()(T x) const { return maskedIndex(x); }
};

inline std::string flagString(vtkm::Id flaggedIndex)
{ // flagString()
  std::string fString("");
  fString += (noSuchElement(flaggedIndex) ? "n" : ".");
  fString += (isTerminalElement(flaggedIndex) ? "t" : ".");
  fString += (isSupernode(flaggedIndex) ? "s" : ".");
  fString += (isHypernode(flaggedIndex) ? "h" : ".");
  fString += (isAscending(flaggedIndex) ? "a" : ".");
  return fString;
} // flagString()



} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
