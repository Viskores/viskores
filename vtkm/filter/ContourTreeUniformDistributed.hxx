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

//-----------------------------------------------------------------------------
// Helper structs needed to support approbriate type discovery as part
// of pre- and post-execute
//-----------------------------------------------------------------------------
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

/// Helper function for saving the content of the tree for debugging
template <typename FieldType>
void SaveAfterFanInResults(
  vtkm::worklet::contourtree_distributed::DistributedContourTreeBlockData<FieldType>* blockData,
  vtkm::Id rank,
  vtkm::cont::LogLevel logLevel)
{
  (void)logLevel; // Suppress unused variable warning if logging is disabled
  VTKM_LOG_S(logLevel,
             "Fan In Complete" << std::endl
                               << "# of CTs: " << blockData->ContourTrees.size() << std::endl
                               << "# of CTMs: " << blockData->ContourTreeMeshes.size() << std::endl
                               << "# of IFs: " << blockData->InteriorForests.size() << std::endl);

  char buffer[256];
  std::snprintf(buffer,
                sizeof(buffer),
                "AfterFanInResults_Rank%d_Block%d.txt",
                static_cast<int>(rank),
                static_cast<int>(blockData->BlockIndex));
  std::ofstream os(buffer);
  os << "Contour Trees" << std::endl;
  os << "=============" << std::endl;
  for (const auto& ct : blockData->ContourTrees)
    ct.PrintContent(os);
  os << std::endl;
  os << "Contour Tree Meshes" << std::endl;
  os << "===================" << std::endl;
  for (const auto& cm : blockData->ContourTreeMeshes)
    cm.PrintContent(os);
  os << std::endl;
  os << "Interior Forests" << std::endl;
  os << "===================" << std::endl;
  for (const auto& info : blockData->InteriorForests)
    info.PrintContent(os);
  os << std::endl;
}

template <typename FieldType>
void SaveHierarchicalTreeDot(
  vtkm::worklet::contourtree_distributed::DistributedContourTreeBlockData<FieldType>* blockData,
  vtkm::worklet::contourtree_distributed::HierarchicalContourTree<FieldType>& hierarchicalTree,
  vtkm::Id rank,
  vtkm::Id nRounds)
{
  std::string hierarchicalTreeFileName = std::string("Rank_") +
    std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
    std::to_string(static_cast<int>(blockData->BlockIndex)) + std::string("_Round_") +
    std::to_string(nRounds) + std::string("_Hierarchical_Tree.gv");
  std::string hierarchicalTreeLabel = std::string("Block ") +
    std::to_string(static_cast<int>(blockData->BlockIndex)) + std::string(" Round ") +
    std::to_string(nRounds) + std::string(" Hierarchical Tree");
  vtkm::Id hierarchicalTreeDotSettings =
    vtkm::worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE |
    vtkm::worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE |
    vtkm::worklet::contourtree_distributed::SHOW_ALL_IDS |
    vtkm::worklet::contourtree_distributed::SHOW_ALL_SUPERIDS |
    vtkm::worklet::contourtree_distributed::SHOW_ALL_HYPERIDS;
  std::ofstream hierarchicalTreeFile(hierarchicalTreeFileName);
  hierarchicalTreeFile
    << vtkm::worklet::contourtree_distributed::HierarchicalContourTreeDotGraphPrint<FieldType>(
         hierarchicalTreeLabel, hierarchicalTree, hierarchicalTreeDotSettings);
}

} // end namespace contourtree_distributed_detail


//-----------------------------------------------------------------------------
// Main constructor
//-----------------------------------------------------------------------------
ContourTreeUniformDistributed::ContourTreeUniformDistributed(
  vtkm::Id3 blocksPerDim,
  vtkm::Id3 globalSize,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockIndices,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockOrigins,
  const vtkm::cont::ArrayHandle<vtkm::Id3>& localBlockSizes,
  bool useBoundaryExtremaOnly,
  bool useMarchingCubes,
  bool saveDotFiles,
  vtkm::cont::LogLevel timingsLogLevel,
  vtkm::cont::LogLevel treeLogLevel)
  : vtkm::filter::FilterField<ContourTreeUniformDistributed>()
  , UseBoundaryExtremaOnly(useBoundaryExtremaOnly)
  , UseMarchingCubes(useMarchingCubes)
  , SaveDotFiles(saveDotFiles)
  , TimingsLogLevel(timingsLogLevel)
  , TreeLogLevel(treeLogLevel)
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


