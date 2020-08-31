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

#ifndef vtk_m_filter_ContourTreeUniformDistributed_hxx
#define vtk_m_filter_ContourTreeUniformDistributed_hxx

// vtkm includes
#include <vtkm/cont/Timer.h>

// single-node augmented contour tree includes
#include <vtkm/filter/ContourTreeUniformDistributed.h>
#include <vtkm/worklet/ContourTreeUniformAugmented.h>
#include <vtkm/worklet/contourtree_augmented/mesh_dem_meshtypes/ContourTreeMesh.h>

// distributed contour tree includes
#include <vtkm/worklet/contourtree_distributed/BoundaryRestrictedAugmentedContourTree.h>
#include <vtkm/worklet/contourtree_distributed/BoundaryRestrictedAugmentedContourTreeMaker.h>
#include <vtkm/worklet/contourtree_distributed/ContourTreeBlockData.h>
#include <vtkm/worklet/contourtree_distributed/MergeBlockFunctor.h>
#include <vtkm/worklet/contourtree_distributed/SpatialDecomposition.h>

// DIY includes
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
namespace contourtree_distributed_detail
{
//----Helper struct to call DoPostExecute. This is used to be able to
//    wrap the PostExecute work in a functor so that we can use VTK-M's
//    vtkm::cont::CastAndCall to infer the FieldType template parameters
struct PostExecuteCaller
{
  template <typename T, typename S, typename DerivedPolicy>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>&,
                            ContourTreeUniformDistributed* self,
                            const vtkm::cont::PartitionedDataSet& input,
                            vtkm::cont::PartitionedDataSet& output,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy) const
  {
    vtkm::cont::ArrayHandle<T, S> dummy;
    self->DoPostExecute(input, output, fieldMeta, dummy, policy);
  }
};

} // end namespace contourtree_distributed_detail


//-----------------------------------------------------------------------------
ContourTreeUniformDistributed::ContourTreeUniformDistributed(bool useMarchingCubes)
  : vtkm::filter::FilterCell<ContourTreeUniformDistributed>()
  , UseMarchingCubes(useMarchingCubes)
  , MultiBlockTreeHelper(nullptr)
{
  this->SetOutputFieldName("resultData");
}

void ContourTreeUniformDistributed::SetSpatialDecomposition(
  vtkm::Id3 blocksPerDim,
  vtkm::Id3 globalSize,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockIndices,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockOrigins,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockSizes)
{
  if (this->MultiBlockTreeHelper)
  {
    this->MultiBlockTreeHelper.reset();
  }
  this->MultiBlockTreeHelper =
    std::unique_ptr<vtkm::worklet::contourtree_distributed::MultiBlockContourTreeHelper>(
      new vtkm::worklet::contourtree_distributed::MultiBlockContourTreeHelper(
        blocksPerDim, globalSize, localBlockIndices, localBlockOrigins, localBlockSizes));
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
vtkm::cont::DataSet ContourTreeUniformDistributed::DoExecute(
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
  vtkm::filter::ApplyPolicyCellSet(cells, policy, *this)
    .CastAndCall(vtkm::worklet::contourtree_augmented::GetRowsColsSlices(), nRows, nCols, nSlices);
  // TODO blockIndex needs to change if we have multiple blocks per MPI rank and DoExecute is called for multiple blocks
  std::size_t blockIndex = 0;

  // We always need to compute the fully augemented contour tree for our local data block
  unsigned int compRegularStruct = 1;

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

  VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
             std::endl
               << "    " << std::setw(38) << std::left << "Contour Tree Filter DoExecute"
               << ": " << timer.GetElapsedTime() << " seconds");

  // Construct the expected result for serial execution. Note, in serial the result currently
  // not actually being used, but in parallel we need the sorted mesh values as output
  // This part is being hit when we run in serial or parallel with more then one rank
  return CreateResultFieldPoint(input, ContourTreeData.Arcs, this->GetOutputFieldName());
} // ContourTreeUniformDistributed::DoExecute


//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT void ContourTreeUniformDistributed::PreExecute(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
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
  }
}


//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
VTKM_CONT void ContourTreeUniformDistributed::DoPostExecute(
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
  std::vector<vtkm::worklet::contourtree_distributed::ContourTreeBlockData<T>*> localDataBlocks;
  localDataBlocks.resize(static_cast<size_t>(input.GetNumberOfPartitions()));
  std::vector<vtkmdiy::Link*> localLinks; // dummy links needed to make DIY happy
  localLinks.resize(static_cast<size_t>(input.GetNumberOfPartitions()));
  // TODO Check setting. In parallel we should need to augment the tree in order to be able to do the merging
  unsigned int compRegularStruct = 1;
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
    auto currContourTreeMesh = vtkm::worklet::contourtree_distributed::MultiBlockContourTreeHelper::
      ComputeLocalContourTreeMesh<T>(
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
    localDataBlocks[bi] = new vtkm::worklet::contourtree_distributed::ContourTreeBlockData<T>();
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
  const vtkm::worklet::contourtree_distributed::SpatialDecomposition& spatialDecomp =
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
  vtkmdiy::reduce(
    master, assigner, partners, &vtkm::worklet::contourtree_distributed::MergeBlockFunctor<T>);

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
      compRegularStruct,
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
inline VTKM_CONT void ContourTreeUniformDistributed::PostExecute(
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

    vtkm::filter::FilterTraits<ContourTreeUniformDistributed> traits;
    vtkm::cont::CastAndCall(vtkm::filter::ApplyPolicyFieldActive(field, policy, traits),
                            vtkm::filter::contourtree_distributed_detail::PostExecuteCaller{},
                            this,
                            input,
                            result,
                            metaData,
                            policy);

    this->MultiBlockTreeHelper.reset();
    VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
               std::endl
                 << "    " << std::setw(38) << std::left << "Contour Tree Filter PostExecute"
                 << ": " << timer.GetElapsedTime() << " seconds");
  }
}


} // namespace filter
} // namespace vtkm::filter

#endif
