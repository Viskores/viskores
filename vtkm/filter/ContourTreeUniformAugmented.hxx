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

#ifndef vtk_m_filter_ContourTreeUniformAugmented_hxx
#define vtk_m_filter_ContourTreeUniformAugmented_hxx

#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/filter/ContourTreeUniformAugmented.h>

#include <vtkm/worklet/ContourTreeUniformAugmented.h>
#include <vtkm/worklet/contourtree_augmented/ProcessContourTree.h>

#include <vtkm/worklet/contourtree_augmented/Mesh_DEM_Triangulation.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/ContourTreeMesh.h>

#include <vtkm/cont/ArrayCopy.h>
//#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/AssignerPartitionedDataSet.h>
#include <vtkm/cont/BoundsCompute.h>
#include <vtkm/cont/BoundsGlobalCompute.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem/IdRelabler.h>
// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include <vtkm/thirdparty/diy/diy.h>
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on


namespace vtkm
{
namespace filter
{
namespace detail
{
template <typename FieldType>
struct ContourTreeBlockData
{
  static void* create() { return new ContourTreeBlockData<FieldType>; }
  static void destroy(void* b) { delete static_cast<ContourTreeBlockData<FieldType>*>(b); }

  // ContourTreeMesh data
  vtkm::Id NumVertices;
  // TODO Should be able to remove sortOrder here, but we need to figure out what to return in the worklet instead
  vtkm::worklet::contourtree_augmented::IdArrayType SortOrder;
  vtkm::cont::ArrayHandle<FieldType> SortedValue;
  vtkm::worklet::contourtree_augmented::IdArrayType GlobalMeshIndex;
  vtkm::worklet::contourtree_augmented::IdArrayType Neighbours;
  vtkm::worklet::contourtree_augmented::IdArrayType FirstNeighbour;
  vtkm::Id MaxNeighbours;

  // Block metadata
  vtkm::Id3 BlockOrigin;                // Origin of the data block
  vtkm::Id3 BlockSize;                  // Extends of the data block
  vtkm::Id3 GlobalSize;                 // Extends of the global mesh
  unsigned int ComputeRegularStructure; // pass through augmentation setting
};
} // namespace detail
} // namespace filter
} // namespace vtkm



namespace vtkmdiy
{

// Struct to serialize ContourBlockData objects (i.e., load/save) needed in parralle for DIY
template <typename FieldType>
struct Serialization<vtkm::filter::detail::ContourTreeBlockData<FieldType>>
{
  static void save(vtkmdiy::BinaryBuffer& bb,
                   const vtkm::filter::detail::ContourTreeBlockData<FieldType>& block)
  {
    vtkmdiy::save(bb, block.NumVertices);
    vtkmdiy::save(bb, block.SortOrder);
    vtkmdiy::save(bb, block.SortedValue);
    vtkmdiy::save(bb, block.GlobalMeshIndex);
    vtkmdiy::save(bb, block.Neighbours);
    vtkmdiy::save(bb, block.FirstNeighbour);
    vtkmdiy::save(bb, block.MaxNeighbours);
    vtkmdiy::save(bb, block.BlockOrigin);
    vtkmdiy::save(bb, block.BlockSize);
    vtkmdiy::save(bb, block.GlobalSize);
    vtkmdiy::save(bb, block.ComputeRegularStructure);
  }

  static void load(vtkmdiy::BinaryBuffer& bb,
                   vtkm::filter::detail::ContourTreeBlockData<FieldType>& block)
  {
    vtkmdiy::load(bb, block.NumVertices);
    vtkmdiy::load(bb, block.SortOrder);
    vtkmdiy::load(bb, block.SortedValue);
    vtkmdiy::load(bb, block.GlobalMeshIndex);
    vtkmdiy::load(bb, block.Neighbours);
    vtkmdiy::load(bb, block.FirstNeighbour);
    vtkmdiy::load(bb, block.MaxNeighbours);
    vtkmdiy::load(bb, block.BlockOrigin);
    vtkmdiy::load(bb, block.BlockSize);
    vtkmdiy::load(bb, block.GlobalSize);
    vtkmdiy::load(bb, block.ComputeRegularStructure);
  }
};

} // namespace mangled_vtkmdiy_namespace


