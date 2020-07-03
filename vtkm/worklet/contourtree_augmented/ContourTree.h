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

#ifndef vtk_m_worklet_contourtree_augmented_contourtree_h
#define vtk_m_worklet_contourtree_augmented_contourtree_h

// global includes
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

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
constexpr const char* NODE_COLORS[N_NODE_COLORS] = { // nodeColors
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
  IdArrayType Nodes;

  // vector of (regular) arcs in the merge tree
  IdArrayType Arcs;

  // vector storing which superarc owns each node
  IdArrayType Superparents;

  // VECTORS INDEXED ON T = SIZE OF TREE

  // vector storing the list of supernodes by ID
  // WARNING: THESE ARE NOT SORTED BY INDEX
  // Instead, they are sorted by hyperarc, secondarily on index
  IdArrayType Supernodes;

  // vector of superarcs in the merge tree
  // stored as supernode indices
  IdArrayType Superarcs;

  // for boundary augmented contour tree (note: these use the same convention as supernodes/superarcs)
  IdArrayType Augmentnodes;
  IdArrayType Augmentarcs;

  // vector of Hyperarcs to which each supernode/arc belongs
  IdArrayType Hyperparents;

  // vector tracking which superarc was transferred on which iteration
  IdArrayType WhenTransferred;

  // VECTORS INDEXED ON H = SIZE OF HYPERTREE

  // vector of sort indices for the hypernodes
  IdArrayType Hypernodes;

  // vector of Hyperarcs in the merge tree
  // NOTE: These are supernode IDs, not hypernode IDs
  // because not all Hyperarcs lead to hypernodes
  IdArrayType Hyperarcs;

  // counter for the number of iterations it took to construct the tree
  // this is also used for hypersweep computations
  vtkm::Id NumIterations;

  // vectors tracking the segments used in each iteration of the hypersweep
  IdArrayType FirstSupernodePerIteration;
  IdArrayType FirstHypernodePerIteration;


  // ROUTINES

  // initialises contour tree arrays - rest is done by another class
  inline ContourTree();

  // initialises contour tree arrays - rest is done by another class
  inline void Init(vtkm::Id dataSize);

  // debug routine
  inline void DebugPrint(const char* message, const char* fileName, long lineNum) const;

  // print contents
  inline void PrintContent() const;

  // print routines
  inline void PrintDotSuperStructure() const;
  inline std::string PrintHyperStructureStatistics(bool print = true) const;

}; // class ContourTree



ContourTree::ContourTree()
  : Arcs()
  , Superparents()
  , Supernodes()
  , Superarcs()
  , Hyperparents()
  , Hypernodes()
  , Hyperarcs()
{ // ContourTree()
} // ContourTree()


// initialises contour tree arrays - rest is done by another class
void ContourTree::Init(vtkm::Id dataSize)
{ // Init()
  vtkm::cont::ArrayHandleConstant<vtkm::Id> noSuchElementArray(
    static_cast<vtkm::Id>(NO_SUCH_ELEMENT), dataSize);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, this->Arcs);
  vtkm::cont::Algorithm::Copy(noSuchElementArray, this->Superparents);
} // Init()


inline void ContourTree::PrintContent() const
{
  PrintHeader(this->Arcs.GetNumberOfValues());
  PrintIndices("Arcs", this->Arcs);
  PrintIndices("Superparents", this->Superparents);
  std::cout << std::endl;
  PrintHeader(this->Supernodes.GetNumberOfValues());
  PrintIndices("Supernodes", this->Supernodes);
  PrintIndices("Superarcs", this->Superarcs);
  PrintIndices("Hyperparents", this->Hyperparents);
  PrintIndices("When Xferred", this->WhenTransferred);
  std::cout << std::endl;
  PrintHeader(this->Hypernodes.GetNumberOfValues());
  PrintIndices("Hypernodes", this->Hypernodes);
  PrintIndices("Hyperarcs", this->Hyperarcs);
  PrintHeader(Augmentnodes.GetNumberOfValues());
  PrintIndices("Augmentnodes", Augmentnodes);
  PrintIndices("Augmentarcs", this->Augmentarcs);
  std::cout << std::endl;
  std::cout << "NumIterations: " << this->NumIterations << std::endl;
  PrintHeader(this->FirstSupernodePerIteration.GetNumberOfValues());
  PrintIndices("First SN Per Iter", this->FirstSupernodePerIteration);
  PrintIndices("First HN Per Iter", this->FirstHypernodePerIteration);
}

