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

// #define DEBUG_PRINT
// #define PRINT_RESULT

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/contourtree_augmented/DataSetMesh.h>
#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>
#include <vtkm/worklet/contourtree_distributed/CombineHyperSweepBlockFunctor.h>
#include <vtkm/worklet/contourtree_distributed/HierarchicalContourTree.h>
#include <vtkm/worklet/contourtree_distributed/HierarchicalHyperSweeper.h>
#include <vtkm/worklet/contourtree_distributed/HyperSweepBlock.h>
#include <vtkm/worklet/contourtree_distributed/SpatialDecomposition.h>
#include <vtkm/worklet/testing/TestingContourTreeUniformDistributedLoadArrays.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/diy.h>
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace
{
template <typename FieldType>
void LoadHierarchicalContourTree(
  const char* filename,
  vtkm::worklet::contourtree_distributed::HierarchicalContourTree<FieldType>& ht)
{
  using vtkm::worklet::testing::contourtree_distributed::ReadIndexArray;
  using vtkm::worklet::testing::contourtree_distributed::ReadIndexArrayVector;
  //template <typename FieldType>
  using vtkm::worklet::testing::contourtree_distributed::ReadDataArray; //<FieldType>;

  std::ifstream is(filename, std::ios_base::binary);
  ReadIndexArray(is, ht.RegularNodeGlobalIds);
  ReadDataArray<FieldType>(is, ht.DataValues);
  ReadIndexArray(is, ht.RegularNodeSortOrder);
  ReadIndexArray(is, ht.Regular2Supernode);
  ReadIndexArray(is, ht.Superparents);
  ReadIndexArray(is, ht.Supernodes);
  ReadIndexArray(is, ht.Superarcs);
  ReadIndexArray(is, ht.Hyperparents);
  ReadIndexArray(is, ht.Super2Hypernode);
  ReadIndexArray(is, ht.WhichRound);
  ReadIndexArray(is, ht.WhichIteration);
  ReadIndexArray(is, ht.Hypernodes);
  ReadIndexArray(is, ht.Hyperarcs);
  ReadIndexArray(is, ht.Superchildren);
  int nRounds;
  is.read(reinterpret_cast<char*>(&nRounds), sizeof(nRounds));
  //std::cout << "nRounds = " << nRounds << std::endl;
  ht.NumRounds = nRounds;
  //ht.NumOwnedRegularVertices = 0;
  ReadIndexArray(is, ht.NumRegularNodesInRound);
  ReadIndexArray(is, ht.NumSupernodesInRound);
  ReadIndexArray(is, ht.NumHypernodesInRound);
  ReadIndexArray(is, ht.NumIterations);
  ReadIndexArrayVector(is, ht.FirstSupernodePerIteration);
  ReadIndexArrayVector(is, ht.FirstHypernodePerIteration);
}

template <typename FieldType>
void TestContourTreeMeshCombine(const std::string& mesh1_filename,
                                const std::string& mesh2_filename,
                                const std::string& combined_filename)
{
  std::cout << "Testing combining meshes " << mesh1_filename << " " << mesh2_filename
            << " with expected result " << combined_filename << std::endl;

  vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType> contourTreeMesh1;
  contourTreeMesh1.Load(mesh1_filename.c_str());
  vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType> contourTreeMesh2;
  contourTreeMesh2.Load(mesh2_filename.c_str());
  contourTreeMesh2.MergeWith(contourTreeMesh1);
  // Result is written to contourTreeMesh2
  vtkm::worklet::contourtree_augmented::ContourTreeMesh<FieldType> combinedContourTreeMesh;
  combinedContourTreeMesh.Load(combined_filename.c_str());
  VTKM_TEST_ASSERT(
    test_equal_ArrayHandles(contourTreeMesh2.SortedValues, combinedContourTreeMesh.SortedValues));
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(contourTreeMesh2.GlobalMeshIndex,
                                           combinedContourTreeMesh.GlobalMeshIndex));
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(contourTreeMesh2.GlobalMeshIndex,
                                           combinedContourTreeMesh.GlobalMeshIndex));
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(contourTreeMesh2.NeighborConnectivity,
                                           combinedContourTreeMesh.NeighborConnectivity));
  VTKM_TEST_ASSERT(test_equal_ArrayHandles(contourTreeMesh2.NeighborOffsets,
                                           combinedContourTreeMesh.NeighborOffsets));
  VTKM_TEST_ASSERT(contourTreeMesh2.NumVertices == combinedContourTreeMesh.NumVertices);
  VTKM_TEST_ASSERT(contourTreeMesh2.MaxNeighbors == combinedContourTreeMesh.MaxNeighbors);
}

