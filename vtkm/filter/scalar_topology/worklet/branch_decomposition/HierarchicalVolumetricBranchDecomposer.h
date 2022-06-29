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
//=======================================================================================
//
//  Parallel Peak Pruning v. 2.0
//
//  Started June 15, 2017
//
// Copyright Hamish Carr, University of Leeds
//
// HierarchicalVolumetricBranchDecomposer.h
//
//=======================================================================================
//
// COMMENTS:
//
//      This class computes the branch decomposition by volume for a given hierarchical
//      contour tree.
//
//      It takes as input arrays of dependent and intrinsic volumes for each superarc
//      (it needs both, in order to compute the dependent volume at each end of each superarc)
//
//      Recall from the non-hierarchical version that in order to compute the branch decomposition,
//      we need to choose the "best up" and "best down" superarc for each supernode - i.e. the
//      superarc with the largest dependent volume. Since we only wish to compare superarcs that
//      meet at a given supernode, we tiebreak by always taking the superarc whose "other" end
//      has a higher ID.
//
//      Once the best up & best down have been found for each supernode, branches are identified
//      with (essentially) a graph connectivity computation.
//
//      Conceptually, each superarc is a vertex in a new (temporary) graph. For each supernode, the
//      "best up" superarc is connected to the "best down" superarc.  This defines a graph in which
//      each branch is a connected component.  A single path-doubling pass then collects the branches
//
//      In the non-hierarchical version, this was done with supernode IDs (I can't remember why),
//      with the upper end of each branch being treated as the root node.
//
//      To construct the hierarchical branch decomposition, we assume that the hierarchical contour
//      tree has already been augmented with all attachment points.  If not, the code may produce
//      undefined results.
//
//      In the first step, we will run a local routine for each rank to determine the best up / down
//      as far as the rank knows.  We will then do a fan-in swap to determine the best up / down for
//      shared vertices. At the end of this step, all ranks will share the knowledge of the best
//      up / down superarc, stored as:
//      i.              the superarc ID, which may be reused on other ranks
//      ii.             the global ID of the outer end of that superarc, which is unique across all ranks
//      iii.    the volume dependent on that superarc
//
//      In the second stage, each rank will do a local computation of the branches. However, most ranks
//      will not have the full set of supernodes / superarcs for each branch, even (or especially)
//      for the master branch.  It is therefore a bad idea to collapse to the upper end of the branch
//      as we did in the non-hierarchical version.
//
//      Instead, we will define the root of each component to be the most senior superarc ID.  This will
//      be canonical, because of the way we construct the hierarchical tree, with low superarc IDs
//      occurring at higher levels of the tree, so all shared superarcs are a prefix set.  Therefore,
//      the most senior superarc ID will always indicate the highest level of the tree through which the
//      branch passes, and is safe.  Moreover, it is not necessary for each rank to determine the full
//      branch, merely the part of the branch that passes through the superarcs it tracks.  It may even
//      happen that no single rank stores the entire branch, as for example if the global minimum
//      and maximum are interior to different ranks.
//
//      Note that most senior means testing iteration, round, then ID
//
//      RESIZE SEMANTICS:  Oliver Ruebel has asked for the semantics of all resize() calls to be annotated
//      in order to ease porting to vtkm.  These will be flagged with a RESIZE SEMANTICS: comment, and will
//      generally fall into several patterns:
//      1.      FIXED:                  Resize() is used to initialize the array size for an array that will never change size
//      2.      COMPRESS:               Resize() is used after a compression operation (eg remove_if()) so that the
//                                              array size() call does not include the elements removed.  This is a standard
//                                              C++ pattern, but could be avoided by storing an explicit element count (curiously,
//                                              the std::vector class does exactly this with logical vs. physical array sizes).
//      3.      MULTI-COMPRESS: Resize() may also be used (as in the early stages of the PPP algorithm) to give
//                                              a collapsing array size of working elements.  Again, this could on principle by
//                                              avoided with an array count, but is likely to be intricate.
//
//=======================================================================================


#ifndef vtk_m_filter_scalar_topology_worklet_HierarchicalVolumetricBranchDecomposer_h
#define vtk_m_filter_scalar_topology_worklet_HierarchicalVolumetricBranchDecomposer_h

