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
#include <vtkm/worklet/contourtree_augmented/DataSetMesh.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/mesh_boundary/MeshBoundaryContourTreeMesh.h>

// distributed contour tree includes
#include <vtkm/worklet/contourtree_distributed/BoundaryTree.h>
#include <vtkm/worklet/contourtree_distributed/BoundaryTreeMaker.h>
#include <vtkm/worklet/contourtree_distributed/ComputeDistributedContourTreeFunctor.h>
#include <vtkm/worklet/contourtree_distributed/DistributedContourTreeBlockData.h>
#include <vtkm/worklet/contourtree_distributed/HierarchicalContourTree.h>
#include <vtkm/worklet/contourtree_distributed/InteriorForest.h>
#include <vtkm/worklet/contourtree_distributed/PrintGraph.h>
#include <vtkm/worklet/contourtree_distributed/SpatialDecomposition.h>
#include <vtkm/worklet/contourtree_distributed/TreeGrafter.h>

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
struct ComputeLocalTree
{
  template <typename T, typename Storage, typename DerivedPolicy>
  void operator()(const vtkm::cont::ArrayHandle<T, Storage>& field,
                  ContourTreeUniformDistributed* self,
                  const vtkm::Id blockIndex,
                  const vtkm::cont::DataSet& inputData,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  vtkm::filter::PolicyBase<DerivedPolicy> policy)
  {
    self->ComputeLocalTree(blockIndex, inputData, field, fieldMeta, policy);
  }
};

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
ContourTreeUniformDistributed::ContourTreeUniformDistributed(
  vtkm::Id3 blocksPerDim,
  vtkm::Id3 globalSize,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockIndices,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockOrigins,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockSizes,
  bool useMarchingCubes)
  : vtkm::filter::FilterField<ContourTreeUniformDistributed>()
  , UseMarchingCubes(useMarchingCubes)
  , MultiBlockSpatialDecomposition(blocksPerDim,
                                   globalSize,
                                   localBlockIndices,
                                   localBlockOrigins,
                                   localBlockSizes)
  , LocalMeshes(static_cast<std::size_t>(localBlockSizes.GetNumberOfValues()))
  , LocalContourTrees(static_cast<std::size_t>(localBlockSizes.GetNumberOfValues()))
  , LocalBoundaryTrees(static_cast<std::size_t>(localBlockSizes.GetNumberOfValues()))
  , LocalInteriorForests(static_cast<std::size_t>(localBlockSizes.GetNumberOfValues()))
{
  this->SetOutputFieldName("resultData");
}

template <typename DerivedPolicy>
vtkm::cont::PartitionedDataSet ContourTreeUniformDistributed::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  // Time execution
  vtkm::cont::Timer timer;
  timer.Start();

  for (vtkm::Id blockNo = 0; blockNo < input.GetNumberOfPartitions(); ++blockNo)
  {
    auto dataset = input.GetPartition(blockNo);
    auto field = dataset.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    vtkm::filter::FieldMetadata metaData(field);

    vtkm::filter::FilterTraits<ContourTreeUniformDistributed> traits;
    vtkm::cont::CastAndCall(vtkm::filter::ApplyPolicyFieldActive(field, policy, traits),
                            vtkm::filter::contourtree_distributed_detail::ComputeLocalTree{},
                            this,
                            blockNo,
                            dataset,
                            metaData,
                            policy);
  }

  // Print timing statistics
  VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
             std::endl
               << "    " << std::setw(38) << std::left << "Contour Tree Filter PrepareForExecution"
               << ": " << timer.GetElapsedTime() << " seconds");

  return input; // TODO/FIXME: What to return?
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
void ContourTreeUniformDistributed::ComputeLocalTree(
  const vtkm::Id blockIndex,
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // Check that the field is Ok
  if (fieldMeta.IsPointField() == false)
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  // Get mesh size
  vtkm::Id3 meshSize;
  const auto& cells = input.GetCellSet();
  vtkm::filter::ApplyPolicyCellSet(cells, policy, *this)
    .CastAndCall(vtkm::worklet::contourtree_augmented::GetPointDimensions(), meshSize);

  // Create the mesh we need for the contour tree computation so that we have access to it
  // afterwards to compute the BRACT for each data block as well
  if (meshSize[2] == 1) // 2D mesh
  {
    vtkm::worklet::contourtree_augmented::DataSetMeshTriangulation2DFreudenthal mesh(
      vtkm::Id2{ meshSize[0], meshSize[1] });
    this->LocalMeshes[static_cast<std::size_t>(blockIndex)] = mesh;
    auto meshBoundaryExecObject = mesh.GetMeshBoundaryExecutionObject();
    this->ComputeLocalTreeImpl(
      blockIndex, input, field, fieldMeta, policy, mesh, meshBoundaryExecObject);
  }
  else if (this->UseMarchingCubes) // 3D marching cubes mesh
  {
    vtkm::worklet::contourtree_augmented::DataSetMeshTriangulation3DMarchingCubes mesh(meshSize);
    this->LocalMeshes[static_cast<std::size_t>(blockIndex)] = mesh;
    auto meshBoundaryExecObject = mesh.GetMeshBoundaryExecutionObject();
    this->ComputeLocalTreeImpl(
      blockIndex, input, field, fieldMeta, policy, mesh, meshBoundaryExecObject);
  }
  else // Regular 3D mesh
  {
    vtkm::worklet::contourtree_augmented::DataSetMeshTriangulation3DFreudenthal mesh(meshSize);
    this->LocalMeshes[static_cast<std::size_t>(blockIndex)] = mesh;
    auto meshBoundaryExecObject = mesh.GetMeshBoundaryExecutionObject();
    this->ComputeLocalTreeImpl(
      blockIndex, input, field, fieldMeta, policy, mesh, meshBoundaryExecObject);
  }
} // ContourTreeUniformDistributed::ComputeLocalTree

