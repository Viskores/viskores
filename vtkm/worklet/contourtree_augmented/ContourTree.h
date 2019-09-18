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

#ifndef vtkm_worklet_contourtree_augmented_contourtree_h
#define vtkm_worklet_contourtree_augmented_contourtree_h

// global includes
#include <algorithm>
#include <iomanip>
#include <iostream>

// local includes
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

//VTKM includes
#include <vtkm/Pair.h>
#include <vtkm/Types.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleConstant.h>

namespace vtkm
{
namespace worklet
{
namespace contourtree_augmented
{

constexpr int N_NODE_COLORS = 12;
constexpr const char* nodeColors[N_NODE_COLORS] = { // nodeColors
  "red",  "red4",  "green",   "green4",   "royalblue", "royalblue4",
  "cyan", "cyan4", "magenta", "magenta4", "yellow",    "yellow4"
}; // nodeColors


struct SaddlePeakSort
{
  VTKM_EXEC_CONT
  inline bool operator()(const vtkm::Pair<vtkm::Id, vtkm::Id>& a,
                         const vtkm::Pair<vtkm::Id, vtkm::Id>& b) const
  {
    if (a.first < b.first)
      return true;
    if (a.first > b.first)
      return false;
    if (a.second < b.second)
      return true;
    if (a.second > b.second)
      return false;
    return false;
  }
};


class ContourTree
{ // class ContourTree
public:
  // VECTORS INDEXED ON N = SIZE OF DATA

  // the list of nodes is implicit - but for some purposes, it's useful to have them pre-sorted by superarc
  IdArrayType nodes;

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

  // for boundary augmented contour tree (note: these use the same convention as supernodes/superarcs)
  IdArrayType augmentnodes;
  IdArrayType augmentarcs;

  // vector of hyperarcs to which each supernode/arc belongs
  IdArrayType hyperparents;

  // vector tracking which superarc was transferred on which iteration
  IdArrayType whenTransferred;

  // VECTORS INDEXED ON H = SIZE OF HYPERTREE

  // vector of sort indices for the hypernodes
  IdArrayType hypernodes;

  // vector of hyperarcs in the merge tree
  // NOTE: These are supernode IDs, not hypernode IDs
  // because not all hyperarcs lead to hypernodes
  IdArrayType hyperarcs;


  // THIS ONE HAS BEEN DELETED BECAUSE IT'S THE SAME AS THE HYPERNODE ID
  // ALTHOUGH THIS IS NOT NECESSARY, IT'S THE RESULT OF THE CONSTRUCTION
  // vector to find the first child superarc
  // IdArrayType firstSuperchild;

  // ROUTINES

  // initialises contour tree arrays - rest is done by another class
  inline ContourTree();

  // initialises contour tree arrays - rest is done by another class
  inline void Init(vtkm::Id dataSize);

  // debug routine
  inline void DebugPrint(const char* message, const char* fileName, long lineNum);

  // print contents
  inline void PrintContent() const;