//-----------------------------------------------------------------------------
// Functions used in PrepareForExecution() to compute the local contour
// tree for the data blocks processed by this rank.
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
  vtkm::cont::Timer timer;
  timer.Start();
  // We always need to compute the fully augmented contour tree for our local data block
  const unsigned int compRegularStruct = 1;

  // Set up the worklet
  vtkm::worklet::ContourTreeAugmented worklet;
  worklet.TimingsLogLevel = vtkm::cont::LogLevel::Off; // turn of the loggin, we do this afterwards
  worklet.Run(field,
              mesh,
              this->LocalContourTrees[static_cast<std::size_t>(blockIndex)],
              this->LocalMeshes[static_cast<std::size_t>(blockIndex)].SortOrder,
              this->NumIterations,
              compRegularStruct,
              meshBoundaryExecObject);
  // Log the contour tree timiing stats
  VTKM_LOG_S(this->TimingsLogLevel,
             std::endl
               << "    ---------------- Contour Tree Worklet Timings ------------------"
               << std::endl
               << "    Block Index : " << blockIndex << std::endl
               << worklet.TimingsLogString);
  VTKM_LOG_S(this->TimingsLogLevel,
             std::endl
               << "    "
                  "ComputeLocalTree ContourTree (blockIndex="
               << blockIndex << ") "
               << ": " << timer.GetElapsedTime() << " seconds");
  timer.Start();
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
  boundaryTreeMaker.Construct(&localToGlobalIdRelabeler, this->UseBoundaryExtremaOnly);
  // Log timing statistics
  VTKM_LOG_S(this->TimingsLogLevel,
             std::endl
               << "    "
                  "ComputeLocalTree BoundaryTreeMaker (blockIndex="
               << blockIndex << ") "
               << ": " << timer.GetElapsedTime() << " seconds");
  timer.Start();

  // At this point, I'm reasonably certain that the contour tree has been computed regardless of data push/pull
  // So although it might be logical to print things out earlier, I'll do it here
  // save the regular structure
  if (this->SaveDotFiles)
  {
    // Get the rank
    vtkm::Id rank = vtkm::cont::EnvironmentTracker::GetCommunicator().rank();

    // Save the BRACT dot for debug
    { // make context so the file will be closed and potentially large strings are cleaned up
      std::string bractFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) +
        std::string("_Block_") + std::to_string(static_cast<int>(blockIndex)) + "_Initial_BRACT.gv";
      std::ofstream bractFile(bractFileName);
      std::string bractString =
        this->LocalBoundaryTrees[static_cast<std::size_t>(blockIndex)].PrintGlobalDot(
          "Before Fan In",
          mesh,
          field,
          this->MultiBlockSpatialDecomposition.LocalBlockOrigins.ReadPortal().Get(blockIndex),
          this->MultiBlockSpatialDecomposition.LocalBlockSizes.ReadPortal().Get(blockIndex),
          this->MultiBlockSpatialDecomposition.GlobalSize);
      bractFile << bractString << std::endl;
    }

    // Save the regular structure as a dot file
    { // make context so the file will be closed and potentially large strings are cleaned up
      std::string regularStructureFileName = std::string("Rank_") +
        std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
        std::to_string(static_cast<int>(blockIndex)) +
        std::string("_Initial_Step_0_Contour_Tree_Regular_Structure.gv");
      std::ofstream regularStructureFile(regularStructureFileName);
      std::string label = std::string("Block ") +
        std::to_string(static_cast<std::size_t>(blockIndex)) +
        " Initial Step 0 Contour Tree Regular Structure";
      vtkm::Id dotSettings = worklet::contourtree_distributed::SHOW_REGULAR_STRUCTURE |
        worklet::contourtree_distributed::SHOW_ALL_IDS;
      std::string regularStructureString =
        worklet::contourtree_distributed::ContourTreeDotGraphPrint<
          T,
          StorageType,
          MeshType,
          vtkm::worklet::contourtree_augmented::IdArrayType>(
          label, // graph title
          static_cast<MeshType&>(this->LocalMeshes[static_cast<std::size_t>(
            blockIndex)]),           // the underlying mesh for the contour tree
          &localToGlobalIdRelabeler, // relabler needed to compute global ids
          field,                     // data values
          this->LocalContourTrees[static_cast<std::size_t>(blockIndex)], // local contour tree
          dotSettings // mask with flags for what elements to show
        );
      regularStructureFile << regularStructureString << std::endl;
    }

    // Save the super structure as a dot file
    { // make context so the file will be closed and potentially large strings are cleaned up
      std::string superStructureFileName = std::string("Rank_") +
        std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
        std::to_string(static_cast<int>(blockIndex)) +
        std::string("_Initial_Step_1_Contour_Tree_Super_Structure.gv");
      std::ofstream superStructureFile(superStructureFileName);
      vtkm::Id ctPrintSettings = worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE |
        worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE |
        worklet::contourtree_distributed::SHOW_ALL_IDS |
        worklet::contourtree_distributed::SHOW_ALL_SUPERIDS |
        worklet::contourtree_distributed::SHOW_ALL_HYPERIDS;
      std::string ctPrintLabel = std::string("Block ") +
        std::to_string(static_cast<size_t>(blockIndex)) +
        " Initial Step 1 Contour Tree Super Structure";
      std::string superStructureString =
        vtkm::worklet::contourtree_distributed::ContourTreeDotGraphPrint<
          T,
          StorageType,
          MeshType,
          vtkm::worklet::contourtree_augmented::IdArrayType>(
          ctPrintLabel,
          static_cast<MeshType&>(this->LocalMeshes[static_cast<std::size_t>(blockIndex)]),
          &localToGlobalIdRelabeler,
          field,
          this->LocalContourTrees[static_cast<std::size_t>(blockIndex)],
          ctPrintSettings);
      superStructureFile << superStructureString << std::endl;
    }

    // save the Boundary Tree as a dot file
    { // make context so the file will be closed and potentially large strings are cleaned up
      std::string boundaryTreeFileName = std::string("Rank_") +
        std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
        std::to_string(static_cast<size_t>(blockIndex)) +
        std::string("_Initial_Step_3_Boundary_Tree.gv");
      std::ofstream boundaryTreeFile(boundaryTreeFileName);
      std::string boundaryTreeString =
        vtkm::worklet::contourtree_distributed::BoundaryTreeDotGraphPrint(
          std::string("Block ") + std::to_string(static_cast<size_t>(blockIndex)) +
            std::string(" Initial Step 3 Boundary Tree"),
          static_cast<MeshType&>(this->LocalMeshes[static_cast<std::size_t>(blockIndex)]),
          meshBoundaryExecObject,
          this->LocalBoundaryTrees[static_cast<std::size_t>(blockIndex)],
          &localToGlobalIdRelabeler,
          field);
      boundaryTreeFile << boundaryTreeString << std::endl;
    }

    // and save the Interior Forest as another dot file
    { // make context so the file will be closed and potentially large strings are cleaned up
      std::string interiorForestFileName = std::string("Rank_") +
        std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
        std::to_string(static_cast<int>(blockIndex)) +
        std::string("_Initial_Step_4_Interior_Forest.gv");
      std::ofstream interiorForestFile(interiorForestFileName);
      std::string interiorForestString =
        worklet::contourtree_distributed::InteriorForestDotGraphPrint(
          std::string("Block ") + std::to_string(rank) + " Initial Step 4 Interior Forest",
          this->LocalInteriorForests[static_cast<std::size_t>(blockIndex)],
          this->LocalContourTrees[static_cast<std::size_t>(blockIndex)],
          this->LocalBoundaryTrees[static_cast<std::size_t>(blockIndex)],
          static_cast<MeshType&>(this->LocalMeshes[static_cast<std::size_t>(blockIndex)]),
          meshBoundaryExecObject,
          &localToGlobalIdRelabeler,
          field);
      interiorForestFile << interiorForestString << std::endl;

      // Log timing statistics
      VTKM_LOG_S(this->TimingsLogLevel,
                 std::endl
                   << "    " << std::setw(38) << std::left << "ComputeLocalTree Save Dot"
                   << ": " << timer.GetElapsedTime() << " seconds");
    }
  } // if (this->SaveDotFiles)
} // ContourTreeUniformDistributed::ComputeLocalTreeImpl