//-----------------------------------------------------------------------------
template <typename T,
          typename StorageType,
          typename DerivedPolicy,
          typename MeshType,
          typename MeshBoundaryExecType>
void ContourTreeUniformDistributed::ComputeLocalTreeImpl(
  const vtkm::Id blockIndex,
  const vtkm::cont::DataSet&, // input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata&,      // fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy>, // policy,
  MeshType& mesh,
  MeshBoundaryExecType& meshBoundaryExecObject)
{
  // We always need to compute the fully augmented contour tree for our local data block
  const unsigned int compRegularStruct = 1;

  // Set up the worklet
  vtkm::worklet::ContourTreeAugmented worklet;
  worklet.Run(field,
              mesh,
              this->LocalContourTrees[static_cast<std::size_t>(blockIndex)],
              this->LocalMeshes[static_cast<std::size_t>(blockIndex)].SortOrder,
              this->NumIterations,
              compRegularStruct,
              meshBoundaryExecObject);

  // Now we compute the BRACT for our data block. We do this here because we know the MeshType
  // here and we don't need to store the mesh separately any more since it is stored in the BRACT

  // Get the mesh information needed to create an IdRelabeler to relable local to global ids
  // Create an IdRelabeler since we are using a DataSetMesh type here, we don't need
  // the IdRelabeler for the BRACT construction when we are using a ContourTreeMesh.
  auto localToGlobalIdRelabeler = vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler(
    this->MultiBlockSpatialDecomposition.LocalBlockOrigins.ReadPortal().Get(blockIndex),
    this->MultiBlockSpatialDecomposition.LocalBlockSizes.ReadPortal().Get(blockIndex),
    this->MultiBlockSpatialDecomposition.GlobalSize);
  // Initialize the BoundaryTreeMaker
  auto boundaryTreeMaker =
    vtkm::worklet::contourtree_distributed::BoundaryTreeMaker<MeshType, MeshBoundaryExecType>(
      &mesh,                                                         // The input mesh
      meshBoundaryExecObject,                                        // The mesh boundary
      this->LocalContourTrees[static_cast<std::size_t>(blockIndex)], // The contour tree
      &this->LocalBoundaryTrees[static_cast<std::size_t>(
        blockIndex)], // The boundary tree (a.k.a BRACT) to be computed
      &this->LocalInteriorForests[static_cast<std::size_t>(
        blockIndex)] // The interior forest (a.k.a. Residue) to be computed
    );
  // Execute the BRACT construction, including the compute of the InteriorForest
  boundaryTreeMaker.Construct(&localToGlobalIdRelabeler);


#ifdef DEBUG_PRINT_CTUD
  // Print BRACT for debug
  vtkm::Id rank = vtkm::cont::EnvironmentTracker::GetCommunicator().rank();
  char buffer[256];
  std::snprintf(buffer,
                sizeof(buffer),
                "Rank_%d_Block_%d_Initial_BRACT.gv",
                static_cast<int>(rank),
                static_cast<int>(blockIndex));
  std::ofstream os(buffer);
  os << this->LocalBoundaryTrees[static_cast<std::size_t>(blockIndex)].PrintGlobalDot(
          "Before Fan In",
          mesh,
          field,
          this->MultiBlockSpatialDecomposition.LocalBlockOrigins.ReadPortal().Get(blockIndex),
          this->MultiBlockSpatialDecomposition.LocalBlockSizes.ReadPortal().Get(blockIndex),
          this->MultiBlockSpatialDecomposition.GlobalSize)
     << std::endl;
#endif
  // TODO: Fix calling conventions so dot print works here
  // At this point, I'm reasonably certain that the contour tree has been computed regardless of data push/pull
  // So although it might be logical to print things out earlier, I'll do it here
  // save the regular structure
  // TODO: Oliver Fix and renable the following print calls
  /*
	std::string regularStructureFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(blockIndex)) + std::string("_Initial_Step_0_Contour_Tree_Regular_Structure.gv");
	std::ofstream regularStructureFile(regularStructureFileName);
	regularStructureFile << worklet::contourtree_distributed::ContourTreeDotGraphPrint<T, MeshType, vtkm::worklet::contourtree_augmented::IdArrayType>
																	(	std::string("Block ") + std::to_string(static_cast<std::size_t>(blockIndex)) + " Initial Step 0 Contour Tree Regular Structure", 
																		this->LocalMeshes[static_cast<std::size_t>(blockIndex)],			
																		this->LocalContourTrees[static_cast<std::size_t>(blockIndex)],
																		worklet::contourtree_distributed::SHOW_REGULAR_STRUCTURE|worklet::contourtree_distributed::SHOW_ALL_IDS);
*/
  /*
	std::string superStructureFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(blockIndex)) + std::string("_Initial_Step_1_Contour_Tree_Super_Structure.gv");
	std::ofstream superStructureFile(superStructureFileName);
  vtkm::Id ctPrintSettings = worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE|worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE|worklet::contourtree_distributed::SHOW_ALL_IDS|worklet::contourtree_distributed::SHOW_ALL_SUPERIDS|worklet::contourtree_distributed::SHOW_ALL_HYPERIDS;
  std::string ctPrintLabel = std::string("Block ") + std::to_string(static_cast<size_t>(blockIndex)) + " Initial Step 1 Contour Tree Super Structure";
	superStructureFile << vtkm::worklet::contourtree_distributed::ContourTreeDotGraphPrint<T, StorageType, MeshType, vtkm::worklet::contourtree_augmented::IdArrayType>
																	(	ctPrintLabel,
																		dynamic_cast<MeshType&>(this->LocalMeshes[static_cast<std::size_t>(blockIndex)]),
                                    &localToGlobalIdRelabeler,
                                    field,
																		this->LocalContourTrees[static_cast<std::size_t>(blockIndex)],
                                    ctPrintSettings);
                                    
	// save the Boundary Tree as a dot file
	std::string boundaryTreeFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<size_t>(blockIndex)) + std::string("_Initial_Step_3_Boundary_Tree.gv");
	std::ofstream boundaryTreeFile(boundaryTreeFileName);
	boundaryTreeFile << vtkm::worklet::contourtree_distributed::BoundaryTreeDotGraphPrint
																	(	std::string("Block ") + std::to_string(static_cast<size_t>(blockIndex)) + std::string(" Initial Step 3 Boundary Tree"),
																		dynamic_cast<MeshType&>(this->LocalMeshes[static_cast<std::size_t>(blockIndex)]),
                                    meshBoundaryExecObject,
                                    this->LocalBoundaryTrees[static_cast<std::size_t>(blockIndex)],
                                    &localToGlobalIdRelabeler,
                                    field);

	// and save the Interior Forest as another dot file
	std::string interiorForestFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(blockIndex)) + std::string("_Initial_Step_4_Interior_Forest.gv");
	std::ofstream interiorForestFile(interiorForestFileName);
	interiorForestFile << worklet::contourtree_distributed::InteriorForestDotGraphPrint
																	(	std::string("Block ") + std::to_string(rank) + " Initial Step 4 Interior Forest", 
																		this->LocalInteriorForests[static_cast<std::size_t>(blockIndex)], 
																		this->LocalContourTrees[static_cast<std::size_t>(blockIndex)],
																		this->LocalBoundaryTrees[static_cast<std::size_t>(blockIndex)],
																		dynamic_cast<MeshType&>(this->LocalMeshes[static_cast<std::size_t>(blockIndex)]),
                                    meshBoundaryExecObject,
                                    &localToGlobalIdRelabeler,
                                    field);
*/
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT void ContourTreeUniformDistributed::PreExecute(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  if (vtkm::worklet::contourtree_distributed::SpatialDecomposition::GetGlobalNumberOfBlocks(
        input) != this->MultiBlockSpatialDecomposition.GetGlobalNumberOfBlocks())
  {
    throw vtkm::cont::ErrorFilterExecution(
      "Global number of blocks in MultiBlock dataset does not match the SpatialDecomposition");
  }
  if (this->MultiBlockSpatialDecomposition.GetLocalNumberOfBlocks() !=
      input.GetNumberOfPartitions())
  {
    throw vtkm::cont::ErrorFilterExecution(
      "Local number of blocks in MultiBlock dataset does not match the SpatialDecomposition");
  }
}

//-----------------------------------------------------------------------------
template <typename FieldType, typename StorageType, typename DerivedPolicy>
VTKM_CONT void ContourTreeUniformDistributed::DoPostExecute(
  const vtkm::cont::PartitionedDataSet& input,
  vtkm::cont::PartitionedDataSet& result,
  const vtkm::filter::FieldMetadata&,                     // dummy parameter for field meta data
  const vtkm::cont::ArrayHandle<FieldType, StorageType>&, // dummy parameter to get the type
  vtkm::filter::PolicyBase<DerivedPolicy>)                // dummy parameter for policy
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id size = comm.size();
  vtkm::Id rank = comm.rank();

  std::vector<vtkm::worklet::contourtree_distributed::DistributedContourTreeBlockData<FieldType>*>
    localDataBlocks(static_cast<size_t>(input.GetNumberOfPartitions()), nullptr);
  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    // Create the local data block structure and set extents
    localDataBlocks[bi] =
      new vtkm::worklet::contourtree_distributed::DistributedContourTreeBlockData<FieldType>();
    localDataBlocks[bi]->BlockIndex = static_cast<vtkm::Id>(bi);
    localDataBlocks[bi]->BlockOrigin =
      this->MultiBlockSpatialDecomposition.LocalBlockOrigins.ReadPortal().Get(
        static_cast<vtkm::Id>(bi));
    localDataBlocks[bi]->BlockSize =
      this->MultiBlockSpatialDecomposition.LocalBlockSizes.ReadPortal().Get(
        static_cast<vtkm::Id>(bi));

    // Save local tree information for fan out FIXME: Try to avoid copy
    localDataBlocks[bi]->ContourTrees.push_back(this->LocalContourTrees[bi]);
    localDataBlocks[bi]->InteriorForests.push_back(this->LocalInteriorForests[bi]);

    // ... Compute arrays needed for constructing contour tree mesh
    const auto sortOrder = this->LocalMeshes[bi].SortOrder;
    // ... Compute the global mesh index for the partially augmented contour tree. I.e., here we
    // don't need the global mesh index for all nodes, but only for the augmented nodes from the
    // tree. We, hence, permute the sortOrder by contourTree.augmentednodes and then compute the
    // GlobalMeshIndex by tranforming those indices with our IdRelabler
    vtkm::worklet::contourtree_augmented::IdArrayType localGlobalMeshIndex;
    vtkm::cont::ArrayHandlePermutation<vtkm::worklet::contourtree_augmented::IdArrayType,
                                       vtkm::worklet::contourtree_augmented::IdArrayType>
      permutedSortOrder(this->LocalBoundaryTrees[bi].VertexIndex, sortOrder);
    auto transformedIndex = vtkm::cont::make_ArrayHandleTransform(
      permutedSortOrder,
      vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler(
        this->MultiBlockSpatialDecomposition.LocalBlockOrigins.ReadPortal().Get(
          static_cast<vtkm::Id>(bi)),
        this->MultiBlockSpatialDecomposition.LocalBlockSizes.ReadPortal().Get(
          static_cast<vtkm::Id>(bi)),
        this->MultiBlockSpatialDecomposition.GlobalSize));
    vtkm::cont::Algorithm::Copy(transformedIndex, localGlobalMeshIndex);

    // ... get data values
    auto currBlock = input.GetPartition(static_cast<vtkm::Id>(bi));
    auto currField =
      currBlock.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    vtkm::cont::ArrayHandle<FieldType> fieldData;
    vtkm::cont::ArrayCopy(currField.GetData().template AsVirtual<FieldType>(), fieldData);

    // ... compute and store the actual mesh
    localDataBlocks[bi]->ContourTreeMeshes.emplace_back(this->LocalBoundaryTrees[bi].VertexIndex,
                                                        this->LocalBoundaryTrees[bi].Superarcs,
                                                        sortOrder,
                                                        fieldData,
                                                        localGlobalMeshIndex);

#ifdef DEBUG_PRINT_CTUD
    // ... save for debugging in text and .gv/.dot format
    char buffer[256];
    std::snprintf(buffer,
                  sizeof(buffer),
                  "Rank_%d_Block_%d_Initial_Step_3_BRACT_Mesh.txt",
                  static_cast<int>(rank),
                  static_cast<int>(bi));
    localDataBlocks[bi]->ContourTreeMeshes.back().Save(buffer);

    // save the corresponding .gv file
    std::string boundaryTreeMeshFileName = std::string("Rank_") +
      std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
      std::to_string(static_cast<int>(bi)) + std::string("_Initial_Step_5_BRACT_Mesh.gv");
    std::ofstream boundaryTreeMeshFile(boundaryTreeMeshFileName);
    boundaryTreeMeshFile
      << vtkm::worklet::contourtree_distributed::ContourTreeMeshDotGraphPrint<FieldType>(
           std::string("Block ") + std::to_string(static_cast<int>(rank)) +
             std::string(" Initial Step 5 BRACT Mesh"),
           localDataBlocks[bi]->ContourTreeMeshes.back(),
           worklet::contourtree_distributed::SHOW_CONTOUR_TREE_MESH_ALL);
#endif
  }

  // Setup vtkmdiy to do global binary reduction of neighbouring blocks. See also RecuctionOperation struct for example
  // Create the vtkmdiy master
  vtkmdiy::Master master(comm,
                         1, // Use 1 thread, VTK-M will do the treading
                         -1 // All block in memory
  );

  // Compute the gids for our local blocks
  const vtkm::worklet::contourtree_distributed::SpatialDecomposition& spatialDecomp =
    this->MultiBlockSpatialDecomposition;
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

  std::vector<vtkmdiy::Link*> localLinks(static_cast<std::vector<vtkmdiy::Link>::size_type>(
    input.GetNumberOfPartitions())); // dummy links needed to make DIY happy
  // Add my local blocks to the vtkmdiy master.
  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    localLinks[bi] = new vtkmdiy::Link;
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

  // Fix the vtkmdiy links.
  vtkmdiy::fix_links(master, assigner);

  // partners for merge over regular block grid
  vtkmdiy::RegularSwapPartners partners(
    decomposer, // domain decomposition
    2,          // raix of k-ary reduction. TODO check this value
    true        // contiguous: true=distance doubling , false=distnace halving TODO check this value
  );
  // reduction
  const vtkm::worklet::contourtree_distributed::ComputeDistributedContourTreeFunctor<FieldType>
    computeDistributedContourTreeFunctor{ this->MultiBlockSpatialDecomposition.GlobalSize };
  vtkmdiy::reduce(master, assigner, partners, computeDistributedContourTreeFunctor);

  comm.barrier(); // Be safe!

  std::vector<vtkm::cont::DataSet> hierarchicalTreeOutputDataSet(
    localDataBlocks.size()); // DataSets for creating output data
  master.foreach ([&](vtkm::worklet::contourtree_distributed::DistributedContourTreeBlockData<
                        FieldType>* b,
                      const vtkmdiy::Master::ProxyWithLink&) {
#ifdef DEBUG_PRINT_CTUD
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Fan In Complete" << std::endl
                                 << "# of CTs: " << b->ContourTrees.size() << std::endl
                                 << "# of CTMs: " << b->ContourTreeMeshes.size() << std::endl
                                 << "# of IFs: " << b->InteriorForests.size() << std::endl);

    char buffer[256];
    std::snprintf(buffer,
                  sizeof(buffer),
                  "AfterFanInResults_Rank%d_Block%d.txt",
                  static_cast<int>(rank),
                  static_cast<int>(b->BlockIndex));
    std::ofstream os(buffer);
    os << "Contour Trees" << std::endl;
    os << "=============" << std::endl;
    for (const auto& ct : b->ContourTrees)
      ct.PrintContent(os);
    os << std::endl;
    os << "Contour Tree Meshes" << std::endl;
    os << "===================" << std::endl;
    for (const auto& cm : b->ContourTreeMeshes)
      cm.PrintContent(os);
    os << std::endl;
    os << "Interior Forests" << std::endl;
    os << "===================" << std::endl;
    for (const auto& info : b->InteriorForests)
      info.PrintContent(os);
    os << std::endl;
#endif

    // Fan out
    auto nRounds = b->ContourTrees.size() - 1;

    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Fan out. nRounds=" << nRounds);
    vtkm::worklet::contourtree_distributed::HierarchicalContourTree<FieldType> hierarchicalTree;
    hierarchicalTree.Initialize(
      static_cast<vtkm::Id>(nRounds), b->ContourTrees[nRounds], b->ContourTreeMeshes[nRounds - 1]);

    // TODO: GET THIS COMPILING
    // save the corresponding .gv file
    // 		std::string hierarchicalTreeFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(b->BlockIndex)) + "_Round_" + std::to_string(nRounds) + "_Hierarchical_Tree.gv";
    // 		std::ofstream hierarchicalTreeFile(hierarchicalTreeFileName);
    // 		hierarchicalTreeFile << vtkm::worklet::contourtree_distributed::HierarchicalContourTreeDotGraphPrint<FieldType>
    // 								(	std::string("Block ") + std::to_string(static_cast<int>(b->BlockIndex)) + " Round " + std::to_string(nRounds) + " Hierarchical Tree", hierarchicalTree,
    //  								vtkm::worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE|vtkm::worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE|vtkm::worklet::contourtree_distributed::SHOW_ALL_IDS|vtkm::worklet::contourtree_distributed::SHOW_ALL_SUPERIDS|vtkm::worklet::contourtree_distributed::SHOW_ALL_HYPERIDS
    // 								);

    for (auto round = nRounds - 1; round > 0; round--)
    {
      vtkm::worklet::contourtree_distributed::
        TreeGrafter<vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType>, FieldType>
          grafter(&(b->ContourTreeMeshes[round - 1]),
                  b->ContourTrees[round],
                  &(b->InteriorForests[round]));
      grafter.GraftInteriorForests(static_cast<vtkm::Id>(round),
                                   hierarchicalTree,
                                   b->ContourTreeMeshes[round - 1].SortedValues);
      // TODO: GET THIS COMPILING
      // save the corresponding .gv file
      // 		std::string hierarchicalTreeFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(b->BlockIndex)) + "_Round_" + std::to_string(nRounds) + "_Hierarchical_Tree.gv";
      // 		std::ofstream hierarchicalTreeFile(hierarchicalTreeFileName);
      // 		hierarchicalTreeFile << vtkm::worklet::contourtree_distributed::HierarchicalContourTreeDotGraphPrint<FieldType>
      // 								(	std::string("Block ") + std::to_string(static_cast<int>(b->BlockIndex)) + " Round " + std::to_string(nRounds) + " Hierarchical Tree", hierarchicalTree,
      //  								vtkm::worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE|vtkm::worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE|vtkm::worklet::contourtree_distributed::SHOW_ALL_IDS|vtkm::worklet::contourtree_distributed::SHOW_ALL_SUPERIDS|vtkm::worklet::contourtree_distributed::SHOW_ALL_HYPERIDS
      // 								);
    }

    // bottom level
    vtkm::worklet::contourtree_distributed::
      TreeGrafter<vtkm::worklet::contourtree_augmented::DataSetMesh, FieldType>
        grafter(&(this->LocalMeshes[static_cast<std::size_t>(b->BlockIndex)]),
                b->ContourTrees[0],
                &(b->InteriorForests[0]));
    auto currBlock = input.GetPartition(b->BlockIndex);
    auto currField =
      currBlock.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());
    vtkm::cont::ArrayHandle<FieldType> fieldData;
    vtkm::cont::ArrayCopy(currField.GetData().template AsVirtual<FieldType>(), fieldData);
    auto localToGlobalIdRelabeler = vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler(
      this->MultiBlockSpatialDecomposition.LocalBlockOrigins.ReadPortal().Get(b->BlockIndex),
      this->MultiBlockSpatialDecomposition.LocalBlockSizes.ReadPortal().Get(b->BlockIndex),
      this->MultiBlockSpatialDecomposition.GlobalSize);
    grafter.GraftInteriorForests(0, hierarchicalTree, fieldData, &localToGlobalIdRelabeler);

    // Create data set from output
    vtkm::cont::Field dataValuesField(
      "DataValues", vtkm::cont::Field::Association::WHOLE_MESH, hierarchicalTree.DataValues);
    hierarchicalTreeOutputDataSet[b->BlockIndex].AddField(dataValuesField);
    vtkm::cont::Field regularNodeGlobalIdsField("RegularNodeGlobalIds",
                                                vtkm::cont::Field::Association::WHOLE_MESH,
                                                hierarchicalTree.RegularNodeGlobalIds);
    hierarchicalTreeOutputDataSet[b->BlockIndex].AddField(regularNodeGlobalIdsField);
    vtkm::cont::Field superarcsField(
      "Superarcs", vtkm::cont::Field::Association::WHOLE_MESH, hierarchicalTree.Superarcs);
    hierarchicalTreeOutputDataSet[b->BlockIndex].AddField(superarcsField);
    vtkm::cont::Field supernodesField(
      "Supernodes", vtkm::cont::Field::Association::WHOLE_MESH, hierarchicalTree.Supernodes);
    hierarchicalTreeOutputDataSet[b->BlockIndex].AddField(supernodesField);
    vtkm::cont::Field superparentsField(
      "Superparents", vtkm::cont::Field::Association::WHOLE_MESH, hierarchicalTree.Superparents);
    hierarchicalTreeOutputDataSet[b->BlockIndex].AddField(superparentsField);

    // TODO: GET THIS COMPILING
    // save the corresponding .gv file
    // 		std::string hierarchicalTreeFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(b->BlockIndex)) + "_Round_" + std::to_string(nRounds) + "_Hierarchical_Tree.gv";
    // 		std::ofstream hierarchicalTreeFile(hierarchicalTreeFileName);
    // 		hierarchicalTreeFile << vtkm::worklet::contourtree_distributed::HierarchicalContourTreeDotGraphPrint<FieldType>
    // 								(	std::string("Block ") + std::to_string(static_cast<int>(b->BlockIndex)) + " Round " + std::to_string(nRounds) + " Hierarchical Tree", hierarchicalTree,
    //  								vtkm::worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE|vtkm::worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE|vtkm::worklet::contourtree_distributed::SHOW_ALL_IDS|vtkm::worklet::contourtree_distributed::SHOW_ALL_SUPERIDS|vtkm::worklet::contourtree_distributed::SHOW_ALL_HYPERIDS
    // 								);
  });

  result = vtkm::cont::PartitionedDataSet(hierarchicalTreeOutputDataSet);
}


//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
inline VTKM_CONT void ContourTreeUniformDistributed::PostExecute(
  const vtkm::cont::PartitionedDataSet& input,
  vtkm::cont::PartitionedDataSet& result,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  vtkm::cont::Timer timer;
  timer.Start();
  // We are running in parallel and need to merge the contour tree in PostExecute
  // TODO/FIXME: Make sure this still makes sense
  if (this->MultiBlockSpatialDecomposition.GetGlobalNumberOfBlocks() == 1)
  {
    return;
  }
  auto field = // TODO/FIXME: Correct for more than one block per rank?
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

  VTKM_LOG_S(vtkm::cont::LogLevel::Perf,
             std::endl
               << "    " << std::setw(38) << std::left << "Contour Tree Filter PostExecute"
               << ": " << timer.GetElapsedTime() << " seconds");
}

} // namespace filter
} // namespace vtkm::filter

#endif
