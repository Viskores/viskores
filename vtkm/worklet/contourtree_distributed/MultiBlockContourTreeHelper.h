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

#ifndef vtk_m_worklet_contourtree_distributed_multiblockcontourtreehelper_h
#define vtk_m_worklet_contourtree_distributed_multiblockcontourtreehelper_h

#include <vtkm/worklet/contourtree_distributed/SpatialDecomposition.h>

#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/ContourTreeMesh.h>

#include <vtkm/Types.h>
#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/BoundsGlobalCompute.h>
//#include <vtkm/cont/AssignerPartitionedDataSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/IdRelabeler.h>


namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{

//--- Helper class to help with the contstuction of the GlobalContourTree
class MultiBlockContourTreeHelper
{
public:
  VTKM_CONT
  MultiBlockContourTreeHelper(vtkm::Id3 blocksPerDim,
                              vtkm::Id3 globalSize,
                              const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockIndices,
                              const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockOrigins,
                              const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockSizes)
    : MultiBlockSpatialDecomposition(blocksPerDim,
                                     globalSize,
                                     localBlockIndices,
                                     localBlockOrigins,
                                     localBlockSizes)
  {
    vtkm::Id localNumBlocks = this->GetLocalNumberOfBlocks();
    LocalContourTrees.resize(static_cast<std::size_t>(localNumBlocks));
    LocalSortOrders.resize(static_cast<std::size_t>(localNumBlocks));
  }

  VTKM_CONT
  ~MultiBlockContourTreeHelper(void)
  {
    LocalContourTrees.clear();
    LocalSortOrders.clear();
  }

  inline static vtkm::Bounds GetGlobalBounds(const vtkm::cont::PartitionedDataSet& input)
  {
    // Get the  spatial bounds  of a multi -block  data  set
    vtkm::Bounds bounds = vtkm::cont::BoundsGlobalCompute(input);
    return bounds;
  }

  inline static vtkm::Bounds GetLocalBounds(const vtkm::cont::PartitionedDataSet& input)
  {
    // Get the spatial bounds  of a multi -block  data  set
    vtkm::Bounds bounds = vtkm::cont::BoundsCompute(input);
    return bounds;
  }

  inline vtkm::Id GetLocalNumberOfBlocks() const
  {
    return this->MultiBlockSpatialDecomposition.GetLocalNumberOfBlocks();
  }

  inline vtkm::Id GetGlobalNumberOfBlocks() const
  {
    return this->MultiBlockSpatialDecomposition.GetGlobalNumberOfBlocks();
  }

  inline static vtkm::Id GetGlobalNumberOfBlocks(const vtkm::cont::PartitionedDataSet& input)
  {
    auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    vtkm::Id localSize = input.GetNumberOfPartitions();
    vtkm::Id globalSize = 0;
#ifdef VTKM_ENABLE_MPI
    vtkmdiy::mpi::all_reduce(comm, localSize, globalSize, std::plus<vtkm::Id>{});
#else
    globalSize = localSize;
#endif
    return globalSize;
  }

  // Used to compute the local contour tree mesh in after DoExecute. I.e., the function is
  // used in PostExecute to construct the initial set of local ContourTreeMesh blocks for
  // DIY. Subsequent construction of updated ContourTreeMeshes is handled separately.
  template <typename T>
  inline static vtkm::worklet::contourtree_augmented::ContourTreeMesh<T>*
  ComputeLocalContourTreeMesh(const vtkm::Id3 localBlockOrigin,
                              const vtkm::Id3 localBlockSize,
                              const vtkm::Id3 globalSize,
                              const vtkm::cont::ArrayHandle<T>& field,
                              const vtkm::worklet::contourtree_augmented::ContourTree& contourTree,
                              const vtkm::worklet::contourtree_augmented::IdArrayType& sortOrder,
                              unsigned int computeRegularStructure)

  {
    vtkm::Id startRow = localBlockOrigin[0];
    vtkm::Id startCol = localBlockOrigin[1];
    vtkm::Id startSlice = localBlockOrigin[2];
    vtkm::Id numRows = localBlockSize[0];
    vtkm::Id numCols = localBlockSize[1];
    vtkm::Id totalNumRows = globalSize[0];
    vtkm::Id totalNumCols = globalSize[1];
    // compute the global mesh index and initalize the local contour tree mesh
    if (computeRegularStructure == 1)
    {
      // Compute the global mesh index
      vtkm::worklet::contourtree_augmented::IdArrayType localGlobalMeshIndex;
      auto transformedIndex = vtkm::cont::ArrayHandleTransform<
        vtkm::worklet::contourtree_augmented::IdArrayType,
        vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler>(
        sortOrder,
        vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler(
          startRow, startCol, startSlice, numRows, numCols, totalNumRows, totalNumCols));
      vtkm::cont::Algorithm::Copy(transformedIndex, localGlobalMeshIndex);
      // Compute the local contour tree mesh
      auto localContourTreeMesh = new vtkm::worklet::contourtree_augmented::ContourTreeMesh<T>(
        contourTree.Arcs, sortOrder, field, localGlobalMeshIndex);
      return localContourTreeMesh;
    }
    else if (computeRegularStructure == 2)
    {
      // Compute the global mesh index for the partially augmented contour tree. I.e., here we
      // don't need the global mesh index for all nodes, but only for the augmented nodes from the
      // tree. We, hence, permute the sortOrder by contourTree.augmentednodes and then compute the
      // GlobalMeshIndex by tranforming those indices with our IdRelabler
      vtkm::worklet::contourtree_augmented::IdArrayType localGlobalMeshIndex;
      vtkm::cont::ArrayHandlePermutation<vtkm::worklet::contourtree_augmented::IdArrayType,
                                         vtkm::worklet::contourtree_augmented::IdArrayType>
        permutedSortOrder(contourTree.Augmentnodes, sortOrder);
      auto transformedIndex = vtkm::cont::make_ArrayHandleTransform(
        permutedSortOrder,
        vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler(
          startRow, startCol, startSlice, numRows, numCols, totalNumRows, totalNumCols));
      vtkm::cont::Algorithm::Copy(transformedIndex, localGlobalMeshIndex);
      // Compute the local contour tree mesh
      auto localContourTreeMesh = new vtkm::worklet::contourtree_augmented::ContourTreeMesh<T>(
        contourTree.Augmentnodes, contourTree.Augmentarcs, sortOrder, field, localGlobalMeshIndex);
      return localContourTreeMesh;
    }
    else
    {
      // We should not be able to get here
      throw vtkm::cont::ErrorFilterExecution(
        "Parallel contour tree requires at least parial boundary augmentation");
    }
  }

  SpatialDecomposition MultiBlockSpatialDecomposition;
  std::vector<vtkm::worklet::contourtree_augmented::ContourTree> LocalContourTrees;
  std::vector<vtkm::worklet::contourtree_augmented::IdArrayType> LocalSortOrders;

}; // end MultiBlockContourTreeHelper

} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