#include <iomanip>
#include <string>

// Contour tree includes, not yet moved into new filter structure
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/PrintGraph.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/FindRegularByGlobal.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/hierarchical_contour_tree/FindSuperArcBetweenNodes.h>

// Worklets
#include <vtkm/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/CollapseBranchesPointerDoublingWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/CollapseBranchesWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/LocalBestUpDownByVolumeBestUpDownEdgeWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/LocalBestUpDownByVolumeInitSuperarcListWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/LocalBestUpDownByVolumeWorklet.h>
#include <vtkm/filter/scalar_topology/worklet/branch_decomposition/hierarchical_volumetric_branch_decomposer/SuperArcVolumetricComparatorIndirectGlobalIdComparator.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/NotNoSuchElementPredicate.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>

#ifdef DEBUG_PRINT
#define DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
#endif

namespace vtkm
{
namespace filter
{
namespace scalar_topology
{

/// Facture class for augmenting the hierarchical contour tree to enable computations of measures, e.g., volumne
class HierarchicalVolumetricBranchDecomposer
{ // class HierarchicalVolumetricBranchDecomposer
public:
  /// we will want arrays for swapping with our partners, holding the best up/down superarc & the corresponding volume
  /// the best up/down will be in local supernode IDs initially, but during the swap will need to be global node IDs
  vtkm::worklet::contourtree_augmented::IdArrayType BestUpSupernode;
  vtkm::worklet::contourtree_augmented::IdArrayType BestDownSupernode;
  vtkm::worklet::contourtree_augmented::IdArrayType BestUpVolume;
  vtkm::worklet::contourtree_augmented::IdArrayType BestDownVolume;

  /// working arrays - kept at class level to simplify debug print
  vtkm::worklet::contourtree_augmented::IdArrayType UpVolume;
  vtkm::worklet::contourtree_augmented::IdArrayType DownVolume;

  /// routines to compute branch decomposition by volume
  /// WARNING: we now have two types of hierarchical tree sharing a data structure:
  ///   I.      hierarchical tree without augmentation
  ///   II.     hierarchical tree with augmentation
  /// We only expect to call this for II, but it's wiser to make sure that it computes for I as well.
  /// Also, this code is substantially identical to ContourTreeMaker::ComputeVolumeBranchDecomposition()
  /// except for:
  ///   A.      it has to deal with the round/iteration paradigm of hierarchical trees, and
  ///   B.      Stages III-IV in particular are modified
  ///   C.      Several stages involve fan-ins
  /// The principal reason for the modifications in B. is that the old code collapses branches to their maximum
  /// which is often a leaf. In the hierarchical version, the leaf will often not be represented on all ranks, so
  /// we modify it to collapse towards the "most senior".  This will be easiest if we collapse by superarc IDs instead of supernode IDs
  /// For C., we have to break the code into separate routines so that the fan-in MPI can be outside this unit.
  ///
  /// WARNING! WARNING! WARNING!
  /// In the non-hierarchical version, the last (virtual root) superarc goes from the highest ID supernode to NO_SUCH_ELEMENT
  /// If it was included in the sorts, this could cause problems
  /// The (simple) way out of this was to set nSuperarcs = nSupernodes - 1 when copying our temporary list of superarcs
  /// that way we don't use it at all.
  /// In the hierarchical version, this no longer works, because attachment points may also have virtual superarcs
  /// So we either need to compress them out (an extra log step) or ignore them in the later loop.
  /// Of the two, compressing them out is safer
  ///
  /// routine that determines the best upwards/downwards edges at each vertex
  /// Unlike the local version, the best might only be stored on another rank
  /// so we will compute the locally best up or down, then swap until all ranks choose the same best
  void LocalBestUpDownByVolume(const vtkm::cont::DataSet& hierarchicalTreeDataSet,
                               const vtkm::cont::ArrayHandle<vtkm::Id>& intrinsicValues,
                               const vtkm::cont::ArrayHandle<vtkm::Id>& dependentValues,
                               vtkm::Id totalVolume);

  /// routine to compute the local set of superarcs that root at a given one
  void CollapseBranches(const vtkm::cont::DataSet& hierarchicalTreeDataSet,
                        vtkm::worklet::contourtree_augmented::IdArrayType& branchRoot);

