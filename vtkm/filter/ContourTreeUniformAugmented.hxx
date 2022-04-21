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

#include <vtkm/cont/Timer.h>

#include <vtkm/filter/ContourTreeUniformAugmented.h>
#include <vtkm/worklet/ContourTreeUniformAugmented.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include <vtkm/thirdparty/diy/diy.h>
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

#include <vtkm/worklet/contourtree_distributed/ContourTreeBlockData.h>
#include <vtkm/worklet/contourtree_distributed/SpatialDecomposition.h>
#include <vtkm/worklet/contourtree_distributed/MergeBlockFunctor.h>

#include <memory>

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

} // end namespace detail



//-----------------------------------------------------------------------------
ContourTreeAugmented::ContourTreeAugmented(bool useMarchingCubes,
                                           unsigned int computeRegularStructure)
  : vtkm::filter::FilterField<ContourTreeAugmented>()
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
    this->MultiBlockTreeHelper.reset();
  }
  this->MultiBlockTreeHelper =
    std::unique_ptr<vtkm::worklet::contourtree_distributed::MultiBlockContourTreeHelper>(
      new vtkm::worklet::contourtree_distributed::MultiBlockContourTreeHelper(
        blocksPerDim, globalSize, localBlockIndices, localBlockOrigins, localBlockSizes));
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

  // Use the GetPointDimensions struct defined in the header to collect the meshSize information
  vtkm::worklet::ContourTreeAugmented worklet;
  vtkm::Id3 meshSize;
  const auto& cells = input.GetCellSet();
  vtkm::filter::ApplyPolicyCellSet(cells, policy, *this)
    .CastAndCall(vtkm::worklet::contourtree_augmented::GetPointDimensions(), meshSize);
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
              meshSize,
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
        this->GetOutputFieldName(), vtkm::cont::Field::Association::WholeMesh, sortedValues);
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
  std::vector<vtkm::worklet::contourtree_distributed::ContourTreeBlockData<T>*> localDataBlocks;
  localDataBlocks.resize(static_cast<size_t>(input.GetNumberOfPartitions()));
  std::vector<vtkmdiy::Link*> localLinks; // dummy links needed to make DIY happy
  localLinks.resize(static_cast<size_t>(input.GetNumberOfPartitions()));
  // We need to augment at least with the boundary vertices when running in parallel, even if the user requested at the end only the unaugmented contour tree
  unsigned int compRegularStruct =
    (this->ComputeRegularStructure > 0) ? this->ComputeRegularStructure : 2;

  auto localBlocksOriginPortal =
    this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.LocalBlockOrigins.ReadPortal();
  auto localBlocksSizesPortal =
    this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.LocalBlockSizes.ReadPortal();
  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    // create the local contour tree mesh
    localLinks[bi] = new vtkmdiy::Link;
    auto currBlock = input.GetPartition(static_cast<vtkm::Id>(bi));
    auto currField =
      currBlock.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    //const vtkm::cont::ArrayHandle<T,StorageType> &fieldData = currField.GetData().Cast<vtkm::cont::ArrayHandle<T,StorageType> >();
    vtkm::cont::ArrayHandle<T> fieldData;
    vtkm::cont::ArrayCopy(currField.GetData(), fieldData);
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
    // localDataBlocks[bi]->SortOrder = currContourTreeMesh->SortOrder;
    localDataBlocks[bi]->SortedValue = currContourTreeMesh->SortedValues;
    localDataBlocks[bi]->GlobalMeshIndex = currContourTreeMesh->GlobalMeshIndex;
    localDataBlocks[bi]->NeighborConnectivity = currContourTreeMesh->NeighborConnectivity;
    localDataBlocks[bi]->NeighborOffsets = currContourTreeMesh->NeighborOffsets;
    localDataBlocks[bi]->MaxNeighbors = currContourTreeMesh->MaxNeighbors;
    localDataBlocks[bi]->BlockOrigin = localBlocksOriginPortal.Get(static_cast<vtkm::Id>(bi));
    localDataBlocks[bi]->BlockSize = localBlocksSizesPortal.Get(static_cast<vtkm::Id>(bi));
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
  using RegularDecomposer = vtkmdiy::RegularDecomposer<vtkmdiy::DiscreteBounds>;
  const vtkm::worklet::contourtree_distributed::SpatialDecomposition& spatialDecomp =
    this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition;
  const auto numDims = spatialDecomp.NumberOfDimensions();

  // ... division vector
  RegularDecomposer::DivisionsVector diyDivisions(numDims);
  for (vtkm::IdComponent d = 0;
       d < static_cast<vtkm::IdComponent>(spatialDecomp.NumberOfDimensions());
       ++d)
  {
    diyDivisions[d] = static_cast<int>(spatialDecomp.BlocksPerDimension[d]);
  }

  // ... coordinates of local blocks
  auto localBlockIndicesPortal = spatialDecomp.LocalBlockIndices.ReadPortal();
  std::vector<vtkm::Id> vtkmdiyLocalBlockGids(static_cast<size_t>(input.GetNumberOfPartitions()));
  for (vtkm::Id bi = 0; bi < input.GetNumberOfPartitions(); bi++)
  {
    RegularDecomposer::DivisionsVector diyCoords(static_cast<size_t>(numDims));
    auto currentCoords = localBlockIndicesPortal.Get(bi);
    for (vtkm::IdComponent d = 0; d < numDims; ++d)
    {
      diyCoords[d] = static_cast<int>(currentCoords[d]);
    }
    vtkmdiyLocalBlockGids[static_cast<size_t>(bi)] =
      RegularDecomposer::coords_to_gid(diyCoords, diyDivisions);
  }

  // Add my local blocks to the vtkmdiy master.
  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    master.add(static_cast<int>(vtkmdiyLocalBlockGids[bi]), // block id
               localDataBlocks[bi],
               localLinks[bi]);
  }

  // Define the decomposition of the domain into regular blocks
  RegularDecomposer::BoolVector shareFace(3, true);
  RegularDecomposer::BoolVector wrap(3, false);
  RegularDecomposer::CoordinateVector ghosts(3, 1);
  RegularDecomposer decomposer(static_cast<int>(numDims),
                               spatialDecomp.GetVTKmDIYBounds(),
                               static_cast<int>(spatialDecomp.GetGlobalNumberOfBlocks()),
                               shareFace,
                               wrap,
                               ghosts,
                               diyDivisions);

  // Define which blocks live on which rank so that vtkmdiy can manage them
  vtkmdiy::DynamicAssigner assigner(
    comm, static_cast<int>(size), static_cast<int>(spatialDecomp.GetGlobalNumberOfBlocks()));
  for (vtkm::Id bi = 0; bi < input.GetNumberOfPartitions(); bi++)
  {
    assigner.set_rank(static_cast<int>(rank),
                      static_cast<int>(vtkmdiyLocalBlockGids[static_cast<size_t>(bi)]));
  }

  // Fix the vtkmdiy links. (NOTE: includes an MPI barrier)
  vtkmdiy::fix_links(master, assigner);

  // partners for merge over regular block grid
  vtkmdiy::RegularMergePartners partners(
    decomposer, // domain decomposition
    2,          // raix of k-ary reduction.
    true        // contiguous: true=distance doubling , false=distnace halving
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
    contourTreeMeshOut.SortOrder = vtkm::cont::ArrayHandleIndex(contourTreeMeshOut.NumVertices);
    contourTreeMeshOut.SortIndices = vtkm::cont::ArrayHandleIndex(contourTreeMeshOut.NumVertices);
    contourTreeMeshOut.SortedValues = localDataBlocks[0]->SortedValue;
    contourTreeMeshOut.GlobalMeshIndex = localDataBlocks[0]->GlobalMeshIndex;
    contourTreeMeshOut.NeighborConnectivity = localDataBlocks[0]->NeighborConnectivity;
    contourTreeMeshOut.NeighborOffsets = localDataBlocks[0]->NeighborOffsets;
    contourTreeMeshOut.MaxNeighbors = localDataBlocks[0]->MaxNeighbors;
    // Construct the mesh boundary exectuion object needed for boundary augmentation
    vtkm::Id3 minIdx(0, 0, 0);
    vtkm::Id3 maxIdx = this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.GlobalSize;
    maxIdx[0] = maxIdx[0] - 1;
    maxIdx[1] = maxIdx[1] - 1;
    maxIdx[2] = maxIdx[2] > 0 ? (maxIdx[2] - 1) : 0;
    auto meshBoundaryExecObj = contourTreeMeshOut.GetMeshBoundaryExecutionObject(
      this->MultiBlockTreeHelper->MultiBlockSpatialDecomposition.GlobalSize, minIdx, maxIdx);
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
                             vtkm::cont::Field::Association::WholeMesh,
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