namespace vtkm
{
namespace filter
{
namespace detail
{
//----Helper struct to call DoPostExecute. This is used to be able to
//    wrap the PostExecute work in a functor so that we can use VTK-M's
//    vtkm::cont::CastAndCall to infer the FieldType template parameters
struct PostExecuteCaller
{
  template <typename T, typename S, typename DerivedPolicy>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>&,
                            ContourTreeAugmented* self,
                            const vtkm::cont::PartitionedDataSet& input,
                            vtkm::cont::PartitionedDataSet& output,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy) const
  {
    vtkm::cont::ArrayHandle<T, S> dummy;
    self->DoPostExecute(input, output, fieldMeta, dummy, policy);
  }
};


// --- Helper class to store the spatial decomposition defined by the PartitionedDataSet input data
class SpatialDecomposition
{
public:
  VTKM_CONT
  SpatialDecomposition(vtkm::Id3 blocksPerDim,
                       vtkm::Id3 globalSize,
                       const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockIndices,
                       const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockOrigins,
                       const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockSizes)
    : BlocksPerDimension(blocksPerDim)
    , GlobalSize(globalSize)
    , LocalBlockIndices(localBlockIndices)
    , LocalBlockOrigins(localBlockOrigins)
    , LocalBlockSizes(localBlockSizes)
  {
  }

  inline vtkmdiy::DiscreteBounds GetVTKmDIYBounds() const
  {
    if (this->NumberOfDimensions() == 2)
    {
      // may need to change back when porting ot later verison of VTKM/vtkmdiy
      vtkmdiy::DiscreteBounds domain; //(2);
      domain.min[0] = domain.min[1] = 0;
      domain.max[0] = static_cast<int>(this->GlobalSize[0]);
      domain.max[1] = static_cast<int>(this->GlobalSize[1]);
      return domain;
    }
    else
    {
      // may need to change back when porting to later version of VTMK/vtkmdiy
      vtkmdiy::DiscreteBounds domain; //(3);
      domain.min[0] = domain.min[1] = domain.min[2] = 0;
      domain.max[0] = static_cast<int>(this->GlobalSize[0]);
      domain.max[1] = static_cast<int>(this->GlobalSize[1]);
      domain.max[2] = static_cast<int>(this->GlobalSize[2]);
      return domain;
    }
  }

  inline vtkm::Id NumberOfDimensions() const { return GlobalSize[2] > 1 ? 3 : 2; }

  inline vtkm::Id GetGlobalNumberOfBlocks() const
  {
    return BlocksPerDimension[0] * BlocksPerDimension[1] * BlocksPerDimension[2];
  }

  inline vtkm::Id GetLocalNumberOfBlocks() const { return LocalBlockSizes.GetNumberOfValues(); }