  /// routines to print branches
  template <typename IdArrayHandleType, typename DataValueArrayHandleType>
  static std::string PrintBranches(const IdArrayHandleType& hierarchicalTreeSuperarcsAH,
                                   const IdArrayHandleType& hierarchicalTreeSupernodesAH,
                                   const IdArrayHandleType& hierarchicalTreeRegularNodeGlobalIdsAH,
                                   const DataValueArrayHandleType& hierarchicalTreeDataValuesAH,
                                   const IdArrayHandleType& branchRootAH);
  static std::string PrintBranches(const vtkm::cont::DataSet& ds);

  /// debug routine
  std::string DebugPrint(std::string message, const char* fileName, long lineNum);

private:
  /// Used internally to Invoke worklets
  vtkm::cont::Invoker Invoke;

}; // class HierarchicalVolumetricBranchDecomposer


inline void HierarchicalVolumetricBranchDecomposer::LocalBestUpDownByVolume(
  const vtkm::cont::DataSet& hierarchicalTreeDataSet,
  const vtkm::cont::ArrayHandle<vtkm::Id>& intrinsicValues,
  const vtkm::cont::ArrayHandle<vtkm::Id>& dependentValues,
  vtkm::Id totalVolume)
{
  // Get required arrays for hierarchical tree form data set
  auto hierarchicalTreeSupernodes = hierarchicalTreeDataSet.GetField("Supernodes")
                                      .GetData()
                                      .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto hierarchicalTreeSuperarcs = hierarchicalTreeDataSet.GetField("Superarcs")
                                     .GetData()
                                     .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto hierarchicalTreeRegularNodeGlobalIds =
    hierarchicalTreeDataSet.GetField("RegularNodeGlobalIds")
      .GetData()
      .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  // LocalBestUpDownByVolume
  // STAGE I:   Allocate memory for our arrays
  vtkm::Id nSupernodes = hierarchicalTreeSupernodes.GetNumberOfValues();
  // WARNING: This differs from the non-hierarchical version by using the full size *WITH* virtual superarcs
  vtkm::Id nSuperarcs = hierarchicalTreeSuperarcs.GetNumberOfValues();

  // set up a list of superarcs as Edges for reference in our comparator
  vtkm::worklet::contourtree_augmented::EdgePairArray superarcList;
  superarcList.Allocate(nSuperarcs);
  this->Invoke(vtkm::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
                 LocalBestUpDownByVolumeInitSuperarcListWorklet{}, // the worklet
               hierarchicalTreeSuperarcs,                          // input
               superarcList                                        // output
  );

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  {
    std::stringstream resultStream;
    vtkm::worklet::contourtree_augmented::PrintHeader(superarcList.GetNumberOfValues(),
                                                      resultStream);
    vtkm::worklet::contourtree_augmented::PrintEdgePairArray(
      "Superarc List", superarcList, -1, resultStream);
    resultStream << std::endl;
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, resultStream.str());
  }
#endif

  // create a list of the non-virtual superarcs
  vtkm::worklet::contourtree_augmented::IdArrayType actualSuperarcs;
  // and fill it up with index values [0, 1, 2 ... nSuperarcs-1] while simultaneously stream compacting the
  // values by keeping only those indices where the hierarchicalTree->Superarcs is not NoSuchElement.
  vtkm::cont::Algorithm::CopyIf(vtkm::cont::ArrayHandleIndex(nSuperarcs), //input
                                hierarchicalTreeSuperarcs,                // stencil
                                actualSuperarcs,                          // output target array
                                vtkm::worklet::contourtree_augmented::NotNoSuchElementPredicate{});
  // NOTE: The behavior here is slightly different from the original implementation, as the original code
  //       here does not resize actualSuperarcs but keeps it at the full length of nSuperacs and instead
  //       relies on the nActualSuperarcs parameter. However, the extra values are never used, so compacting
  //       the array here should be fine.
  vtkm::Id nActualSuperarcs = actualSuperarcs.GetNumberOfValues();

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  {
    std::stringstream resultStream;
    vtkm::worklet::contourtree_augmented::PrintHeader(nActualSuperarcs, resultStream);
    vtkm::worklet::contourtree_augmented::PrintIndices(
      "Actual Superarcs", actualSuperarcs, -1, resultStream);
    resultStream << std::endl;
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, resultStream.str());
  }
#endif