//-----------------------------------------------------------------------------
// Main execution phases of the filter
//
// The functions are executed by VTKm in the following order
// - PreExecute
// - PrepareForExecution
//   --> ComputeLocalTree (struct)
//      --> ComputeLocalTree (filter funct)
//        --> ComputeLocalTreeImpl (filter funct)
// - PostExecute
//   --> DoPostExecute
//-----------------------------------------------------------------------------
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


template <typename DerivedPolicy>
vtkm::cont::PartitionedDataSet ContourTreeUniformDistributed::PrepareForExecution(
  const vtkm::cont::PartitionedDataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  // Time execution
  vtkm::cont::Timer timer;
  timer.Start();

  // Compute the local contour tree, boundary tree, and interior forest for each local data block
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

  // Log sizes of the local contour trees, boundary trees, and interior forests
  for (size_t bi = 0; bi < this->LocalContourTrees.size(); bi++)
  {
    VTKM_LOG_S(this->TreeLogLevel,
               std::endl
                 << "    ---------------- Contour Tree Array Sizes ---------------------"
                 << std::endl
                 << "    Block Index : " << bi << std::endl
                 << LocalContourTrees[bi].PrintArraySizes());
    VTKM_LOG_S(this->TreeLogLevel,
               std::endl
                 << "    ---------------- Boundary Tree Array Sizes ---------------------"
                 << std::endl
                 << "    Block Index : " << bi << std::endl
                 << LocalBoundaryTrees[bi].PrintArraySizes());
    VTKM_LOG_S(this->TreeLogLevel,
               std::endl
                 << "    ---------------- Interior Forest Array Sizes ---------------------"
                 << std::endl
                 << "    Block Index : " << bi << std::endl
                 << LocalInteriorForests[bi].PrintArraySizes());
    // VTKM_LOG_S(this->TreeLogLevel,
    //           std::endl
    //           << "    ---------------- Hyperstructure Statistics ---------------------"  << std::endl
    //           << LocalContourTrees[bi].PrintHyperStructureStatistics(false) << std::endl);
  }

  // Log timing statistics
  VTKM_LOG_S(this->TimingsLogLevel,
             std::endl
               << "    " << std::setw(38) << std::left << "Contour Tree Filter PrepareForExecution"
               << ": " << timer.GetElapsedTime() << " seconds");

  return input; // TODO/FIXME: What to return?
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

  VTKM_LOG_S(this->TimingsLogLevel,
             std::endl
               << "    " << std::setw(38) << std::left << "Contour Tree Filter PostExecute"
               << ": " << timer.GetElapsedTime() << " seconds");
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
  vtkm::cont::Timer timer;
  timer.Start();
  std::stringstream timingsStream;

  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id size = comm.size();
  vtkm::Id rank = comm.rank();

  // 1. Fan in to compute the hiearchical contour tree
  // 1.1 Setup the block data for DIY
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
    vtkm::cont::ArrayCopy(currField.GetData(), fieldData);

    // ... compute and store the actual mesh
    localDataBlocks[bi]->ContourTreeMeshes.emplace_back(this->LocalBoundaryTrees[bi].VertexIndex,
                                                        this->LocalBoundaryTrees[bi].Superarcs,
                                                        sortOrder,
                                                        fieldData,
                                                        localGlobalMeshIndex);
  } // for

  // Record time for setting block data
  timingsStream << "    " << std::setw(38) << std::left << "Compute Block Data for Fan In"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // ... save for debugging in text and .gv/.dot format. We could do this in the loop above,
  //     but in order to separate timing we do this here and the extra loop over the partitions
  //     should not be significnatly more expensive then doing it all in one loop
  if (this->SaveDotFiles)
  {
    for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
    {
      // save the contour tree mesh
      std::string contourTreeMeshFileName = std::string("Rank_") +
        std::to_string(static_cast<int>(rank)) + std::string("_Block_") +
        std::to_string(static_cast<int>(bi)) + std::string("_Initial_Step_3_BRACT_Mesh.txt");
      localDataBlocks[bi]->ContourTreeMeshes.back().Save(contourTreeMeshFileName.c_str());

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
    } // for
  }   // // if(SaveDotFiles)

  // Record time for saving debug data
  timingsStream << "    " << std::setw(38) << std::left << "Save block data for debug"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // 1.2 Setup vtkmdiy to do global binary reduction of neighbouring blocks.
  //     See also RecuctionOperation struct for example
  // Create the vtkmdiy master
  vtkmdiy::Master master(comm,
                         1, // Use 1 thread, VTK-M will do the treading
                         -1 // All block in memory
  );

  // Record time for creating the DIY master
  timingsStream << "    " << std::setw(38) << std::left << "Create DIY Master"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // 1.2.1 Compute the gids for our local blocks
  using RegularDecomposer = vtkmdiy::RegularDecomposer<vtkmdiy::DiscreteBounds>;
  const vtkm::worklet::contourtree_distributed::SpatialDecomposition& spatialDecomp =
    this->MultiBlockSpatialDecomposition;
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

  // Record time to compute the local block ids
  timingsStream << "    " << std::setw(38) << std::left << "Compute Block Ids and Local Links"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // 1.2.2 Add my local blocks to the vtkmdiy master.
  for (std::size_t bi = 0; bi < static_cast<std::size_t>(input.GetNumberOfPartitions()); bi++)
  {
    master.add(static_cast<int>(vtkmdiyLocalBlockGids[bi]), // block id
               localDataBlocks[bi],
               new vtkmdiy::Link); // Use dummy link to make DIY happy.
    // NOTE: The dummy link is never used, since all communication is via RegularDecomposer,
    //       which sets up its own links
    // NOTE: No need to keep the pointer, as DIY will "own" it and delete it when no longer
    //       needed TODO/FIXME: Confirm that last statement
  }

  // Record time for dding data blocks to the master
  timingsStream << "    " << std::setw(38) << std::left << "Add Data Blocks to DIY"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // 1.2.3 Define the decomposition of the domain into regular blocks
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

  // Record time for creating the decomposer and assigner
  timingsStream << "    " << std::setw(38) << std::left << "Create DIY Decomposer and Assigner"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // 1.2.4  Fix the vtkmdiy links.
  vtkmdiy::fix_links(master, assigner);

  // Record time to fix the links
  timingsStream << "    " << std::setw(38) << std::left << "Fix DIY Links"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // partners for merge over regular block grid
  vtkmdiy::RegularSwapPartners partners(
    decomposer, // domain decomposition
    2,          // radix of k-ary reduction.
    true        // contiguous: true=distance doubling, false=distance halving
  );

  // Record time to create the swap partners
  timingsStream << "    " << std::setw(38) << std::left << "Create DIY Swap Partners"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // 1.3 execute the fan in reduction
  const vtkm::worklet::contourtree_distributed::ComputeDistributedContourTreeFunctor<FieldType>
    computeDistributedContourTreeFunctor(this->MultiBlockSpatialDecomposition.GlobalSize,
                                         this->UseBoundaryExtremaOnly,
                                         this->TimingsLogLevel,
                                         this->TreeLogLevel);
  vtkmdiy::reduce(master, assigner, partners, computeDistributedContourTreeFunctor);

  // Record timing for the actual reduction
  timingsStream << "    " << std::setw(38) << std::left << "Fan In Reduction"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // Be safe! that the Fan In is completed on all blocks and ranks
  comm.barrier();

  timingsStream << "    " << std::setw(38) << std::left << "Post Fan In Barrier"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;
  timer.Start();

  // 2. Fan out to update all the tree
  // 2.1  DataSets for creating output data
  std::vector<vtkm::cont::DataSet> hierarchicalTreeOutputDataSet(localDataBlocks.size());
  // 2.2. Use foreach to compute the fan-out
  master.foreach (
    [&](
      vtkm::worklet::contourtree_distributed::DistributedContourTreeBlockData<FieldType>* blockData,
      const vtkmdiy::Master::ProxyWithLink&) {
#ifdef DEBUG_PRINT_CTUD
      // Save the contour tree, contour tree meshes, and interior forest data for debugging
      vtkm::filter::contourtree_distributed_detail::SaveAfterFanInResults(
        blockData, rank, this->TreeLogLevel);
#endif
      vtkm::cont::Timer iterationTimer;
      iterationTimer.Start();
      std::stringstream fanoutTimingsStream;

      // Fan out
      auto nRounds = blockData->ContourTrees.size() - 1;

      vtkm::worklet::contourtree_distributed::HierarchicalContourTree<FieldType> hierarchicalTree;
      hierarchicalTree.Initialize(static_cast<vtkm::Id>(nRounds),
                                  blockData->ContourTrees[nRounds],
                                  blockData->ContourTreeMeshes[nRounds - 1]);

      // save the corresponding .gv file
      if (this->SaveDotFiles)
      {
        vtkm::filter::contourtree_distributed_detail::SaveHierarchicalTreeDot(
          blockData, hierarchicalTree, rank, nRounds);
      } // if(this->SaveDotFiles)

      fanoutTimingsStream << "    Fan Out Init Hierarchical Tree (block=" << blockData->BlockIndex
                          << ") : " << iterationTimer.GetElapsedTime() << " seconds" << std::endl;
      iterationTimer.Start();

      for (auto round = nRounds - 1; round > 0; round--)
      {
        iterationTimer.Start();
        vtkm::worklet::contourtree_distributed::
          TreeGrafter<vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType>, FieldType>
            grafter(&(blockData->ContourTreeMeshes[round - 1]),
                    blockData->ContourTrees[round],
                    &(blockData->InteriorForests[round]));
        grafter.GraftInteriorForests(static_cast<vtkm::Id>(round),
                                     hierarchicalTree,
                                     blockData->ContourTreeMeshes[round - 1].SortedValues);
        // save the corresponding .gv file
        if (this->SaveDotFiles)
        {
          vtkm::filter::contourtree_distributed_detail::SaveHierarchicalTreeDot(
            blockData, hierarchicalTree, rank, nRounds);
        } // if(this->SaveDotFiles)
        // Log the time for each of the iterations of the fan out loop
        fanoutTimingsStream << "    Fan Out Time (block=" << blockData->BlockIndex
                            << " , round=" << round << ") : " << iterationTimer.GetElapsedTime()
                            << " seconds" << std::endl;
      } // for

      // bottom level
      iterationTimer.Start();
      vtkm::worklet::contourtree_distributed::
        TreeGrafter<vtkm::worklet::contourtree_augmented::DataSetMesh, FieldType>
          grafter(&(this->LocalMeshes[static_cast<std::size_t>(blockData->BlockIndex)]),
                  blockData->ContourTrees[0],
                  &(blockData->InteriorForests[0]));
      auto currBlock = input.GetPartition(blockData->BlockIndex);
      auto currField =
        currBlock.GetField(this->GetActiveFieldName(), this->GetActiveFieldAssociation());
      vtkm::cont::ArrayHandle<FieldType> fieldData;
      vtkm::cont::ArrayCopy(currField.GetData(), fieldData);
      auto localToGlobalIdRelabeler = vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler(
        this->MultiBlockSpatialDecomposition.LocalBlockOrigins.ReadPortal().Get(
          blockData->BlockIndex),
        this->MultiBlockSpatialDecomposition.LocalBlockSizes.ReadPortal().Get(
          blockData->BlockIndex),
        this->MultiBlockSpatialDecomposition.GlobalSize);
      grafter.GraftInteriorForests(0, hierarchicalTree, fieldData, &localToGlobalIdRelabeler);

      // Log the time for each of the iterations of the fan out loop
      fanoutTimingsStream << "    Fan Out Time (block=" << blockData->BlockIndex << " , round=" << 0
                          << ") : " << iterationTimer.GetElapsedTime() << " seconds" << std::endl;
      iterationTimer.Start();

      // Create data set from output
      vtkm::cont::Field dataValuesField(
        "DataValues", vtkm::cont::Field::Association::WHOLE_MESH, hierarchicalTree.DataValues);
      hierarchicalTreeOutputDataSet[blockData->BlockIndex].AddField(dataValuesField);
      vtkm::cont::Field regularNodeGlobalIdsField("RegularNodeGlobalIds",
                                                  vtkm::cont::Field::Association::WHOLE_MESH,
                                                  hierarchicalTree.RegularNodeGlobalIds);
      hierarchicalTreeOutputDataSet[blockData->BlockIndex].AddField(regularNodeGlobalIdsField);
      vtkm::cont::Field superarcsField(
        "Superarcs", vtkm::cont::Field::Association::WHOLE_MESH, hierarchicalTree.Superarcs);
      hierarchicalTreeOutputDataSet[blockData->BlockIndex].AddField(superarcsField);
      vtkm::cont::Field supernodesField(
        "Supernodes", vtkm::cont::Field::Association::WHOLE_MESH, hierarchicalTree.Supernodes);
      hierarchicalTreeOutputDataSet[blockData->BlockIndex].AddField(supernodesField);
      vtkm::cont::Field superparentsField(
        "Superparents", vtkm::cont::Field::Association::WHOLE_MESH, hierarchicalTree.Superparents);
      hierarchicalTreeOutputDataSet[blockData->BlockIndex].AddField(superparentsField);

      // Copy cell set from input data set. This is mainly to ensure that the output data set
      // has a defined cell set. Without one, serialization for DIY does not work properly.
      // Having the extents of the input data set may also help in other use cases.
      hierarchicalTreeOutputDataSet[blockData->BlockIndex].SetCellSet(
        input.GetPartition(blockData->BlockIndex).GetCellSet());

      // Log the time for each of the iterations of the fan out loop
      fanoutTimingsStream << "    Fan Out Create Output Dataset (block=" << blockData->BlockIndex
                          << ") : " << iterationTimer.GetElapsedTime() << " seconds" << std::endl;
      iterationTimer.Start();

      // save the corresponding .gv file
      if (this->SaveDotFiles)
      {
        vtkm::filter::contourtree_distributed_detail::SaveHierarchicalTreeDot(
          blockData, hierarchicalTree, rank, nRounds);

        fanoutTimingsStream << "    Fan Out Save Dot (block=" << blockData->BlockIndex
                            << ") : " << iterationTimer.GetElapsedTime() << " seconds" << std::endl;
        iterationTimer.Start();
      } // if(this->SaveDotFiles)

      // Log the timing stats we collected
      VTKM_LOG_S(this->TimingsLogLevel,
                 std::endl
                   << "    ------------ Fan Out (block=" << blockData->BlockIndex
                   << ")  ------------" << std::endl
                   << fanoutTimingsStream.str());

      // Log the stats from the hierarchical contour tree
      VTKM_LOG_S(this->TreeLogLevel,
                 std::endl
                   << "    ------------ Hierarchical Tree Construction Stats ------------"
                   << std::endl
                   << std::setw(42) << std::left << "    BlockIndex"
                   << ": " << blockData->BlockIndex << std::endl
                   << hierarchicalTree.PrintTreeStats() << std::endl);
    }); // master.foreach

  // Clean-up
  for (auto block : localDataBlocks)
    delete block;

  // 2.2 Log timings for fan out
  timingsStream << "    " << std::setw(38) << std::left << "Fan Out Foreach"
                << ": " << timer.GetElapsedTime() << " seconds" << std::endl;

  VTKM_LOG_S(this->TimingsLogLevel,
             std::endl
               << "    ------------ DoPostExecute Timings ------------" << std::endl
               << timingsStream.str());

  result = vtkm::cont::PartitionedDataSet(hierarchicalTreeOutputDataSet);
} // DoPostExecute

} // namespace filter
} // namespace vtkm::filter

#endif