  // Number of blocks along each dimension
  vtkm::Id3 BlocksPerDimension;
  // Size of the global mesh
  vtkm::Id3 GlobalSize;
  // Index of the local blocks in x,y,z, i.e., in i,j,k mesh coordinates
  vtkm::cont::ArrayHandle<vtkm::Id3> LocalBlockIndices;
  // Origin of the local blocks in mesh index space
  vtkm::cont::ArrayHandle<vtkm::Id3> LocalBlockOrigins;
  // Size of each local block in x, y,z
  vtkm::cont::ArrayHandle<vtkm::Id3> LocalBlockSizes;
};


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
        vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabler>(
        sortOrder,
        vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabler(
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
        vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabler(
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

// Functor needed so we can discover the FieldType and DeviceAdapter template parameters to call MergeWith
struct MergeFunctor
{
  template <typename DeviceAdapterTag, typename FieldType>
  bool operator()(DeviceAdapterTag,
                  vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType>& in,
                  vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType>& out) const
  {
    out.template MergeWith<DeviceAdapterTag>(in);
    return true;
  }
};

// Functor used by DIY reduce the merge data blocks in parallel
template <typename FieldType>
void MergeBlockFunctor(
  ContourTreeBlockData<FieldType>* block,       // local Block.
  const vtkmdiy::ReduceProxy& rp,               // communication proxy
  const vtkmdiy::RegularMergePartners& partners // partners of the current block
  )
{                 //MergeBlockFunctor
  (void)partners; // Avoid unused parameter warning

  const auto selfid = rp.gid();

  // TODO This should be changed so that we have the ContourTree itself as the block and then the
  //      ContourTreeMesh would still be used for exchange. In this case we would need to compute
  //      the ContourTreeMesh at the beginning of the function for the current block every time
  //      but then we would not need to compute those meshes when we initialize vtkmdiy
  //      and we don't need to have the special case for rank 0.

  // Here we do the deque first before the send due to the way the iteration is handled in DIY, i.e., in each iteration
  // A block needs to first collect the data from its neighours and then send the combined block to its neighbours
  // for the next iteration.
  // 1. dequeue the block and compute the new contour tree and contour tree mesh for the block if we have the hight GID
  std::vector<int> incoming;
  rp.incoming(incoming);
  for (const int ingid : incoming)
  {
    if (ingid != selfid)
    {
      ContourTreeBlockData<FieldType> recvblock;
      rp.dequeue(ingid, recvblock);

      // Construct the two contour tree mesh by assignign the block data
      vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType> contourTreeMeshIn;
      contourTreeMeshIn.NumVertices = recvblock.NumVertices;
      contourTreeMeshIn.SortOrder = recvblock.SortOrder;
      contourTreeMeshIn.SortedValues = recvblock.SortedValue;
      contourTreeMeshIn.GlobalMeshIndex = recvblock.GlobalMeshIndex;
      contourTreeMeshIn.Neighbours = recvblock.Neighbours;
      contourTreeMeshIn.FirstNeighbour = recvblock.FirstNeighbour;
      contourTreeMeshIn.MaxNeighbours = recvblock.MaxNeighbours;

      vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType> contourTreeMeshOut;
      contourTreeMeshOut.NumVertices = block->NumVertices;
      contourTreeMeshOut.SortOrder = block->SortOrder;
      contourTreeMeshOut.SortedValues = block->SortedValue;
      contourTreeMeshOut.GlobalMeshIndex = block->GlobalMeshIndex;
      contourTreeMeshOut.Neighbours = block->Neighbours;
      contourTreeMeshOut.FirstNeighbour = block->FirstNeighbour;
      contourTreeMeshOut.MaxNeighbours = block->MaxNeighbours;
      // Merge the two contour tree meshes
      vtkm::cont::TryExecute(MergeFunctor{}, contourTreeMeshIn, contourTreeMeshOut);

      // Compute the origin and size of the new block
      vtkm::Id3 globalSize = block->GlobalSize;
      vtkm::Id3 currBlockOrigin;
      currBlockOrigin[0] = std::min(recvblock.BlockOrigin[0], block->BlockOrigin[0]);
      currBlockOrigin[1] = std::min(recvblock.BlockOrigin[1], block->BlockOrigin[1]);
      currBlockOrigin[2] = std::min(recvblock.BlockOrigin[2], block->BlockOrigin[2]);
      vtkm::Id3 currBlockMaxIndex; // Needed only to compute the block size
      currBlockMaxIndex[0] = std::max(recvblock.BlockOrigin[0] + recvblock.BlockSize[0],
                                      block->BlockOrigin[0] + block->BlockSize[0]);
      currBlockMaxIndex[1] = std::max(recvblock.BlockOrigin[1] + recvblock.BlockSize[1],
                                      block->BlockOrigin[1] + block->BlockSize[1]);
      currBlockMaxIndex[2] = std::max(recvblock.BlockOrigin[2] + recvblock.BlockSize[2],
                                      block->BlockOrigin[2] + block->BlockSize[2]);
      vtkm::Id3 currBlockSize;
      currBlockSize[0] = currBlockMaxIndex[0] - currBlockOrigin[0];
      currBlockSize[1] = currBlockMaxIndex[1] - currBlockOrigin[1];
      currBlockSize[2] = currBlockMaxIndex[2] - currBlockOrigin[2];

      // On rank 0 we compute the contour tree at the end when the merge is done, so we don't need to do it here
      if (selfid == 0)
      {
        // Save the data from our block for the next iteration
        block->NumVertices = contourTreeMeshOut.NumVertices;
        block->SortOrder = contourTreeMeshOut.SortOrder;
        block->SortedValue = contourTreeMeshOut.SortedValues;
        block->GlobalMeshIndex = contourTreeMeshOut.GlobalMeshIndex;
        block->Neighbours = contourTreeMeshOut.Neighbours;
        block->FirstNeighbour = contourTreeMeshOut.FirstNeighbour;
        block->MaxNeighbours = contourTreeMeshOut.MaxNeighbours;
        block->BlockOrigin = currBlockOrigin;
        block->BlockSize = currBlockSize;
        block->GlobalSize = globalSize;
      }
      else // If we are a block that will continue to be merged then we need compute the contour tree here
      {
        // Compute the contour tree from our merged mesh
        vtkm::Id currNumIterations;
        vtkm::worklet::contourtree_augmented::ContourTree currContourTree;
        vtkm::worklet::contourtree_augmented::IdArrayType currSortOrder;
        vtkm::worklet::ContourTreeAugmented worklet;
        vtkm::cont::ArrayHandle<FieldType> currField;
        vtkm::Id3 maxIdx(currBlockOrigin[0] + currBlockSize[0] - 1,
                         currBlockOrigin[1] + currBlockSize[1] - 1,
                         currBlockOrigin[2] + currBlockSize[2] - 1);
        auto meshBoundaryExecObj =
          contourTreeMeshOut.GetMeshBoundaryExecutionObject(globalSize[0],   // totalNRows
                                                            globalSize[1],   // totalNCols
                                                            currBlockOrigin, // minIdx
                                                            maxIdx           // maxIdx
                                                            );
        worklet.Run(
          contourTreeMeshOut.SortedValues, // Unused param. Provide something to keep the API happy
          contourTreeMeshOut,
          currContourTree,
          currSortOrder,
          currNumIterations,
          block->ComputeRegularStructure,
          meshBoundaryExecObj);
        vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType>* newContourTreeMesh = 0;
        if (block->ComputeRegularStructure == 1)
        {
          // If we have the fully augmented contour tree
          newContourTreeMesh = new vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType>(
            currContourTree.Arcs, contourTreeMeshOut);
        }
        else if (block->ComputeRegularStructure == 2)
        {
          // If we have the partially augmented (e.g., boundary augmented) contour tree
          newContourTreeMesh = new vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType>(
            currContourTree.Augmentnodes, currContourTree.Augmentarcs, contourTreeMeshOut);
        }
        else
        {
          // We should not be able to get here
          throw vtkm::cont::ErrorFilterExecution(
            "Parallel contour tree requires at least parial boundary augmentation");
        }

        // Copy the data from newContourTreeMesh into  block
        block->NumVertices = newContourTreeMesh->NumVertices;
        block->SortOrder = newContourTreeMesh->SortOrder;
        block->SortedValue = newContourTreeMesh->SortedValues;
        block->GlobalMeshIndex = newContourTreeMesh->GlobalMeshIndex;
        block->Neighbours = newContourTreeMesh->Neighbours;
        block->FirstNeighbour = newContourTreeMesh->FirstNeighbour;
        block->MaxNeighbours = newContourTreeMesh->MaxNeighbours;
        block->BlockOrigin = currBlockOrigin;
        block->BlockSize = currBlockSize;
        block->GlobalSize = globalSize;

        // VTKm keeps track of the arrays for us, so we can savely delete the ContourTreeMesh
        // as all data has been transferred into our data block
        delete newContourTreeMesh;
      }
    }
  }
  // Send our current block (which is either our original block or the one we just combined from the ones we received) to our next neighbour.
  // Once a rank has send his block (either in its orignal or merged form) it is done with the reduce
  for (int cc = 0; cc < rp.out_link().size(); ++cc)
  {
    auto target = rp.out_link().target(cc);
    if (target.gid != selfid)
    {
      rp.enqueue(target, *block);
    }
  }
} //end MergeBlockFunctor

} // end namespace detail



//-----------------------------------------------------------------------------
ContourTreeAugmented::ContourTreeAugmented(bool useMarchingCubes,
                                           unsigned int computeRegularStructure)
  : vtkm::filter::FilterCell<ContourTreeAugmented>()
  , UseMarchingCubes(useMarchingCubes)
  , ComputeRegularStructure(computeRegularStructure)
  , MultiBlockTreeHelper(nullptr)
{
  this->SetOutputFieldName("resultData");
}

void ContourTreeAugmented::SetSpatialDecomposition(
  vtkm::Id3 blocksPerDim,
  vtkm::Id3 globalSize,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockIndices,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockOrigins,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockSizes)
{
  if (this->MultiBlockTreeHelper)
  {
    delete this->MultiBlockTreeHelper;
    this->MultiBlockTreeHelper = nullptr;
  }
  this->MultiBlockTreeHelper = new detail::MultiBlockContourTreeHelper(
    blocksPerDim, globalSize, localBlockIndices, localBlockOrigins, localBlockSizes);
}

const vtkm::worklet::contourtree_augmented::ContourTree& ContourTreeAugmented::GetContourTree()
  const
{
  return this->ContourTreeData;
}

const vtkm::worklet::contourtree_augmented::IdArrayType& ContourTreeAugmented::GetSortOrder() const
{
  return this->MeshSortOrder;
}

vtkm::Id ContourTreeAugmented::GetNumIterations() const
{
  return this->NumIterations;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
vtkm::cont::DataSet ContourTreeAugmented::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  vtkm::cont::Timer timer;
  timer.Start();

  // Check that the field is Ok
  if (fieldMeta.IsPointField() == false)
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  // Use the GetRowsColsSlices struct defined in the header to collect the nRows, nCols, and nSlices information
  vtkm::worklet::ContourTreeAugmented worklet;
  vtkm::Id nRows;
  vtkm::Id nCols;
  vtkm::Id nSlices = 1;
  const auto& cells = input.GetCellSet();
  vtkm::filter::ApplyPolicyCellSet(cells, policy)
    .CastAndCall(GetRowsColsSlices(), nRows, nCols, nSlices);
  // TODO blockIndex needs to change if we have multiple blocks per MPI rank and DoExecute is called for multiple blocks
  std::size_t blockIndex = 0;

  // Determine if and what augmentation we need to do
  unsigned int compRegularStruct = this->ComputeRegularStructure;
  // When running in parallel we need to at least augment with the boundary vertices
  if (compRegularStruct == 0)
  {
    if (this->MultiBlockTreeHelper)
    {
      if (this->MultiBlockTreeHelper->GetGlobalNumberOfBlocks() > 1)
      {
        compRegularStruct = 2; // Compute boundary augmentation
      }
    }
  }

  // Run the worklet
  worklet.Run(field,
              MultiBlockTreeHelper ? MultiBlockTreeHelper->LocalContourTrees[blockIndex]
                                   : this->ContourTreeData,
              MultiBlockTreeHelper ? MultiBlockTreeHelper->LocalSortOrders[blockIndex]
                                   : this->MeshSortOrder,
              this->NumIterations,
              nRows,
              nCols,
              nSlices,
              this->UseMarchingCubes,
              compRegularStruct);

  // If we run in parallel but with only one global block, then we need set our outputs correctly
  // here to match the expected behavior in parallel
  if (this->MultiBlockTreeHelper)
  {
    if (this->MultiBlockTreeHelper->GetGlobalNumberOfBlocks() == 1)
    {
      // Copy the contour tree and mesh sort order to the output
      this->ContourTreeData = this->MultiBlockTreeHelper->LocalContourTrees[0];
      this->MeshSortOrder = this->MultiBlockTreeHelper->LocalSortOrders[0];
      // In parallel we need the sorted values as output resulti
      // Construct the sorted values by permutting the input field
      auto fieldPermutted = vtkm::cont::make_ArrayHandlePermutation(this->MeshSortOrder, field);
      vtkm::cont::ArrayHandle<T> sortedValues;
      vtkm::cont::Algorithm::Copy(fieldPermutted, sortedValues);
      // Create the result object
      vtkm::cont::DataSet result;
      vtkm::cont::Field rfield(
        this->GetOutputFieldName(), vtkm::cont::Field::Association::WHOLE_MESH, sortedValues);
      result.AddField(rfield);
      return result;
    }
  }

  VTKM_LOG_S(vtkm::cont::LogLevel::Info,
             std::endl
               << "    "
               << std::setw(38)
               << std::left
               << "Contour Tree Filter DoExecute"
               << ": "
               << timer.GetElapsedTime()
               << " seconds");

  // Construct the expected result for serial execution. Note, in serial the result currently
  // not actually being used, but in parallel we need the sorted mesh values as output
  // This part is being hit when we run in serial or parallel with more then one rank
  return CreateResultFieldPoint(input, ContourTreeData.Arcs, this->GetOutputFieldName());
} // ContourTreeAugmented::DoExecute


//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT void ContourTreeAugmented::PreExecute(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  //if( input.GetNumberOfBlocks() != 1){
  //  throw vtkm::cont::ErrorBadValue("Expected MultiBlock data with 1 block per rank ");
  //}
  if (this->MultiBlockTreeHelper)
  {
    if (this->MultiBlockTreeHelper->GetGlobalNumberOfBlocks(input) !=
        this->MultiBlockTreeHelper->GetGlobalNumberOfBlocks())
    {
      throw vtkm::cont::ErrorFilterExecution(
        "Global number of block in MultiBlock dataset does not match the SpatialDecomposition");
    }
    if (this->MultiBlockTreeHelper->GetLocalNumberOfBlocks() != input.GetNumberOfPartitions())
    {
      throw vtkm::cont::ErrorFilterExecution(
        "Global number of block in MultiBlock dataset does not match the SpatialDecomposition");
    }
  } //else
  //{
  //  throw vtkm::cont::ErrorFilterExecution("Spatial decomposition not defined for MultiBlock execution. Use ContourTreeAugmented::SetSpatialDecompoistion to define the domain decomposition.");
  //}
}


//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
VTKM_CONT void ContourTreeAugmented::DoPostExecute(
  const vtkm::cont::PartitionedDataSet& input,
  vtkm::cont::PartitionedDataSet& output,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::cont::ArrayHandle<T, StorageType>&, // dummy parameter to get the type
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  (void)fieldMeta; // avoid unused parameter warning
  (void)policy;    // avoid unused parameter warning

  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id size = comm.size();
  vtkm::Id rank = comm.rank();

  std::vector<vtkm::worklet::contourtree_augmented::ContourTreeMesh<T>*> localContourTreeMeshes;
  localContourTreeMeshes.resize(static_cast<std::size_t>(input.GetNumberOfPartitions()));
  // TODO need to allocate and free these ourselves. May need to update detail::MultiBlockContourTreeHelper::ComputeLocalContourTreeMesh
  std::vector<vtkm::filter::detail::ContourTreeBlockData<T>*> localDataBlocks;
  localDataBlocks.resize(static_cast<size_t>(input.GetNumberOfPartitions()));
  std::vector<vtkmdiy::Link*> localLinks; // dummy links needed to make DIY happy
  localLinks.resize(static_cast<size_t>(input.GetNumberOfPartitions()));
  // We need to augment at least with the boundary vertices when running in parallel, even if the user requested at the end only the unaugmented contour tree
  unsigned int compRegularStruct =
    (this->ComputeRegularStructure > 0) ? this->ComputeRegularStructure : 2;
  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    // create the local contour tree mesh
    localLinks[bi] = new vtkmdiy::Link;
    auto currBlock = input.GetPartition(static_cast<vtkm::Id>(bi));
    auto currField =
      currBlock.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    //const vtkm::cont::ArrayHandle<T,StorageType> &fieldData = currField.GetData().Cast<vtkm::cont::ArrayHandle<T,StorageType> >();
    vtkm::cont::ArrayHandle<T> fieldData;
    vtkm::cont::ArrayCopy(currField.GetData().AsVirtual<T>(), fieldData);
    auto currContourTreeMesh =
      vtkm::filter::detail::MultiBlockContourTreeHelper::ComputeLocalContourTreeMesh<T>(
        this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.LocalBlockOrigins.ReadPortal()
          .Get(static_cast<vtkm::Id>(bi)),
        this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.LocalBlockSizes.ReadPortal().Get(
          static_cast<vtkm::Id>(bi)),
        this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.GlobalSize,
        fieldData,
        MultiBlockTreeHelper->LocalContourTrees[bi],
        MultiBlockTreeHelper->LocalSortOrders[bi],
        compRegularStruct);
    localContourTreeMeshes[bi] = currContourTreeMesh;
    // create the local data block structure
    localDataBlocks[bi] = new vtkm::filter::detail::ContourTreeBlockData<T>();
    localDataBlocks[bi]->NumVertices = currContourTreeMesh->NumVertices;
    localDataBlocks[bi]->SortOrder = currContourTreeMesh->SortOrder;
    localDataBlocks[bi]->SortedValue = currContourTreeMesh->SortedValues;
    localDataBlocks[bi]->GlobalMeshIndex = currContourTreeMesh->GlobalMeshIndex;
    localDataBlocks[bi]->Neighbours = currContourTreeMesh->Neighbours;
    localDataBlocks[bi]->FirstNeighbour = currContourTreeMesh->FirstNeighbour;
    localDataBlocks[bi]->MaxNeighbours = currContourTreeMesh->MaxNeighbours;
    localDataBlocks[bi]->BlockOrigin =
      this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.LocalBlockOrigins.ReadPortal().Get(
        static_cast<vtkm::Id>(bi));
    localDataBlocks[bi]->BlockSize =
      this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.LocalBlockSizes.ReadPortal().Get(
        static_cast<vtkm::Id>(bi));
    localDataBlocks[bi]->GlobalSize =
      this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.GlobalSize;
    // We need to augment at least with the boundary vertices when running in parallel
    localDataBlocks[bi]->ComputeRegularStructure = compRegularStruct;
  }
  // Setup vtkmdiy to do global binary reduction of neighbouring blocks. See also RecuctionOperation struct for example

  // Create the vtkmdiy master
  vtkmdiy::Master master(comm,
                         1, // Use 1 thread, VTK-M will do the treading
                         -1 // All block in memory
                         );

  // Compute the gids for our local blocks
  const detail::SpatialDecomposition& spatialDecomp =
    this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition;
  auto localBlockIndicesPortal = spatialDecomp.LocalBlockIndices.ReadPortal();
  std::vector<vtkm::Id> vtkmdiyLocalBlockGids(static_cast<size_t>(input.GetNumberOfPartitions()));
  for (vtkm::Id bi = 0; bi < input.GetNumberOfPartitions(); bi++)
  {
    std::vector<int> tempCoords;    // DivisionsVector type in DIY
    std::vector<int> tempDivisions; // DivisionsVector type in DIY
    tempCoords.resize(static_cast<size_t>(spatialDecomp.NumberOfDimensions()));
    tempDivisions.resize(static_cast<size_t>(spatialDecomp.NumberOfDimensions()));
    auto currentCoords = localBlockIndicesPortal.Get(bi);
    for (std::size_t di = 0; di < static_cast<size_t>(spatialDecomp.NumberOfDimensions()); di++)
    {
      tempCoords[di] = static_cast<int>(currentCoords[static_cast<vtkm::IdComponent>(di)]);
      tempDivisions[di] =
        static_cast<int>(spatialDecomp.BlocksPerDimension[static_cast<vtkm::IdComponent>(di)]);
    }
    vtkmdiyLocalBlockGids[static_cast<size_t>(bi)] =
      vtkmdiy::RegularDecomposer<vtkmdiy::DiscreteBounds>::coords_to_gid(tempCoords, tempDivisions);
  }

  // Add my local blocks to the vtkmdiy master.
  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    master.add(static_cast<int>(vtkmdiyLocalBlockGids[bi]), // block id
               localDataBlocks[bi],
               localLinks[bi]);
  }