  // and set up arrays for the best upwards, downwards superarcs at each supernode
  // initialize everything to NO_SUCH_ELEMENT for safety (we will test against this, so it's necessary)
  // Set up temporary constant arrays for each group of arrays and initalize the arrays
  // Initalize the arrays
  using vtkm::worklet::contourtree_augmented::NO_SUCH_ELEMENT;
  this->UpVolume.AllocateAndFill(nSuperarcs, 0);
  this->DownVolume.AllocateAndFill(nSuperarcs, 0);
  this->BestUpSupernode.AllocateAndFill(nSupernodes, NO_SUCH_ELEMENT);
  this->BestDownSupernode.AllocateAndFill(nSupernodes, NO_SUCH_ELEMENT);
  this->BestUpVolume.AllocateAndFill(nSupernodes, 0);
  this->BestDownVolume.AllocateAndFill(nSupernodes, 0);

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, DebugPrint("Arrays Allocated", __FILE__, __LINE__));
#endif

  // STAGE II: Pick the best (largest volume) edge upwards and downwards
  // II A. Compute the up / down volumes for indirect sorting
  // this is the same in spirit as ContourTreeMaker::ComputeVolumeBranchDecomposition() STAGE II A.
  // given that we have already suppressed the non-virtual superarcs
  // however, in this case, we need to use the actualSuperarcs array instead of the main array
  {
    vtkm::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      LocalBestUpDownByVolumeBestUpDownEdgeWorklet bestUpDownEdgeWorklet(totalVolume);
    // permut input and output arrays here so we can use FieldIn and FieldOut to
    // avoid the use of WholeArray access in the worklet
    auto permutedHierarchicalTreeSuperarcs =
      vtkm::cont::make_ArrayHandlePermutation(actualSuperarcs, hierarchicalTreeSuperarcs); // input
    auto permutedDependetValues =
      vtkm::cont::make_ArrayHandlePermutation(actualSuperarcs, dependentValues); // input
    auto permutedIntrinsicValues =
      vtkm::cont::make_ArrayHandlePermutation(actualSuperarcs, intrinsicValues); // input
    auto permutedUpVolume =
      vtkm::cont::make_ArrayHandlePermutation(actualSuperarcs, this->UpVolume); // output
    auto permitedDownVolume =
      vtkm::cont::make_ArrayHandlePermutation(actualSuperarcs, this->DownVolume); // outout

    this->Invoke(bestUpDownEdgeWorklet,             // the worklet
                 permutedHierarchicalTreeSuperarcs, // input
                 permutedDependetValues,            // input
                 permutedIntrinsicValues,           // input
                 permutedUpVolume,                  // output
                 permitedDownVolume                 // outout
    );
  }

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, DebugPrint("Volume Arrays Set Up", __FILE__, __LINE__));
  {
    std::stringstream resultStream;
    vtkm::worklet::contourtree_augmented::PrintHeader(superarcList.GetNumberOfValues(),
                                                      resultStream);
    vtkm::worklet::contourtree_augmented::PrintEdgePairArray(
      "Superarc List", superarcList, -1, resultStream);
    resultStream << std::endl;
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, resultStream.str());
  }
#endif
  // II B. Pick the best downwards volume by sorting on upper vertex then processing by segments (segmented by vertex)
  // II B 1.    Sort the superarcs by upper vertex
  // NB:  We reuse the actual superarcs list here - this works because we have indexed the volumes on the underlying superarc ID
  // NB 2: Notice that we only sort the "actual" ones - this is to avoid unnecessary resize() calls in vtkm later on
  {
    vtkm::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      SuperArcVolumetricComparatorIndirectGlobalIdComparator
        SuperArcVolumetricComparatorIndirectGlobalIdComparator(
          this->UpVolume, superarcList, hierarchicalTreeRegularNodeGlobalIds, false);
    vtkm::cont::Algorithm::Sort(actualSuperarcs,
                                SuperArcVolumetricComparatorIndirectGlobalIdComparator);
  }

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  {
    std::stringstream resultStream;
    resultStream
      << "Actual Superarc List After Sorting By High End (Full Array, including ignored elements)"
      << std::endl;
    vtkm::worklet::contourtree_augmented::PrintHeader(nActualSuperarcs, resultStream);
    vtkm::worklet::contourtree_augmented::PrintIndices(
      "Actual Superarcs", actualSuperarcs, -1, resultStream);
    resultStream << std::endl;
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, resultStream.str());
  }