void TestHierarchicalHyperSweeper()
{
  std::cout << "Testing HierarchicalHyperSweeper" << std::endl;
  using vtkm::cont::testing::Testing;
  using ContourTreeDataFieldType = vtkm::FloatDefault;

  // Test input
  const int numBlocks = 4;
  const char* filenames[numBlocks] = { "misc/8x9test_HierarchicalAugmentedTree_Block0.dat",
                                       "misc/8x9test_HierarchicalAugmentedTree_Block1.dat",
                                       "misc/8x9test_HierarchicalAugmentedTree_Block2.dat",
                                       "misc/8x9test_HierarchicalAugmentedTree_Block3.dat" };
  vtkm::Id3 globalSize{ 9, 8, 1 };
  vtkm::Id3 blocksPerDim{ 2, 2, 1 };
  vtkm::Id3 sizes[numBlocks] = { { 5, 4, 1 }, { 5, 5, 1 }, { 5, 4, 1 }, { 5, 5, 1 } };
  vtkm::Id3 origins[numBlocks] = { { 0, 0, 0 }, { 0, 3, 0 }, { 4, 0, 0 }, { 4, 3, 0 } };
  vtkm::Id3 blockIndices[numBlocks] = { { 0, 0, 0 }, { 0, 1, 0 }, { 1, 0, 0 }, { 1, 1, 0 } };

  // Expected output
  vtkm::cont::ArrayHandle<vtkm::Id> expectedIntrinsicVolume[numBlocks] = {
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 9, 8, 24, 20, 1, 1 }),
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 9, 8, 24, 20, 1, 1 }),
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 9, 8, 24, 20, 1 }),
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 9, 8, 24, 20, 1, 2 })
  };

  vtkm::cont::ArrayHandle<vtkm::Id> expectedDependentVolume[numBlocks] = {
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 9, 18, 24, 46, 72, 1 }),
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 9, 18, 24, 46, 72, 1 }),
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 9, 18, 24, 46, 72 }),
    vtkm::cont::make_ArrayHandle<vtkm::Id>({ 6, 9, 18, 24, 46, 72, 2 })
  };

  // Create spatial decomposition
  vtkm::worklet::contourtree_distributed::SpatialDecomposition spatialDecomp(
    blocksPerDim,
    globalSize,
    vtkm::cont::make_ArrayHandle(blockIndices, numBlocks, vtkm::CopyFlag::Off),
    vtkm::cont::make_ArrayHandle(origins, numBlocks, vtkm::CopyFlag::Off),
    vtkm::cont::make_ArrayHandle(sizes, numBlocks, vtkm::CopyFlag::Off));

  // Load trees
  vtkm::worklet::contourtree_distributed::HierarchicalContourTree<vtkm::FloatDefault>
    hct[numBlocks];
  for (vtkm::Id blockNo = 0; blockNo < numBlocks; ++blockNo)
  {
    LoadHierarchicalContourTree(Testing::DataPath(filenames[blockNo]).c_str(), hct[blockNo]);
#ifdef DEBUG_PRINT
    std::cout << hct[blockNo].DebugPrint("AfterLoad", __FILE__, __LINE__);
#endif
  }

  // Create and add DIY blocks
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkm::Id rank = comm.rank();

  vtkmdiy::Master master(comm,
                         1, // Use 1 thread, VTK-M will do the treading
                         -1 // All block in memory
  );

  // Set up connectivity
  using RegularDecomposer = vtkmdiy::RegularDecomposer<vtkmdiy::DiscreteBounds>;
  RegularDecomposer::BoolVector shareFace(3, true);
  RegularDecomposer::BoolVector wrap(3, false);
  RegularDecomposer::CoordinateVector ghosts(3, 1);
  RegularDecomposer::DivisionsVector diyDivisions{ 2, 2, 1 }; // HARDCODED FOR TEST

  int numDims = static_cast<int>(globalSize[2] > 1 ? 3 : 2);
  RegularDecomposer decomposer(numDims,
                               spatialDecomp.GetVTKmDIYBounds(),
                               static_cast<int>(spatialDecomp.GetGlobalNumberOfBlocks()),
                               shareFace,
                               wrap,
                               ghosts,
                               diyDivisions);

  // ... coordinates of local blocks
  auto localBlockIndicesPortal = spatialDecomp.LocalBlockIndices.ReadPortal();
  std::vector<int> vtkmdiyLocalBlockGids(numBlocks);
  for (vtkm::Id bi = 0; bi < numBlocks; bi++)
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

  // Define which blocks live on which rank so that vtkmdiy can manage them
  vtkmdiy::DynamicAssigner assigner(
    comm, comm.size(), static_cast<int>(spatialDecomp.GetGlobalNumberOfBlocks()));
  for (vtkm::Id bi = 0; bi < numBlocks; bi++)
  {
    assigner.set_rank(static_cast<int>(rank),
                      static_cast<int>(vtkmdiyLocalBlockGids[static_cast<size_t>(bi)]));
  }
  vtkmdiy::fix_links(master, assigner);

  vtkm::worklet::contourtree_distributed::HyperSweepBlock<ContourTreeDataFieldType>*
    localHyperSweeperBlocks[numBlocks];
  for (vtkm::Id blockNo = 0; blockNo < numBlocks; ++blockNo)
  {
    localHyperSweeperBlocks[blockNo] =
      new vtkm::worklet::contourtree_distributed::HyperSweepBlock<ContourTreeDataFieldType>(
        blockNo,
        vtkmdiyLocalBlockGids[blockNo],
        origins[blockNo],
        sizes[blockNo],
        globalSize,
        hct[blockNo]);
    master.add(
      vtkmdiyLocalBlockGids[blockNo], localHyperSweeperBlocks[blockNo], new vtkmdiy::Link());
  }

  master.foreach (
    [](vtkm::worklet::contourtree_distributed::HyperSweepBlock<ContourTreeDataFieldType>* b,
       const vtkmdiy::Master::ProxyWithLink&) {
#ifdef DEBUG_PRINT
      std::cout << "Block " << b->GlobalBlockId << std::endl;
      std::cout << b->HierarchicalContourTree.DebugPrint(
        "Before initializing HyperSweeper", __FILE__, __LINE__);
#endif
      // Create HyperSweeper
      vtkm::worklet::contourtree_distributed::HierarchicalHyperSweeper<vtkm::Id,
                                                                       ContourTreeDataFieldType>
        hyperSweeper(
          b->GlobalBlockId, b->HierarchicalContourTree, b->IntrinsicVolume, b->DependentVolume);

#ifdef DEBUG_PRINT
      std::cout << "Block " << b->GlobalBlockId << std::endl;
      std::cout << b->HierarchicalContourTree.DebugPrint(
        "After initializing HyperSweeper", __FILE__, __LINE__);
#endif

      // Create mesh and initialize vertex counts
      vtkm::worklet::contourtree_augmented::mesh_dem::IdRelabeler idRelabeler{ b->Origin,
                                                                               b->Size,
                                                                               b->GlobalSize };

      if (b->GlobalSize[2] <= 1)
      {
        vtkm::worklet::contourtree_augmented::DataSetMeshTriangulation2DFreudenthal mesh(
          vtkm::Id2{ b->Size[0], b->Size[1] });
        hyperSweeper.InitializeIntrinsicVertexCount(
          b->HierarchicalContourTree, mesh, idRelabeler, b->IntrinsicVolume);
      }
      else
      {
        // TODO/FIXME: For getting owned vertices, it should not make a difference if marching
        // cubes or not. Verify.
        vtkm::worklet::contourtree_augmented::DataSetMeshTriangulation3DFreudenthal mesh(b->Size);
        hyperSweeper.InitializeIntrinsicVertexCount(
          b->HierarchicalContourTree, mesh, idRelabeler, b->IntrinsicVolume);
      }

#ifdef DEBUG_PRINT
      std::cout << "Block " << b->GlobalBlockId << std::endl;
      std::cout << b->HierarchicalContourTree.DebugPrint(
        "After initializing intrinsic vertex count", __FILE__, __LINE__);
#endif

      // Initialize dependentVolume by copy from intrinsicVolume
      vtkm::cont::Algorithm::Copy(b->IntrinsicVolume, b->DependentVolume);

      // Perform the local hypersweep
      hyperSweeper.LocalHyperSweep();

#ifdef DEBUG_PRINT
      std::cout << "Block " << b->GlobalBlockId << std::endl;
      std::cout << b->HierarchicalContourTree.DebugPrint(
        "After local hypersweep", __FILE__, __LINE__);
#endif
    });

  // Reduce
  // partners for merge over regular block grid
  vtkmdiy::RegularSwapPartners partners(
    decomposer, // domain decomposition
    2,          // radix of k-ary reduction.
    true        // contiguous: true=distance doubling, false=distance halving
  );
  vtkmdiy::reduce(master,
                  assigner,
                  partners,
                  vtkm::worklet::contourtree_distributed::CobmineHyperSweepBlockFunctor<
                    ContourTreeDataFieldType>{});