  // Define the decomposition of the domain into regular blocks
  vtkmdiy::RegularDecomposer<vtkmdiy::DiscreteBounds> decomposer(
    static_cast<int>(spatialDecomp.NumberOfDimensions()), // number of dims
    spatialDecomp.GetVTKmDIYBounds(),
    static_cast<int>(spatialDecomp.GetGlobalNumberOfBlocks()));

  // Define which blocks live on which rank so that vtkmdiy can manage them
  vtkmdiy::DynamicAssigner assigner(
    comm, static_cast<int>(size), static_cast<int>(spatialDecomp.GetGlobalNumberOfBlocks()));
  for (vtkm::Id bi = 0; bi < input.GetNumberOfPartitions(); bi++)
  {
    assigner.set_rank(static_cast<int>(rank),
                      static_cast<int>(vtkmdiyLocalBlockGids[static_cast<size_t>(bi)]));
  }

  // Fix the vtkmdiy links. TODO Remove changes to the vtkmdiy in VTKM when we update to the new VTK-M
  vtkmdiy::fix_links(master, assigner);

  // partners for merge over regular block grid
  vtkmdiy::RegularMergePartners partners(
    decomposer, // domain decomposition
    2,          // raix of k-ary reduction. TODO check this value
    true        // contiguous: true=distance doubling , false=distnace halving TODO check this value
    );
  // reduction
  vtkmdiy::reduce(master, assigner, partners, &detail::MergeBlockFunctor<T>);