#endif
  // II B 2.  Per vertex, best superarc writes to the best downward array
  {
    auto permutedUpVolume =
      vtkm::cont::make_ArrayHandlePermutation(actualSuperarcs, this->UpVolume);
    this->Invoke(vtkm::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
                   LocalBestUpDownByVolumeWorklet<true>{ nActualSuperarcs },
                 actualSuperarcs,                      // input
                 superarcList,                         // input
                 permutedUpVolume,                     // input
                 hierarchicalTreeRegularNodeGlobalIds, // input
                 hierarchicalTreeSupernodes,           // input
                 this->BestDownSupernode,              // output
                 this->BestDownVolume                  // output
    );
  }
#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             DebugPrint("BestDownSupernode Written", __FILE__, __LINE__));
#endif

  // II B 3.  Repeat for lower vertex
  {
    vtkm::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      SuperArcVolumetricComparatorIndirectGlobalIdComparator
        SuperArcVolumetricComparatorIndirectGlobalIdComparator(
          this->DownVolume, superarcList, hierarchicalTreeRegularNodeGlobalIds, true);
    vtkm::cont::Algorithm::Sort(actualSuperarcs,
                                SuperArcVolumetricComparatorIndirectGlobalIdComparator);
  }

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  {
    std::stringstream resultStream;
    resultStream
      << "Actual Superarc List After Sorting By Low End (Full Array, including ignored elements)"
      << std::endl;
    vtkm::worklet::contourtree_augmented::PrintHeader(nActualSuperarcs, resultStream);
    vtkm::worklet::contourtree_augmented::PrintIndices(
      "Actual Superarcs", actualSuperarcs, -1, resultStream);
    resultStream << std::endl;
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, resultStream.str());
  }
#endif

  // II B 2.  Per vertex, best superarc writes to the best upward array
  {
    auto permutedDownVolume =
      vtkm::cont::make_ArrayHandlePermutation(actualSuperarcs, this->DownVolume);
    this->Invoke(vtkm::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
                   LocalBestUpDownByVolumeWorklet<false>{ nActualSuperarcs },
                 actualSuperarcs,                      // input
                 superarcList,                         // input
                 permutedDownVolume,                   // input
                 hierarchicalTreeRegularNodeGlobalIds, // input
                 hierarchicalTreeSupernodes,           // input
                 this->BestUpSupernode,                // output
                 this->BestUpVolume                    // output
    );
  }

#ifdef DEBUG_HIERARCHICAL_VOLUMETRIC_BRANCH_DECOMPOSER
  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             DebugPrint("Local Best Up/Down Computed", __FILE__, __LINE__));
#endif
} // LocalBestUpDownByVolume