void ContourTree::DebugPrint(const char* message, const char* fileName, long lineNum) const
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

void ContourTree::PrintDotSuperStructure() const
{ // PrintDotSuperStructure()
  // print the header information
  printf("digraph G\n\t{\n");
  printf("\tsize=\"6.5, 9\"\n\tratio=\"fill\"\n");

  auto whenTransferredPortal = this->WhenTransferred.ReadPortal();
  auto supernodesPortal = this->Supernodes.ReadPortal();
  auto superarcsPortal = this->Superarcs.ReadPortal();
  auto hypernodesPortal = this->Hypernodes.ReadPortal();
  auto hyperparentsPortal = this->Hyperparents.ReadPortal();
  auto hyperarcsPortal = this->Hyperarcs.ReadPortal();

  // colour the nodes by the iteration they transfer (mod # of colors) - paired iterations have similar colors RGBCMY
  for (vtkm::Id supernode = 0; supernode < this->Supernodes.GetNumberOfValues(); supernode++)
  { // per supernode
    vtkm::Id iteration = MaskedIndex(whenTransferredPortal.Get(supernode));
    printf("\tnode s%lli [style=filled,fillcolor=%s]\n",
           static_cast<vtkm::Int64>(supernodesPortal.Get(supernode)),
           NODE_COLORS[iteration % N_NODE_COLORS]);
  } // per supernode

  // loop through supernodes
  for (vtkm::Id supernode = 0; supernode < this->Supernodes.GetNumberOfValues(); supernode++)
  { // per supernode
    // skip the global root
    if (NoSuchElement(superarcsPortal.Get(supernode)))
      continue;

    if (IsAscending(superarcsPortal.Get(supernode)))
      printf(
        "\tedge s%lli -> s%lli[label=S%lli,dir=back]\n",
        static_cast<vtkm::Int64>(supernodesPortal.Get(MaskedIndex(superarcsPortal.Get(supernode)))),
        static_cast<vtkm::Int64>(supernodesPortal.Get(supernode)),
        static_cast<vtkm::Int64>(supernode));
    else
      printf(
        "\tedge s%lli -> s%lli[label=S%lli]\n",
        static_cast<vtkm::Int64>(supernodesPortal.Get(supernode)),
        static_cast<vtkm::Int64>(supernodesPortal.Get(MaskedIndex(superarcsPortal.Get(supernode)))),
        static_cast<vtkm::Int64>(supernode));
  } // per supernode

  // now loop through hypernodes to show hyperarcs
  for (vtkm::Id hypernode = 0; hypernode < this->Hypernodes.GetNumberOfValues(); hypernode++)
  { // per hypernode
    // skip the global root
    if (NoSuchElement(hyperarcsPortal.Get(hypernode)))
      continue;

    printf(
      "\ts%lli -> s%lli [constraint=false][width=5.0][label=\"H%lli\\nW%lli\"]\n",
      static_cast<vtkm::Int64>(supernodesPortal.Get(hypernodesPortal.Get(hypernode))),
      static_cast<vtkm::Int64>(supernodesPortal.Get(MaskedIndex(hyperarcsPortal.Get(hypernode)))),
      static_cast<vtkm::Int64>(hypernode),
      static_cast<vtkm::Int64>(
        MaskedIndex(whenTransferredPortal.Get(hypernodesPortal.Get(hypernode)))));
  } // per hypernode

  // now add the hyperparents
  for (vtkm::Id supernode = 0; supernode < this->Supernodes.GetNumberOfValues(); supernode++)
  { // per supernode
    printf("\ts%lli -> s%lli [constraint=false][style=dotted]\n",
           static_cast<vtkm::Int64>(supernodesPortal.Get(supernode)),
           static_cast<vtkm::Int64>(
             supernodesPortal.Get(hypernodesPortal.Get(hyperparentsPortal.Get(supernode)))));
  } // per supernode

  // now use the hyperstructure to define subgraphs
  for (vtkm::Id hypernode = 0; hypernode < this->Hypernodes.GetNumberOfValues(); hypernode++)
  { // per hypernode
    vtkm::Id firstChild = hypernodesPortal.Get(hypernode);
    vtkm::Id childSentinel = (hypernode == this->Hypernodes.GetNumberOfValues() - 1)
      ? this->Supernodes.GetNumberOfValues()
      : hypernodesPortal.Get(hypernode + 1);
    printf("\tsubgraph H%lli{ ", static_cast<vtkm::Int64>(hypernode));
    for (vtkm::Id supernode = firstChild; supernode < childSentinel; supernode++)
    {
      printf("s%lli ", static_cast<vtkm::Int64>(supernodesPortal.Get(supernode)));
    }
    printf("}\n");
  } // per hypernode

  // print the footer information
  printf("\t}\n");
} // PrintDotSuperStructure()

