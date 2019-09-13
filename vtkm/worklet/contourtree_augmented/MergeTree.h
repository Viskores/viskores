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


#ifndef vtkm_worklet_contourtree_augmented_mergetree_h
#define vtkm_worklet_contourtree_augmented_mergetree_h

#include <iomanip>

// local includes
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/ContourTreeMesh.h>


//VTKM includes
#include <vtkm/Types.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleConstant.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

class MergeTree
{ // class MergeTree
public:
  // whether it is join or split tree
  bool isJoinTree;

  // VECTORS INDEXED ON N = SIZE OF DATA

  // the list of nodes is implicit

  // vector of (regular) arcs in the merge tree
  IdArrayType arcs;

  // vector storing which superarc owns each node
  IdArrayType superparents;

  // VECTORS INDEXED ON T = SIZE OF TREE

  // vector storing the list of supernodes by ID
  // WARNING: THESE ARE NOT SORTED BY INDEX
  // Instead, they are sorted by hyperarc, secondarily on index
  IdArrayType supernodes;

  // vector of superarcs in the merge tree
  // stored as supernode indices
  IdArrayType superarcs;

  // vector of hyperarcs to which each supernode/arc belongs
  IdArrayType hyperparents;

  // VECTORS INDEXED ON H = SIZE OF HYPERTREE

  // vector of sort indices for the hypernodes
  IdArrayType hypernodes;

  // vector of hyperarcs in the merge tree
  // NOTE: These are supernode IDs, not hypernode IDs
  // because not all hyperarcs lead to hypernodes
  IdArrayType hyperarcs;

  // vector to find the first child superarc
  IdArrayType firstSuperchild;

  // ROUTINES

  // creates merge tree (empty)
  MergeTree(vtkm::Id meshSize, bool IsJoinTree);

  // debug routine
  void DebugPrint(const char* message, const char* fileName, long lineNum);

  // debug routine for printing the tree for contourtree meshes
  template <typename FieldType>
  void DebugPrintTree(const char* message,
                      const char* fileName,
                      long lineNum,
                      const ContourTreeMesh<FieldType>& mesh);
  // debug routine for printing the tree for regular meshes
  template <typename MeshType>
  void DebugPrintTree(const char* message,
                      const char* fileName,
                      long lineNum,
                      const MeshType& mesh);


}; // class MergeTree


// creates merge tree (empty)
inline MergeTree::MergeTree(vtkm::Id meshSize, bool IsJoinTree)
  : isJoinTree(IsJoinTree)
  , supernodes()
  , superarcs()
  , hyperparents()
  , hypernodes()
  , hyperarcs()
  , firstSuperchild()
{ // MergeTree()
  // Allocate the arcs array
  // TODO it should be sufficient to just allocate arcs without initializing it with 0s
  vtkm::cont::ArrayHandleConstant<vtkm::Id> meshSizeNullArray(0, meshSize);
  vtkm::cont::Algorithm::Copy(meshSizeNullArray, arcs);

  // Initialize the superparents with NO_SUCH_ELEMENT
  vtkm::cont::ArrayHandleConstant<vtkm::Id> noSuchElementArray((vtkm::Id)NO_SUCH_ELEMENT, meshSize);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, superparents);

} // MergeTree()


// debug routine
inline void MergeTree::DebugPrint(const char* message, const char* fileName, long lineNum)
{ // DebugPrint()
#ifdef DEBUG_PRINT
  std::cout << "---------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Merge Tree Contains:       " << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  printHeader(arcs.GetNumberOfValues());
  printIndices("Arcs", arcs);
  printIndices("Superparents", superparents);
  std::cout << std::endl;
  printHeader(supernodes.GetNumberOfValues());
  printIndices("Supernodes", supernodes);
  printIndices("Superarcs", superarcs);
  printIndices("Hyperparents", hyperparents);
  std::cout << std::endl;
  printHeader(hypernodes.GetNumberOfValues());
  printIndices("Hypernodes", hypernodes);
  printIndices("Hyperarcs", hyperarcs);
  printIndices("First Superchild", firstSuperchild);
  std::cout << std::endl;
#else
  // Prevent unused parameter warning
  (void)message;
  (void)fileName;
  (void)lineNum;
#endif
} // DebugPrint()


template <typename FieldType>
inline void MergeTree::DebugPrintTree(const char* message,
                                      const char* fileName,
                                      long lineNum,
                                      const ContourTreeMesh<FieldType>& mesh)
{
  (void)mesh; // prevent unused parameter warning
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "MergeTree::DebugPrintTree not implemented for ContourTreeMesh" << std::endl;
}



template <typename MeshType>
inline void MergeTree::DebugPrintTree(const char* message,
                                      const char* fileName,
                                      long lineNum,
                                      const MeshType& mesh)
{ //PrintMergeTree()
#ifdef DEBUG_PRINT
  std::cout << "---------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  if (isJoinTree)
  {
    std::cout << "Join Tree:" << std::endl;
  }
  else
  {
    std::cout << "Split Tree:" << std::endl;
  }
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  std::cout << "==========" << std::endl;

  for (vtkm::Id entry = 0; entry < mesh.nVertices; entry++)
  {
    vtkm::Id sortIndex = mesh.sortIndices.GetPortalConstControl().Get(entry);
    vtkm::Id arc = this->arcs.GetPortalConstControl().Get(sortIndex);
    if (noSuchElement(arc))
    {
      std::cout << "-1" << std::endl;
    }
    else
    {
      std::cout << mesh.sortOrder.GetPortalConstControl().Get(arc) << std::endl;
    }
    if (mesh.nDims == 2)
    {
      if ((entry % mesh.nCols) == (mesh.nCols - 1))
      {
        std::cout << std::endl;
      }
    }
    else if (mesh.nDims == 3)
    {
      if ((entry % (mesh.nCols * mesh.nRows)) == (mesh.nCols * mesh.nRows - 1))
      {
        std::cout << std::endl;
      }
    }
  }
  std::cout << std::endl;
#else
  // Prevent unused parameter warning
  (void)message;
  (void)fileName;
  (void)lineNum;
  (void)mesh;
#endif
} // PrintMergeTree()

} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