inline void HierarchicalVolumetricBranchDecomposer::CollapseBranches(
  const vtkm::cont::DataSet& hierarchicalTreeDataSet,
  vtkm::worklet::contourtree_augmented::IdArrayType& branchRoot)
{ // CollapseBranches
  // Get required arrays for hierarchical tree form data set
  auto hierarchicalTreeSupernodes = hierarchicalTreeDataSet.GetField("Supernodes")
                                      .GetData()
                                      .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto hierarchicalTreeSuperarcs = hierarchicalTreeDataSet.GetField("Superarcs")
                                     .GetData()
                                     .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto hierarchicalTreeRegularNodeGlobalIds =
    hierarchicalTreeDataSet.GetField("RegularNodeGlobalIds")
      .GetData()
      .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto hierarchicalTreeRegularNodeSortOrder =
    hierarchicalTreeDataSet.GetField("RegularNodeSortOrder")
      .GetData()
      .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto hierarchicalTreeRegular2Supernode = hierarchicalTreeDataSet.GetField("Regular2Supernode")
                                             .GetData()
                                             .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto hierarchicalTreeWhichRound = hierarchicalTreeDataSet.GetField("WhichRound")
                                      .GetData()
                                      .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  // initialise the superarcs to be their own branch roots
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(branchRoot.GetNumberOfValues()), branchRoot);

  //    For each supernode, convert the best up into a superarc ID
  {
    vtkm::worklet::contourtree_distributed::FindRegularByGlobal findRegularByGlobal{
      hierarchicalTreeRegularNodeSortOrder, hierarchicalTreeRegularNodeGlobalIds
    };
    vtkm::worklet::contourtree_distributed::FindSuperArcBetweenNodes findSuperArcBetweenNodes{
      hierarchicalTreeSuperarcs
    };

    using vtkm::worklet::scalar_topology::hierarchical_volumetric_branch_decomposer::
      CollapseBranchesWorklet;
    this->Invoke(CollapseBranchesWorklet{},         // the worklet
                 this->BestUpSupernode,             // input
                 this->BestDownSupernode,           // input
                 findRegularByGlobal,               // input ExecutionObject
                 findSuperArcBetweenNodes,          // input ExecutionObject
                 hierarchicalTreeRegular2Supernode, // input
                 hierarchicalTreeWhichRound,        // input
                 branchRoot);
  }

  // OK.  We've now initialized it, and can use pointer-doubling
  // Compute the number of log steps required
  vtkm::Id nLogSteps = 1;
  for (vtkm::Id shifter = branchRoot.GetNumberOfValues(); shifter != 0; shifter >>= 1)
  {
    nLogSteps++;
  }

  // loop that many times, pointer-doubling
  for (vtkm::Id iteration = 0; iteration < nLogSteps; iteration++)
  { // per iteration
    // loop through the vertices, updating
    using vtkm::filter::scalar_topology::hierarchical_volumetric_branch_decomposer::
      CollapseBranchesPointerDoublingWorklet;
    this->Invoke(CollapseBranchesPointerDoublingWorklet{}, branchRoot);
  } // per iteration
} // CollapseBranches


// PrintBranches
// we want to dump out the branches as viewed by this rank.
// most of the processing will be external, so we keep this simple.
// For each end of the superarc, we print out value & global ID prefixed by global ID of the branch root
// The external processing will then sort them to construct segments (as usual) in the array
// then a post-process can find the first and last in each segment & print out the branch
// In order for the sort to work lexicographically, we need to print out in the following order:
//            I       Branch Root Global ID
//            II      Supernode Value
//            III     Supernode Global ID

// Note the following is a template to be called via cast-and-call
template <typename IdArrayHandleType, typename DataValueArrayHandleType>
std::string HierarchicalVolumetricBranchDecomposer::PrintBranches(
  const IdArrayHandleType& hierarchicalTreeSuperarcsAH,
  const IdArrayHandleType& hierarchicalTreeSupernodesAH,
  const IdArrayHandleType& hierarchicalTreeRegularNodeGlobalIdsAH,
  const DataValueArrayHandleType& hierarchicalTreeDataValuesAH,
  const IdArrayHandleType& branchRootAH)
{
  auto hierarchicalTreeSuperarcsPortal = hierarchicalTreeSuperarcsAH.ReadPortal();
  vtkm::Id nSuperarcs = hierarchicalTreeSuperarcsAH.GetNumberOfValues();
  auto hierarchicalTreeSupernodesPortal = hierarchicalTreeSupernodesAH.ReadPortal();
  auto hierarchicalTreeRegularNodeGlobalIdsPortal =
    hierarchicalTreeRegularNodeGlobalIdsAH.ReadPortal();
  auto hierarchicalTreeDataValuesPortal = hierarchicalTreeDataValuesAH.ReadPortal();
  auto branchRootPortal = branchRootAH.ReadPortal();

  std::stringstream resultStream;
  // loop through the individual superarcs

  for (vtkm::Id superarc = 0; superarc < nSuperarcs; superarc++)
  { // per superarc
    // explicit test for root superarc / attachment points
    if (vtkm::worklet::contourtree_augmented::NoSuchElement(
          hierarchicalTreeSuperarcsPortal.Get(superarc)))
    {
      continue;
    }

    // now retrieve the branch root's global ID
    vtkm::Id branchRootSuperId = branchRootPortal.Get(superarc);
    vtkm::Id branchRootRegularId = hierarchicalTreeSupernodesPortal.Get(branchRootSuperId);
    vtkm::Id branchRootGlobalId =
      hierarchicalTreeRegularNodeGlobalIdsPortal.Get(branchRootRegularId);

    // now retrieve the global ID & value for each end & output them
    vtkm::Id superFromRegularId = hierarchicalTreeSupernodesPortal.Get(superarc);
    vtkm::Id superFromGlobalId = hierarchicalTreeRegularNodeGlobalIdsPortal.Get(superFromRegularId);
    typename DataValueArrayHandleType::ValueType superFromValue =
      hierarchicalTreeDataValuesPortal.Get(superFromRegularId);
    resultStream << branchRootGlobalId << "\t" << superFromValue << "\t" << superFromGlobalId
                 << std::endl;

    // now retrieve the global ID & value for each end & output them
    vtkm::Id superToRegularId = vtkm::worklet::contourtree_augmented::MaskedIndex(
      hierarchicalTreeSuperarcsPortal.Get(superarc));
    vtkm::Id superToGlobalId = hierarchicalTreeRegularNodeGlobalIdsPortal.Get(superToRegularId);
    typename DataValueArrayHandleType::ValueType superToValue =
      hierarchicalTreeDataValuesPortal.Get(superToRegularId);
    resultStream << branchRootGlobalId << "\t" << superToValue << "\t" << superToGlobalId
                 << std::endl;
  } // per superarc

  return resultStream.str();
} // PrintBranches