std::string ContourTree::PrintHyperStructureStatistics(bool print) const
{ // PrintHyperStructureStatistics()
  // arrays for collecting statistics
  std::vector<vtkm::Id> minPath;
  std::vector<vtkm::Id> maxPath;
  std::vector<vtkm::Id> supernodeCount;
  std::vector<vtkm::Id> hypernodeCount;
  auto whenTransferredPortal = this->WhenTransferred.ReadPortal();
  auto hypernodesPortal = this->Hypernodes.ReadPortal();

  // set an initial iteration number to negative to get it started
  long whichIteration = -1;

  // loop through the hypernodes
  for (vtkm::Id hypernode = 0; hypernode < this->Hypernodes.GetNumberOfValues(); hypernode++)
  { // per hypernode
    // retrieve corresponding supernode ID
    vtkm::Id supernodeID = hypernodesPortal.Get(hypernode);
    // and the iteration of transfer
    vtkm::Id iterationNo = MaskedIndex(whenTransferredPortal.Get(supernodeID));

    // if it doesn't match, we've hit a boundary
    if (whichIteration != iterationNo)
    { // new iteration
      // initialise the next iteration
      // this one is larger than the maximum possible to force minimum
      minPath.push_back(static_cast<vtkm::Id>(this->Supernodes.GetNumberOfValues() + 1));
      maxPath.push_back(0);
      supernodeCount.push_back(0);
      hypernodeCount.push_back(0);
      // and increment the iteration ID
      whichIteration++;
    } // new iteration

    // now compute the new path length - default to off the end
    vtkm::Id pathLength = static_cast<vtkm::Id>(this->Supernodes.GetNumberOfValues() - supernodeID);
    // for all except the last, take the next one
    if (hypernode != this->Hypernodes.GetNumberOfValues() - 1)
    {
      pathLength = hypernodesPortal.Get(hypernode + 1) - supernodeID;
    }
    // update the statistics
    if (pathLength < minPath[static_cast<std::size_t>(whichIteration)])
    {
      minPath[static_cast<std::size_t>(whichIteration)] = pathLength;
    }
    if (pathLength > maxPath[static_cast<std::size_t>(whichIteration)])
    {
      maxPath[static_cast<std::size_t>(whichIteration)] = pathLength;
    }
    supernodeCount[static_cast<std::size_t>(whichIteration)] += pathLength;
    hypernodeCount[static_cast<std::size_t>(whichIteration)]++;
  } // per hypernode

  // now print out the statistics
  std::stringstream resultString;
  for (std::size_t iteration = 0; iteration < minPath.size(); iteration++)
  { // per iteration
    double averagePath = static_cast<double>(supernodeCount[iteration]) /
      static_cast<double>(hypernodeCount[iteration]);
    resultString << "Iteration: " << iteration << " Hyper: " << hypernodeCount[iteration]
                 << " Super: " << supernodeCount[iteration] << " Min: " << minPath[iteration]
                 << " Avg: " << averagePath << " Max: " << maxPath[iteration] << std::endl;
  } // per iteration
  resultString << "Total Hypernodes: " << this->Hypernodes.GetNumberOfValues()
               << " Supernodes: " << this->Supernodes.GetNumberOfValues() << std::endl;
  if (print)
  {
    std::cout << resultString.str() << std::endl;
  }

  return resultString.str();
} // PrintHyperStructureStatistics()


} // namespace contourtree_augmented
} // worklet
} // vtkm

#endif