  comm.barrier(); // Be safe!

  if (rank == 0)
  {
    // Now run the contour tree algorithm on the last block to compute the final tree
    vtkm::Id currNumIterations;
    vtkm::worklet::contourtree_augmented::ContourTree currContourTree;
    vtkm::worklet::contourtree_augmented::IdArrayType currSortOrder;
    vtkm::worklet::ContourTreeAugmented worklet;
    vtkm::cont::ArrayHandle<T> currField;
    // Construct the contour tree mesh from the last block
    vtkm::worklet::contourtree_augmented::ContourTreeMesh<T> contourTreeMeshOut;
    contourTreeMeshOut.NumVertices = localDataBlocks[0]->NumVertices;
    contourTreeMeshOut.SortOrder = localDataBlocks[0]->SortOrder;
    contourTreeMeshOut.SortedValues = localDataBlocks[0]->SortedValue;
    contourTreeMeshOut.GlobalMeshIndex = localDataBlocks[0]->GlobalMeshIndex;
    contourTreeMeshOut.Neighbours = localDataBlocks[0]->Neighbours;
    contourTreeMeshOut.FirstNeighbour = localDataBlocks[0]->FirstNeighbour;
    contourTreeMeshOut.MaxNeighbours = localDataBlocks[0]->MaxNeighbours;
    // Construct the mesh boundary exectuion object needed for boundary augmentation
    vtkm::Id3 minIdx(0, 0, 0);
    vtkm::Id3 maxIdx = this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.GlobalSize;
    maxIdx[0] = maxIdx[0] - 1;
    maxIdx[1] = maxIdx[1] - 1;
    maxIdx[2] = maxIdx[2] > 0 ? (maxIdx[2] - 1) : 0;
    auto meshBoundaryExecObj = contourTreeMeshOut.GetMeshBoundaryExecutionObject(
      this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.GlobalSize[0],
      this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.GlobalSize[1],
      minIdx,
      maxIdx);
    // Run the worklet to compute the final contour tree
    worklet.Run(
      contourTreeMeshOut.SortedValues, // Unused param. Provide something to keep API happy
      contourTreeMeshOut,
      this->ContourTreeData,
      this->MeshSortOrder,
      currNumIterations,
      this->ComputeRegularStructure,
      meshBoundaryExecObj);

    // Set the final mesh sort order we need to use
    this->MeshSortOrder = contourTreeMeshOut.GlobalMeshIndex;
    // Remeber the number of iterations for the output
    this->NumIterations = currNumIterations;

    // Return the sorted values of the contour tree as the result
    // TODO the result we return for the parallel and serial case are different right now. This should be made consistent. However, only in the parallel case are we useing the result output
    vtkm::cont::DataSet temp;
    vtkm::cont::Field rfield(this->GetOutputFieldName(),
                             vtkm::cont::Field::Association::WHOLE_MESH,
                             contourTreeMeshOut.SortedValues);
    temp.AddField(rfield);
    output = vtkm::cont::PartitionedDataSet(temp);
  }
  else
  {
    this->ContourTreeData = MultiBlockTreeHelper->LocalContourTrees[0];
    this->MeshSortOrder = MultiBlockTreeHelper->LocalSortOrders[0];

    // Free allocated temporary pointers
    for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
    {
      delete localContourTreeMeshes[bi];
      delete localDataBlocks[bi];
      // delete localLinks[bi];
    }
  }
  localContourTreeMeshes.clear();
  localDataBlocks.clear();
  localLinks.clear();
}


//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT void ContourTreeAugmented::PostExecute(
  const vtkm::cont::PartitionedDataSet& input,
  vtkm::cont::PartitionedDataSet& result,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (this->MultiBlockTreeHelper)
  {
    vtkm::cont::Timer timer;
    timer.Start();
    // We are running in parallel and need to merge the contour tree in PostExecute
    if (MultiBlockTreeHelper->GetGlobalNumberOfBlocks() == 1)
    {
      return;
    }
    auto field =
      input.GetPartition(0).GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    vtkm::filter::FieldMetadata metaData(field);

    vtkm::filter::FilterTraits<ContourTreeAugmented> traits;
    vtkm::cont::CastAndCall(vtkm::filter::ApplyPolicyFieldActive(field, policy, traits),
                            detail::PostExecuteCaller{},
                            this,
                            input,
                            result,
                            metaData,
                            policy);

    delete this->MultiBlockTreeHelper;
    this->MultiBlockTreeHelper = nullptr;
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               std::endl
                 << "    "
                 << std::setw(38)
                 << std::left
                 << "Contour Tree Filter PostExecute"
                 << ": "
                 << timer.GetElapsedTime()
                 << " seconds");
  }
}


} // namespace filter
} // namespace vtkm::filter

#endif