inline std::string HierarchicalVolumetricBranchDecomposer::PrintBranches(
  const vtkm::cont::DataSet& ds)
{
  auto hierarchicalTreeSuperarcsAH =
    ds.GetField("Superarcs").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();
  auto hierarchicalTreeSupernodesAH =
    ds.GetField("Supernodes").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  auto hierarchicalTreeRegularNodeGlobalIdsAH =
    ds.GetField("RegularNodeGlobalIds")
      .GetData()
      .AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  auto hierarchicalTreeDataValuesData = ds.GetField("DataValues").GetData();

  auto branchRootAH =
    ds.GetField("BranchRoots").GetData().AsArrayHandle<vtkm::cont::ArrayHandle<vtkm::Id>>();

  std::string result;

  hierarchicalTreeDataValuesData.CastAndCallForTypes<TypeListScalarAll, VTKM_DEFAULT_STORAGE_LIST>(
    [&](const auto& hierarchicalTreeDataValuesAH) {
      result = PrintBranches(hierarchicalTreeSuperarcsAH,
                             hierarchicalTreeSupernodesAH,
                             hierarchicalTreeRegularNodeGlobalIdsAH,
                             hierarchicalTreeDataValuesAH,
                             branchRootAH);
    });

  return result;
} // PrintBranches


// debug routine
inline std::string HierarchicalVolumetricBranchDecomposer::DebugPrint(std::string message,
                                                                      const char* fileName,
                                                                      long lineNum)
{ // DebugPrint()
  std::stringstream resultStream;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::setw(30) << std::left << fileName << ":" << std::right << std::setw(4)
               << lineNum << std::endl;
  resultStream << std::left << message << std::endl;
  resultStream << "Hypersweep Value Array Contains:        " << std::endl;
  resultStream << "----------------------------------------" << std::endl;
  resultStream << std::endl;

  vtkm::worklet::contourtree_augmented::PrintHeader(this->UpVolume.GetNumberOfValues(),
                                                    resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Up Volume by SA", this->UpVolume, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Down Volume by SA", this->DownVolume, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Best Down Snode by SN", this->BestDownSupernode, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Best Down Volume by SN", this->BestDownVolume, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Best Up Snode by SN", this->BestUpSupernode, -1, resultStream);
  vtkm::worklet::contourtree_augmented::PrintIndices(
    "Best Up Volume by SN", this->BestUpVolume, -1, resultStream);
  std::cout << std::endl;
  return resultStream.str();
} // DebugPrint()

} // namespace scalar_topology
} // namespace filter
} // namespace vtkm


#endif
