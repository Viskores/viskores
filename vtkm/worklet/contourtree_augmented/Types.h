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


#ifndef vtk_m_worklet_contourtree_augmented_types_h
#define vtk_m_worklet_contourtree_augmented_types_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetStructured.h>

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
constexpr vtkm::Id ELEMENT_EXISTS = std::numeric_limits<vtkm::Id>::max() / 4 + 1; //0x20000000 || 0x2000000000000000 , same as IS_SUPERNODE
// clang-format on

using IdArrayType = vtkm::cont::ArrayHandle<vtkm::Id>;

using EdgePair = vtkm::Pair<vtkm::Id, vtkm::Id>; // here EdgePair.first=low and EdgePair.second=high
using EdgePairArray = vtkm::cont::ArrayHandle<EdgePair>; // Array of edge pairs

// inline functions for retrieving flags or index
VTKM_EXEC_CONT
inline bool NoSuchElement(vtkm::Id flaggedIndex)
{ // NoSuchElement()
  return ((flaggedIndex & (vtkm::Id)NO_SUCH_ELEMENT) != 0);
} // NoSuchElement()

VTKM_EXEC_CONT
inline bool IsTerminalElement(vtkm::Id flaggedIndex)
{ // IsTerminalElement()
  return ((flaggedIndex & TERMINAL_ELEMENT) != 0);
} // IsTerminalElement()

VTKM_EXEC_CONT
inline bool IsSupernode(vtkm::Id flaggedIndex)
{ // IsSupernode()
  return ((flaggedIndex & IS_SUPERNODE) != 0);
} // IsSupernode()

VTKM_EXEC_CONT
inline bool IsHypernode(vtkm::Id flaggedIndex)
{ // IsHypernode()
  return ((flaggedIndex & IS_HYPERNODE) != 0);
} // IsHypernode()

VTKM_EXEC_CONT
inline bool IsAscending(vtkm::Id flaggedIndex)
{ // IsAscending()
  return ((flaggedIndex & IS_ASCENDING) != 0);
} // IsAscending()

VTKM_EXEC_CONT
inline vtkm::Id MaskedIndex(vtkm::Id flaggedIndex)
{ // MaskedIndex()
  return (flaggedIndex & INDEX_MASK);
} // MaskedIndex()

// Used in the context of CombinedVector class used in ContourTreeMesh to merge the mesh of contour trees
VTKM_EXEC_CONT
inline bool IsThis(vtkm::Id flaggedIndex)
{ // IsThis
  return ((flaggedIndex & CV_OTHER_FLAG) == 0);
} // IsThis

template <typename T>
struct MaskedIndexFunctor
{
  VTKM_EXEC_CONT

  MaskedIndexFunctor() {}

  VTKM_EXEC_CONT
  vtkm::Id operator()(T x) const { return MaskedIndex(x); }
};

inline std::string FlagString(vtkm::Id flaggedIndex)
{ // FlagString()
  std::string fString("");
  fString += (NoSuchElement(flaggedIndex) ? "n" : ".");
  fString += (IsTerminalElement(flaggedIndex) ? "t" : ".");
  fString += (IsSupernode(flaggedIndex) ? "s" : ".");
  fString += (IsHypernode(flaggedIndex) ? "h" : ".");
  fString += (IsAscending(flaggedIndex) ? "a" : ".");
  return fString;
} // FlagString()

class EdgeDataHeight
{
public:
  // RegularNodeID (or sortIndex)
  Id I;
  // RegularNodeID (or sortIndex)
  Id J;
  // RegularNodeID (or sortIndex)
  Id SubtreeMin;
  // RegularNodeID (or sortIndex)
  Id SubtreeMax;
  bool UpEdge;
  Float64 SubtreeHeight;

  VTKM_EXEC
  bool operator<(const EdgeDataHeight& b) const
  {
    if (this->I == b.I)
    {
      if (this->UpEdge == b.UpEdge)
      {
        if (this->SubtreeHeight == b.SubtreeHeight)
        {
          if (this->SubtreeMin == b.SubtreeMin)
          {
            return this->SubtreeMax > b.SubtreeMax;
          }
          else
          {
            return this->SubtreeMin < b.SubtreeMin;
          }
        }
        else
        {
          return this->SubtreeHeight > b.SubtreeHeight;
        }
      }
      else
      {
        return this->UpEdge < b.UpEdge;
      }
    }
    else
    {
      return this->I < b.I;
    }
  }
};

class EdgeDataVolume
{
public:
  // RegularNodeID (or sortIndex)
  Id I;
  // RegularNodeID (or sortIndex)
  Id J;
  bool UpEdge;
  Id SubtreeVolume;

  VTKM_EXEC
  bool operator<(const EdgeDataVolume& b) const
  {
    if (this->I == b.I)
    {
      if (this->UpEdge == b.UpEdge)
      {
        if (this->SubtreeVolume == b.SubtreeVolume)
        {
          if (this->UpEdge == true)
          {
            return this->J > b.J;
          }
          else
          {
            return this->J < b.J;
          }
        }
        else
        {
          return this->SubtreeVolume > b.SubtreeVolume;
        }
      }
      else
      {
        return this->UpEdge < b.UpEdge;
      }
    }
    else
    {
      return this->I < b.I;
    }
  }
};


///
/// Helper struct to collect sizing information from a dataset.
/// The struct is used in the contour tree filter implementation
/// to determine the rows, cols, slices parameters from the
/// datasets so we can call the contour tree worklet properly.
///
struct GetRowsColsSlices
{
  //@{
  /// Get the number of rows, cols, and slices of a vtkm::cont::CellSetStructured
  /// @param[in] cells  The input vtkm::cont::CellSetStructured
  /// @param[out] nRows  Number of rows (x) in the cell set
  /// @param[out[ nCols  Number of columns (y) in the cell set
  /// @param[out] nSlices Number of slices (z) in the cell set
  void operator()(const vtkm::cont::CellSetStructured<2>& cells,
                  vtkm::Id& nRows,
                  vtkm::Id& nCols,
                  vtkm::Id& nSlices) const
  {
    vtkm::Id2 pointDimensions = cells.GetPointDimensions();
    nRows = pointDimensions[0];
    nCols = pointDimensions[1];
    nSlices = 1;
  }
  void operator()(const vtkm::cont::CellSetStructured<3>& cells,
                  vtkm::Id& nRows,
                  vtkm::Id& nCols,
                  vtkm::Id& nSlices) const
  {
    vtkm::Id3 pointDimensions = cells.GetPointDimensions();
    nRows = pointDimensions[0];
    nCols = pointDimensions[1];
    nSlices = pointDimensions[2];
  }
  //@}

  ///  Raise ErrorBadValue if the input cell set is not a vtkm::cont::CellSetStructured<2> or <3>
  template <typename T>
  void operator()(const T& cells, vtkm::Id& nRows, vtkm::Id& nCols, vtkm::Id& nSlices) const
  {
    (void)nRows;
    (void)nCols;
    (void)nSlices;
    (void)cells;
    throw vtkm::cont::ErrorBadValue("Expected 2D or 3D structured cell cet! ");
  }
};

} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