#ifdef PRINT_RESULT
  // Print
  vtkm::Id totalVolume = globalSize[0] * globalSize[1] * globalSize[2];
  master.foreach (
    [&totalVolume](
      vtkm::worklet::contourtree_distributed::HyperSweepBlock<ContourTreeDataFieldType>* b,
      const vtkmdiy::Master::ProxyWithLink&) {
      std::cout << "Block " << b->GlobalBlockId << std::endl;
      std::cout << "=========" << std::endl;
      vtkm::worklet::contourtree_augmented::PrintHeader(b->IntrinsicVolume.GetNumberOfValues(),
                                                        std::cout);
      vtkm::worklet::contourtree_augmented::PrintIndices(
        "Intrinsic Volume", b->IntrinsicVolume, -1, std::cout);
      vtkm::worklet::contourtree_augmented::PrintIndices(
        "Dependent Volume", b->DependentVolume, -1, std::cout);

      std::cout << b->HierarchicalContourTree.DebugPrint(
        "Called from DumpVolumes", __FILE__, __LINE__);
      std::cout << vtkm::worklet::contourtree_distributed::HierarchicalContourTree<
        ContourTreeDataFieldType>::DumpVolumes(b->HierarchicalContourTree.Supernodes,
                                               b->HierarchicalContourTree.Superarcs,
                                               b->HierarchicalContourTree.RegularNodeGlobalIds,
                                               totalVolume,
                                               b->IntrinsicVolume,
                                               b->DependentVolume);
    });
