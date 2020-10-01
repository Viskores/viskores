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

#ifndef vtk_m_worklet_contourtree_distributed_computedistributedcontourtreefunctor_h
#define vtk_m_worklet_contourtree_distributed_computedistributedcontourtreefunctor_h

#include <vtkm/Types.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_distributed/DistributedContourTreeBlockData.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/diy.h>
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace vtkm
{
namespace worklet
{
namespace contourtree_distributed
{

// Functor needed so we can discover the FieldType and DeviceAdapter template parameters to call MergeWith
struct MergeContourTreeMeshFunctor
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
class ComputeDistributedContourTreeFunctor
{
public:
  ComputeDistributedContourTreeFunctor(vtkm::Id3 globalSize)
    : GlobalSize(globalSize)
  {
  }

  void operator()(
    vtkm::worklet::contourtree_distributed::DistributedContourTreeBlockData<FieldType>*
      block,                            // local Block.
    const vtkmdiy::ReduceProxy& rp,     // communication proxy
    const vtkmdiy::RegularSwapPartners& // partners of the current block (unused)
  ) const
  {
    const auto selfid = rp.gid();

#ifdef DEBUG_PRINT_CTUD
    // Get rank (for debug output only)
    const vtkm::Id rank = vtkm::cont::EnvironmentTracker::GetCommunicator().rank();
#endif

    // Here we do the deque first before the send due to the way the iteration is handled in DIY, i.e., in each iteration
    // A block needs to first collect the data from its neighours and then send the combined block to its neighbours
    // for the next iteration.
    // 1. dequeue the block and compute the new contour tree and contour tree mesh for the block if we have the hight GID
    std::vector<int> incoming;
    rp.incoming(incoming);
    //std::cout << "Incoming size is " << incoming.size() << std::endl;
    for (const int ingid : incoming)
    {
      // NOTE/IMPORTANT: In each round we should have only one swap partner (despite for-loop here).
      // If that assumption does not hold, it will break things.
      if (ingid != selfid)
      {
        vtkm::Id3 otherBlockOrigin;
        rp.dequeue(ingid, otherBlockOrigin);
        vtkm::Id3 otherBlockSize;
        rp.dequeue(ingid, otherBlockSize);
        vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType> otherContourTreeMesh;
        rp.dequeue(ingid, otherContourTreeMesh);

#ifdef DEBUG_PRINT_CTUD
        char buffer[256];
#if 0
          // FIXME: Delete after debugging
          std::cout << "Our block has extents: " << block->BlockOrigin << " " << block->BlockSize << std::endl;
          std::cout << "Received block from " << ingid << " with extents: " << otherBlockOrigin << " " << otherBlockSize << std::endl;

          std::cout << "=================== BEFORE COMBINING =====================" << std::endl;
          block->ContourTreeMeshes.back().DebugPrint("OUR CTM", __FILE__, __LINE__);
          otherContourTreeMesh.DebugPrint("OTHER CTM", __FILE__, __LINE__);
#endif
#if 0
          std::snprintf(buffer, sizeof(buffer), "BeforeCombine_MyMesh_ContourTreeMesh_Rank%lld_Round%d.txt", rank, rp.round());
          block->ContourTreeMeshes.back().Save(buffer);
          std::snprintf(buffer, sizeof(buffer), "BeforeCombine_Other_ContourTreeMesh_Rank%lld_Round%d.txt", rank, rp.round());
          otherContourTreeMesh.Save(buffer);
#endif
#endif

        // Merge the two contour tree meshes
        vtkm::cont::TryExecute(
          MergeContourTreeMeshFunctor{}, otherContourTreeMesh, block->ContourTreeMeshes.back());

#ifdef DEBUG_PRINT_CTUD
        std::snprintf(buffer,
                      sizeof(buffer),
                      "AfterCombine_ContourTreeMesh_Rank%d_Block%d_Round%d.txt",
                      static_cast<int>(rank),
                      static_cast<int>(block->BlockIndex),
                      rp.round());
        block->ContourTreeMeshes.back().Save(buffer);
#if 0
          // FIXME: Delete after debugging
          std::cout << "================== AFTER COMBINING =================" << std::endl;
          std::cout << "OUR CTM" << std::endl;
          block->ContourTreeMeshes.back().DebugPrint("OUR CTM", __FILE__, __LINE__);
#endif
#endif

        // Compute the origin and size of the new block
        vtkm::Id3 currBlockOrigin{
          std::min(otherBlockOrigin[0], block->BlockOrigin[0]),
          std::min(otherBlockOrigin[1], block->BlockOrigin[1]),
          std::min(otherBlockOrigin[2], block->BlockOrigin[2]),
        };
        vtkm::Id3 currBlockMaxIndex{ // Needed only to compute the block size
                                     std::max(otherBlockOrigin[0] + otherBlockSize[0],
                                              block->BlockOrigin[0] + block->BlockSize[0]),
                                     std::max(otherBlockOrigin[1] + otherBlockSize[1],
                                              block->BlockOrigin[1] + block->BlockSize[1]),
                                     std::max(otherBlockOrigin[2] + otherBlockSize[2],
                                              block->BlockOrigin[2] + block->BlockSize[2])
        };
        vtkm::Id3 currBlockSize{ currBlockMaxIndex[0] - currBlockOrigin[0],
                                 currBlockMaxIndex[1] - currBlockOrigin[1],
                                 currBlockMaxIndex[2] - currBlockOrigin[2] };

        // Compute the contour tree from our merged mesh
        vtkm::Id currNumIterations;
        block->ContourTrees.emplace_back(); // Create new empty contour tree object
        vtkm::worklet::contourtree_augmented::IdArrayType currSortOrder;
        vtkm::worklet::ContourTreeAugmented worklet;
        vtkm::Id3 maxIdx{ currBlockOrigin[0] + currBlockSize[0] - 1,
                          currBlockOrigin[1] + currBlockSize[1] - 1,
                          currBlockOrigin[2] + currBlockSize[2] - 1 };
        auto meshBoundaryExecObj = block->ContourTreeMeshes.back().GetMeshBoundaryExecutionObject(
          this->GlobalSize, currBlockOrigin, maxIdx);
        worklet.Run(block->ContourTreeMeshes.back()
                      .SortedValues, // Unused param. Provide something to keep the API happy
                    block->ContourTreeMeshes.back(),
                    block->ContourTrees.back(),
                    currSortOrder,
                    currNumIterations,
                    1, // Fully augmented
                    meshBoundaryExecObj);
#ifdef DEBUG_PRINT_CTUD
#if 0
          // FIXME: Delete after debugging
          std::cout << "=================== BEGIN: COMBINED CONTOUR TREE =========================" << std::endl;
          block->ContourTrees.back().PrintContent();
          std::cout << "=================== END: COMBINED CONTOUR TREE =========================" << std::endl;
#endif
#endif

        // Update block extents
        block->BlockOrigin = currBlockOrigin;
        block->BlockSize = currBlockSize;
      }
    }

    // If we are not in the first round (contour tree mesh for that round was pre-computed
    // in filter outside functor) and if we are sending to someone else (i.e., not in
    // last round) then compute contour tree mesh to send and save it.
    if (rp.round() != 0 && rp.out_link().size() != 0)
    {
#ifdef DEBUG_PRINT_CTUD
      char buffer[256];
#if 0
        std::snprintf(buffer, sizeof(buffer), "CombinedMeshes_GID%d_Round%d.txt", selfid, rp.round());
        block->ContourTreeMeshes.back().Save(buffer);
#endif
#endif

      vtkm::Id3 maxIdx{ block->BlockOrigin[0] + block->BlockSize[0] - 1,
                        block->BlockOrigin[1] + block->BlockSize[1] - 1,
                        block->BlockOrigin[2] + block->BlockSize[2] - 1 };

#ifdef DEBUG_PRINT_CTUD
      std::snprintf(buffer,
                    sizeof(buffer),
                    "BRACTInputCTM_Rank%d_Block%d_Round%d.ctm_txt",
                    static_cast<int>(rank),
                    static_cast<int>(block->BlockIndex),
                    static_cast<int>(block->ContourTreeMeshes.size() - 1));
      block->ContourTreeMeshes.back().Save(buffer);
      std::snprintf(buffer,
                    sizeof(buffer),
                    "BRACTComputation_Rank%d_Block%d_Round%d.txt",
                    static_cast<int>(rank),
                    static_cast<int>(block->BlockIndex),
                    rp.round());
      std::ofstream os(buffer);
      os << "Block Origin: " << block->BlockOrigin << " Block Size: " << block->BlockSize
         << " Block MaxIdx: " << maxIdx << std::endl;
      os << "================= INPUT ===================" << std::endl;
      os << "+++++++++++++++++ Contour Tree +++++++++++++++++" << std::endl;
      block->ContourTrees.back().PrintContent(os);
      os << "+++++++++++++++++ Contour Tree Mesh +++++++++++++++++" << std::endl;
      block->ContourTreeMeshes.back().PrintContent(os);

#if 0
        // TODO: GET THIS COMPILING
        // save the corresponding .gv file for the contour tree mesh
        std::string contourTreeMeshFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(block->BlockIndex)) + "_Round_" + std::to_string(rp.round()) + "_Partner_" + std::to_string(ingid) + std::string("_Step_0_Combined_Mesh.gv");
        std::ofstream contourTreeMeshFile(contourTreeMeshFileName);
        contourTreeMeshFile << vtkm::worklet::contourtree_distributed::ContourTreeMeshDotGraphPrint<FieldType>
          (std::string("Block ") + std::to_string(static_cast<int>(block->BlockIndex)) + " Round " + std::to_string(rp.round()) + " Partner " + std::to_string(ingid) + std::string(" Step 0 Combined Mesh"),
                block->ContourTreeMeshes.back(),  worklet::contourtree_distributed::SHOW_CONTOUR_TREE_MESH_ALL);

        // and the ones for the contour tree regular and superstructures
        std::string regularStructureFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(block->BlockIndex)) + "_Round_" + std::to_string(rp.round()) + " Partner " + std::to_string(ingid) + std::string("_Step_1_Contour_Tree_Regular_Structure.gv");
        std::ofstream regularStructureFile(regularStructureFileName);
        regularStructureFile << worklet::contourtree_distributed::ContourTreeDotGraphPrint<T, MeshType, vtkm::worklet::contourtree_augmented::IdArrayType()
          (std::string("Block ") + std::to_string(static_cast<int>(block->BlockIndex)) + " Round " + std::to_string(rp.round()) + " Partner " + std::to_string(ingid) + std::string(" Step 1 Contour Tree Regular Structure"),
                block->Meshes.back(),
                block->ContourTrees.back(),
                worklet::contourtree_distributed::SHOW_REGULAR_STRUCTURE|worklet::contourtree_distributed::SHOW_ALL_IDS);

        std::string superStructureFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(block->BlockIndex)) + "_Round_" + std::to_string(rp.round()) + " Partner " + std::to_string(ingid) + std::string("_Step_2_Contour_Tree_Super_Structure.gv");
        std::ofstream superStructureFile(superStructureFileName);
        superStructureFile << worklet::contourtree_distributed::ContourTreeDotGraphPrint<T, MeshType, vtkm::worklet::contourtree_augmented::IdArrayType()
          (std::string("Block ") + std::to_string(static_cast<int>(block->BlockIndex)) + " Round " + std::to_string(rp.round()) + " Partner " + std::to_string(ingid) + std::string(" Step 2 Contour Tree Super Structure"),
                block->Meshes.back(),
                block->ContourTrees.back(),
                worklet::contourtree_distributed::SHOW_SUPER_STRUCTURE|worklet::contourtree_distributed::SHOW_HYPER_STRUCTURE|worklet::contourtree_distributed::SHOW_ALL_IDS|worklet::contourtree_distributed::SHOW_ALL_SUPERIDS|worklet::contourtree_distributed::SHOW_ALL_HYPERIDS);
#endif
#endif

      // Compute BRACT
      vtkm::worklet::contourtree_distributed::BoundaryTree boundaryTree;
      // ... Get the mesh boundary object
      auto meshBoundaryExecObj = block->ContourTreeMeshes.back().GetMeshBoundaryExecutionObject(
        this->GlobalSize, block->BlockOrigin, maxIdx);
      // Make the BRACT and InteriorForest (i.e., residue)
      block->InteriorForests.emplace_back();
      auto boundaryTreeMaker = vtkm::worklet::contourtree_distributed::BoundaryTreeMaker<
        vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType>,
        vtkm::worklet::contourtree_augmented::MeshBoundaryContourTreeMeshExec>(
        &(block->ContourTreeMeshes.back()),
        meshBoundaryExecObj,
        block->ContourTrees.back(),
        &boundaryTree,
        &(block->InteriorForests.back()));
      // Construct the BRACT and InteriorForest. Since we are working on a ContourTreeMesh we do
      // not need to provide and IdRelabeler here in order to compute the InteriorForest
      boundaryTreeMaker.Construct();
#ifdef DEBUG_PRINT_CTUD
      os << "================= OUTPUT ===================" << std::endl;
      os << "+++++++++++++++++ BRACT +++++++++++++++++++" << std::endl;
      os << boundaryTree.DebugPrint("validate", __FILE__, __LINE__);
      os << "+++++++++++++++++ BRACT Contour Tree Mesh +++++++++++++++++++" << std::endl;

      //char buffer[256];
      std::snprintf(buffer,
                    sizeof(buffer),
                    "GID %d, Round %d, Block %d, Computed by BRACTMaker",
                    selfid,
                    rp.round(),
                    static_cast<int>(block->BlockIndex));
      std::string debug_dot = boundaryTree.PrintGlobalDot(buffer, block->ContourTreeMeshes.back());
      std::snprintf(buffer,
                    sizeof(buffer),
                    "BRACT_Rank%d__Block%d_Round%d.gv",
                    static_cast<int>(rank),
                    static_cast<int>(block->BlockIndex),
                    rp.round());
      std::ofstream dotStream(buffer);
      dotStream << debug_dot;
#endif
      // Construct contour tree mesh from BRACT
      block->ContourTreeMeshes.emplace_back(
        boundaryTree.VertexIndex, boundaryTree.Superarcs, block->ContourTreeMeshes.back());

#ifdef DEBUG_PRINT_CTUD
      block->ContourTreeMeshes.back().PrintContent(os);
      //char buffer[256];
      std::snprintf(buffer,
                    sizeof(buffer),
                    "CombinedMeshes_Rank%d_Block%d_Round%lu.ctm_txt",
                    static_cast<int>(rank),
                    static_cast<int>(block->BlockIndex),
                    block->ContourTreeMeshes.size() - 1);
      block->ContourTreeMeshes.back().Save(buffer);

#if 0
        // TODO: GET THIS COMPILING
        // save the Boundary Tree as a dot file
        std::string boundaryTreeFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(block->BlockIndex)) + "_Round_" + std::to_string(rp.round()) + "_Partner_" + std::to_string(ingid) + std::string("_Step_3_Boundary_Tree.gv");
        std::ofstream boundaryTreeFile(boundaryTreeFileName);
        boundaryTreeFile << vtkm::worklet::contourtree_distributed::BoundaryTreeDotGraphPrint
          (std::string("Block ") + std::to_string(static_cast<int>(block->BlockIndex)) + " Round " + std::to_string(rp.round()) + " Partner " + std::to_string(ingid) + std::string(" Step 3 Boundary Tree"),
                block->Meshes.back()],
                block->BoundaryTrees.back());

        // and save the Interior Forest as another dot file
        std::string interiorForestFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(block->BlockIndex)) + "_Round_" + std::to_string(rp.round()) + "_Partner_" + std::to_string(ingid) + std::string("_Step_4_Interior_Forest.gv");
        std::ofstream interiorForestFile(interiorForestFileName);
        interiorForestFileName << InteriorForestDotGraphPrintFile<MeshType>
          (std::string("Block ") + std::to_string(static_cast<int>(block->BlockIndex)) + " Round " + std::to_string(rp.round()) + " Partner " + std::to_string(ingid) + std::string(" Step 4 Interior Forest"),
                block->InteriorForests.back(),
                block->ContourTrees.back(),
                block->BoundaryTrees.back(),
                block->Meshes.back());

        // save the corresponding .gv file
        std::string boundaryTreeMeshFileName = std::string("Rank_") + std::to_string(static_cast<int>(rank)) + std::string("_Block_") + std::to_string(static_cast<int>(block->BlockIndex)) + "_Round_" + std::to_string(rp.round()) + "_Partner_" + std::to_string(ingid) + std::string("_Step_5_Boundary_Tree_Mesh.gv");
        std::ofstream boundaryTreeMeshFile(boundaryTreeMeshFileName);
        boundaryTreeMeshFile << vtkm::worklet::contourtree_distributed::ContourTreeMeshDotGraphPrint<FieldType>
          (std::string("Block ") + std::to_string(static_cast<int>(block->BlockIndex)) + " Round " + std::to_string(rp.round()) + " Partner " + std::to_string(ingid) + std::string(" Step 5 Boundary Tree Mesh"),
                block->ContourTreeMeshes.back(),
                worklet::contourtree_distributed::SHOW_CONTOUR_TREE_MESH_ALL);
#endif
#endif
    }

    // Send our current block (which is either our original block or the one we just combined from the ones we received) to our next neighbour.
    // Once a rank has send his block (either in its orignal or merged form) it is done with the reduce
    for (int cc = 0; cc < rp.out_link().size(); ++cc)
    {
      auto target = rp.out_link().target(cc);
      if (target.gid != selfid)
      {
        rp.enqueue(target, block->BlockOrigin);
        rp.enqueue(target, block->BlockSize);
        rp.enqueue(target, block->ContourTreeMeshes.back());
      }
    }
  } //end ComputeDistributedContourTreeFunctor

private:
  vtkm::Id3 GlobalSize; // Extends of the global mesh
};


} // namespace contourtree_distributed
} // namespace worklet
} // namespace vtkm

#endif