  // print routines
  //void PrintDotHyperStructure();
  inline void PrintDotSuperStructure();
  //void PrintDotRegularStructure();

}; // class ContourTree



ContourTree::ContourTree()
  : arcs()
  , superparents()
  , supernodes()
  , superarcs()
  , hyperparents()
  , hypernodes()
  , hyperarcs()
{ // ContourTree()
} // ContourTree()


// initialises contour tree arrays - rest is done by another class
void ContourTree::Init(vtkm::Id dataSize)
{ // Init()
  vtkm::cont::ArrayHandleConstant<vtkm::Id> noSuchElementArray((vtkm::Id)NO_SUCH_ELEMENT, dataSize);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, arcs);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, superparents);
} // Init()


inline void ContourTree::PrintContent() const
{
  printHeader(arcs.GetNumberOfValues());
  printIndices("Arcs", arcs);
  printIndices("Superparents", superparents);
  std::cout << std::endl;
  printHeader(supernodes.GetNumberOfValues());
  printIndices("Supernodes", supernodes);
  printIndices("Superarcs", superarcs);
  printIndices("Hyperparents", hyperparents);
  printIndices("When Xferred", whenTransferred);
  std::cout << std::endl;
  printHeader(hypernodes.GetNumberOfValues());
  printIndices("Hypernodes", hypernodes);
  printIndices("Hyperarcs", hyperarcs);
  printHeader(augmentnodes.GetNumberOfValues());
  printIndices("Augmentnodes", augmentnodes);
  printIndices("Augmentarcs", augmentarcs);
}

void ContourTree::DebugPrint(const char* message, const char* fileName, long lineNum)
{ // DebugPrint()
#ifdef DEBUG_PRINT
  std::cout << "---------------------------" << std::endl;
  std::cout << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
            << lineNum << std::endl;
  std::cout << std::left << std::string(message) << std::endl;
  std::cout << "Contour Tree Contains:     " << std::endl;
  std::cout << "---------------------------" << std::endl;
  std::cout << std::endl;

  this->PrintContent();
#else
  // Avoid unused parameter warnings
  (void)message;
  (void)fileName;
  (void)lineNum;
#endif
} // DebugPrint()

// print routines
// void ContourTree::PrintDotHyperStructure()
//        { // PrintDotHyperStructure()
//        } // PrintDotHyperStructure()


void ContourTree::PrintDotSuperStructure()
{ // PrintDotSuperStructure()
  // print the header information
  printf("digraph G\n\t{\n");
  printf("\tsize=\"6.5, 9\"\n\tratio=\"fill\"\n");

  auto whenTransferredPortal = whenTransferred.GetPortalConstControl();
  auto supernodesPortal = supernodes.GetPortalConstControl();
  auto superarcsPortal = superarcs.GetPortalConstControl();
  auto hypernodesPortal = hypernodes.GetPortalConstControl();
  auto hyperparentsPortal = hyperparents.GetPortalConstControl();
  auto hyperarcsPortal = hyperarcs.GetPortalConstControl();

  // colour the nodes by the iteration they transfer (mod # of colors) - paired iterations have similar colors RGBCMY
  for (vtkm::Id supernode = 0; supernode < supernodes.GetNumberOfValues(); supernode++)
  { // per supernode
    vtkm::Id iteration = maskedIndex(whenTransferredPortal.Get(supernode));
    printf("\tnode s%lli [style=filled,fillcolor=%s]\n",
           (vtkm::Int64)supernodesPortal.Get(supernode),
           nodeColors[iteration % N_NODE_COLORS]);
  } // per supernode

  // loop through supernodes
  for (vtkm::Id supernode = 0; supernode < supernodes.GetNumberOfValues(); supernode++)
  { // per supernode
    // skip the global root
    if (noSuchElement(superarcsPortal.Get(supernode)))
      continue;

    if (isAscending(superarcsPortal.Get(supernode)))
      printf("\tedge s%lli -> s%lli[label=S%lli,dir=back]\n",
             (vtkm::Int64)supernodesPortal.Get(maskedIndex(superarcsPortal.Get(supernode))),
             (vtkm::Int64)supernodesPortal.Get(supernode),
             (vtkm::Int64)supernode);
    else
      printf("\tedge s%lli -> s%lli[label=S%lli]\n",
             (vtkm::Int64)supernodesPortal.Get(supernode),
             (vtkm::Int64)supernodesPortal.Get(maskedIndex(superarcsPortal.Get(supernode))),
             (vtkm::Int64)supernode);
  } // per supernode

  // now loop through hypernodes to show hyperarcs
  for (vtkm::Id hypernode = 0; hypernode < hypernodes.GetNumberOfValues(); hypernode++)
  { // per hypernode
    // skip the global root
    if (noSuchElement(hyperarcsPortal.Get(hypernode)))
      continue;

    printf("\ts%lli -> s%lli [constraint=false][width=5.0][label=\"H%lli\\nW%lli\"]\n",
           (vtkm::Int64)supernodesPortal.Get(hypernodesPortal.Get(hypernode)),
           (vtkm::Int64)supernodesPortal.Get(maskedIndex(hyperarcsPortal.Get(hypernode))),
           (vtkm::Int64)hypernode,
           (vtkm::Int64)maskedIndex(whenTransferredPortal.Get(hypernodesPortal.Get(hypernode))));
  } // per hypernode

  // now add the hyperparents
  for (vtkm::Id supernode = 0; supernode < supernodes.GetNumberOfValues(); supernode++)
  { // per supernode
    printf(
      "\ts%lli -> s%lli [constraint=false][style=dotted]\n",
      (vtkm::Int64)supernodesPortal.Get(supernode),
      (vtkm::Int64)supernodesPortal.Get(hypernodesPortal.Get(hyperparentsPortal.Get(supernode))));
  } // per supernode

  // now use the hyperstructure to define subgraphs
  for (vtkm::Id hypernode = 0; hypernode < hypernodes.GetNumberOfValues(); hypernode++)
  { // per hypernode
    vtkm::Id firstChild = hypernodesPortal.Get(hypernode);
    vtkm::Id childSentinel = (hypernode == hypernodes.GetNumberOfValues() - 1)
      ? supernodes.GetNumberOfValues()
      : hypernodesPortal.Get(hypernode + 1);
    printf("\tsubgraph H%lli{ ", (vtkm::Int64)hypernode);
    for (vtkm::Id supernode = firstChild; supernode < childSentinel; supernode++)
    {
      printf("s%lli ", (vtkm::Int64)supernodesPortal.Get(supernode));
    }
    printf("}\n");
  } // per hypernode

  // print the footer information
  printf("\t}\n");
} // PrintDotSuperStructure()

// void ContourTree::PrintDotRegularStructure()
//        { // PrintDotRegularStructure()
//        } // PrintDotRegularStructure()

} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