#endif

  // Compare to expected results
  master.foreach (
    [&expectedIntrinsicVolume, &expectedDependentVolume](
      vtkm::worklet::contourtree_distributed::HyperSweepBlock<ContourTreeDataFieldType>* b,
      const vtkmdiy::Master::ProxyWithLink&) {
#ifdef DEBUG_PRINT
      vtkm::worklet::contourtree_augmented::PrintIndices(
        "Intrinsic Volume", b->IntrinsicVolume, -1, std::cout);
      vtkm::worklet::contourtree_augmented::PrintIndices(
        "Expected Intrinsic Volume", expectedIntrinsicVolume[b->GlobalBlockId], -1, std::cout);
      vtkm::worklet::contourtree_augmented::PrintIndices(
        "Dependent Volume", b->DependentVolume, -1, std::cout);
      vtkm::worklet::contourtree_augmented::PrintIndices(
        "Expected Dependent Volume", expectedDependentVolume[b->GlobalBlockId], -1, std::cout);
#endif
      VTKM_TEST_ASSERT(test_equal_portals(expectedIntrinsicVolume[b->GlobalBlockId].ReadPortal(),
                                          b->IntrinsicVolume.ReadPortal()));
      VTKM_TEST_ASSERT(test_equal_portals(expectedDependentVolume[b->GlobalBlockId].ReadPortal(),
                                          b->DependentVolume.ReadPortal()));
    });

  // Clean-up
  for (auto b : localHyperSweeperBlocks)
  {
    delete b;
  }
}

void TestContourTreeUniformDistributed()
{
  using vtkm::cont::testing::Testing;
  TestContourTreeMeshCombine<vtkm::FloatDefault>(
    Testing::DataPath("misc/5x6_7_MC_Rank0_Block0_Round1_BeforeCombineMesh1.ctm"),
    Testing::DataPath("misc/5x6_7_MC_Rank0_Block0_Round1_BeforeCombineMesh2.ctm"),
    Testing::RegressionImagePath("5x6_7_MC_Rank0_Block0_Round1_CombinedMesh.ctm"));

  TestHierarchicalHyperSweeper();
}

} // anonymous namespace

int UnitTestContourTreeUniformDistributed(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourTreeUniformDistributed, argc, argv);
}
