//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

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

#include <viskores/Deprecated.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/MakeTestDataSet.h>

#include <viskores/filter/scalar_topology/ContourTreeUniformAugmented.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ProcessContourTree.h>

#include <set>

#ifdef VISKORES_ENABLE_MPI
#include "TestingContourTreeUniformDistributedFilter.h"
#include <mpi.h>
#endif

namespace
{

using viskores::cont::testing::MakeTestDataSet;

//
//  Test regular single block contour tree construction
//
class TestContourTreeUniformAugmented
{
private:
#ifdef VISKORES_ENABLE_MPI
  void ShiftLogicalOriginToZero(viskores::cont::PartitionedDataSet& pds) const
  {
    // Shift the logical origin (minimum of LocalPointIndexStart) to zero
    // along each dimension

    // Compute minimum global point index start for all data sets on this MPI rank
    std::vector<viskores::Id> minimumGlobalPointIndexStartThisRank;
    using ds_const_iterator = viskores::cont::PartitionedDataSet::const_iterator;
    for (ds_const_iterator ds_it = pds.cbegin(); ds_it != pds.cend(); ++ds_it)
    {
      ds_it->GetCellSet().CastAndCallForTypes<viskores::cont::CellSetListStructured>(
        [&minimumGlobalPointIndexStartThisRank](const auto& css)
        {
          minimumGlobalPointIndexStartThisRank.resize(css.Dimension,
                                                      std::numeric_limits<viskores::Id>::max());
          for (viskores::IdComponent d = 0; d < css.Dimension; ++d)
          {
            minimumGlobalPointIndexStartThisRank[d] =
              std::min(minimumGlobalPointIndexStartThisRank[d], css.GetGlobalPointIndexStart()[d]);
          }
        });
    }

    // Perform global reduction to find GlobalPointDimensions across all ranks
    std::vector<viskores::Id> minimumGlobalPointIndexStart;
    auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
    viskoresdiy::mpi::all_reduce(comm,
                                 minimumGlobalPointIndexStartThisRank,
                                 minimumGlobalPointIndexStart,
                                 viskoresdiy::mpi::minimum<viskores::Id>{});

    // Shift all cell sets so that minimum global point index start
    // along each dimension is zero
    using ds_iterator = viskores::cont::PartitionedDataSet::iterator;
    for (ds_iterator ds_it = pds.begin(); ds_it != pds.end(); ++ds_it)
    {
      // This does not work, i.e., it does not really change the cell set for the DataSet
      ds_it->GetCellSet().CastAndCallForTypes<viskores::cont::CellSetListStructured>(
        [&minimumGlobalPointIndexStart, &ds_it](auto& css)
        {
          auto pointIndexStart = css.GetGlobalPointIndexStart();
          typename std::remove_reference_t<decltype(css)>::SchedulingRangeType
            shiftedPointIndexStart;
          for (viskores::IdComponent d = 0; d < css.Dimension; ++d)
          {
            shiftedPointIndexStart[d] = pointIndexStart[d] - minimumGlobalPointIndexStart[d];
          }
          css.SetGlobalPointIndexStart(shiftedPointIndexStart);
          // Why is the following necessary? Shouldn't it be sufficient to update the
          // CellSet through the reference?
          ds_it->SetCellSet(css);
        });
    }
  }

  void ComputeGlobalPointSize(viskores::cont::PartitionedDataSet& pds) const
  {
    // Compute GlobalPointDimensions as maximum of GlobalPointIndexStart + PointDimensions
    // for each dimension across all blocks

    // Compute GlobalPointDimensions for all data sets on this MPI rank
    std::vector<viskores::Id> globalPointDimensionsThisRank;
    using ds_const_iterator = viskores::cont::PartitionedDataSet::const_iterator;
    for (ds_const_iterator ds_it = pds.cbegin(); ds_it != pds.cend(); ++ds_it)
    {
      ds_it->GetCellSet().CastAndCallForTypes<viskores::cont::CellSetListStructured>(
        [&globalPointDimensionsThisRank](const auto& css)
        {
          globalPointDimensionsThisRank.resize(css.Dimension, -1);
          for (viskores::IdComponent d = 0; d < css.Dimension; ++d)
          {
            globalPointDimensionsThisRank[d] =
              std::max(globalPointDimensionsThisRank[d],
                       css.GetGlobalPointIndexStart()[d] + css.GetPointDimensions()[d]);
          }
        });
    }

    // Perform global reduction to find GlobalPointDimensions across all ranks
    std::vector<viskores::Id> globalPointDimensions;
    auto comm = viskores::cont::EnvironmentTracker::GetCommunicator();
    viskoresdiy::mpi::all_reduce(comm,
                                 globalPointDimensionsThisRank,
                                 globalPointDimensions,
                                 viskoresdiy::mpi::maximum<viskores::Id>{});

    // Set this information in all cell sets
    using ds_iterator = viskores::cont::PartitionedDataSet::iterator;
    for (ds_iterator ds_it = pds.begin(); ds_it != pds.end(); ++ds_it)
    {
      // This does not work, i.e., it does not really change the cell set for the DataSet
      ds_it->GetCellSet().CastAndCallForTypes<viskores::cont::CellSetListStructured>(
        [&globalPointDimensions, &ds_it](auto& css)
        {
          typename std::remove_reference_t<decltype(css)>::SchedulingRangeType gpd;
          for (viskores::IdComponent d = 0; d < css.Dimension; ++d)
          {
            gpd[d] = globalPointDimensions[d];
          }
          css.SetGlobalPointDimensions(gpd);
          // Why is the following necessary? Shouldn't it be sufficient to update the
          // CellSet through the reference?
          ds_it->SetCellSet(css);
        });
    }
  }

  void GetPartitionedDataSet(const viskores::cont::DataSet& ds,
                             const std::string& fieldName,
                             const int numberOfBlocks,
                             const int rank,
                             const int numberOfRanks,
                             viskores::cont::PartitionedDataSet& pds) const
  {
    // Get dimensions of data set
    viskores::Id3 globalSize;
    viskores::cont::CastAndCall(
      ds.GetCellSet(), viskores::worklet::contourtree_augmented::GetPointDimensions(), globalSize);

    // Determine split
    viskores::Id3 blocksPerAxis =
      viskores::filter::testing::contourtree_uniform_distributed::ComputeNumberOfBlocksPerAxis(
        globalSize, numberOfBlocks);
    viskores::Id blocksPerRank = numberOfBlocks / numberOfRanks;
    viskores::Id numRanksWithExtraBlock = numberOfBlocks % numberOfRanks;
    viskores::Id blocksOnThisRank, startBlockNo;

    if (rank < numRanksWithExtraBlock)
    {
      blocksOnThisRank = blocksPerRank + 1;
      startBlockNo = (blocksPerRank + 1) * rank;
    }
    else
    {
      blocksOnThisRank = blocksPerRank;
      startBlockNo = numRanksWithExtraBlock * (blocksPerRank + 1) +
        (rank - numRanksWithExtraBlock) * blocksPerRank;
    }

    // Created partitioned (split) data set
    //viskores::cont::PartitionedDataSet pds;
    viskores::cont::ArrayHandle<viskores::Id3> localBlockIndices;

    localBlockIndices.Allocate(blocksOnThisRank);

    auto localBlockIndicesPortal = localBlockIndices.WritePortal();

    for (viskores::Id blockNo = 0; blockNo < blocksOnThisRank; ++blockNo)
    {
      viskores::Id3 blockOrigin, blockSize, blockIndex;
      std::tie(blockIndex, blockOrigin, blockSize) =
        viskores::filter::testing::contourtree_uniform_distributed::ComputeBlockExtents(
          globalSize, blocksPerAxis, startBlockNo + blockNo);
      pds.AppendPartition(
        viskores::filter::testing::contourtree_uniform_distributed::CreateSubDataSet(
          ds, blockOrigin, blockSize, fieldName));
      localBlockIndicesPortal.Set(blockNo, blockIndex);
    }
  }
#endif

  template <typename DataValueType>
  void analysis(const viskores::cont::DataSet& result,
                const std::string& fieldName,
                const viskores::Id& levels,
                std::vector<DataValueType>& isoValues) const
  {
    namespace caugmented_ns = viskores::worklet::contourtree_augmented;

    DataValueType eps = 0.00001f;
    viskores::Id numComp = levels + 1;
    bool usePersistenceSorter = true;
    int contourType = 0;

    isoValues = caugmented_ns::ProcessContourTree::SelectTopVolumeBranches<DataValueType>(
      result, fieldName, numComp, contourType, eps, usePersistenceSorter);

    std::sort(isoValues.begin(), isoValues.end());
    auto it = std::unique(isoValues.begin(), isoValues.end());
    isoValues.resize(std::distance(isoValues.begin(), it));
  }

#ifdef VISKORES_ENABLE_MPI
  // Legacy getter-based analysis for the deprecated multi-block path. Results are accessed via
  // GetContourTree()/GetNumIterations()/GetSortOrder() rather than from the output DataSet.
  template <typename DataValueType>
  void analysisLegacy(viskores::filter::scalar_topology::ContourTreeAugmented& filter,
                      bool dataFieldIsSorted,
                      const viskores::cont::UnknownArrayHandle& arr,
                      const viskores::Id& levels,
                      std::vector<DataValueType>& isoValues) const
  {
    namespace caugmented_ns = viskores::worklet::contourtree_augmented;

    DataValueType eps = 0.00001f;      // Distance away from critical point
    viskores::Id numComp = levels + 1; // Number of components the tree should be simplified to
    bool usePersistenceSorter = true;

    VISKORES_DEPRECATED_SUPPRESS_BEGIN
    // Compute the volume for each hyperarc and superarc
    caugmented_ns::IdArrayType superarcIntrinsicWeight;
    caugmented_ns::IdArrayType superarcDependentWeight;
    caugmented_ns::IdArrayType supernodeTransferWeight;
    caugmented_ns::IdArrayType hyperarcDependentWeight;

    caugmented_ns::ProcessContourTree::ComputeVolumeWeightsSerial(filter.GetContourTree(),
                                                                  filter.GetNumIterations(),
                                                                  superarcIntrinsicWeight,
                                                                  superarcDependentWeight,
                                                                  supernodeTransferWeight,
                                                                  hyperarcDependentWeight);

    // Compute the branch decomposition by volume
    caugmented_ns::IdArrayType whichBranch;
    caugmented_ns::IdArrayType branchMinimum;
    caugmented_ns::IdArrayType branchMaximum;
    caugmented_ns::IdArrayType branchSaddle;
    caugmented_ns::IdArrayType branchParent;

    caugmented_ns::ProcessContourTree::ComputeVolumeBranchDecompositionSerial(
      filter.GetContourTree(),
      superarcDependentWeight,
      superarcIntrinsicWeight,
      whichBranch,
      branchMinimum,
      branchMaximum,
      branchSaddle,
      branchParent);

    using ValueArray = viskores::cont::ArrayHandle<DataValueType>;
    ValueArray dataField;
    arr.AsArrayHandle(dataField);

    using BranchType =
      viskores::worklet::contourtree_augmented::process_contourtree_inc::Branch<DataValueType>;

    BranchType* branchDecompositionRoot =
      caugmented_ns::ProcessContourTree::ComputeBranchDecomposition<DataValueType>(
        filter.GetContourTree().Superparents,
        filter.GetContourTree().Supernodes,
        whichBranch,
        branchMinimum,
        branchMaximum,
        branchSaddle,
        branchParent,
        filter.GetSortOrder(),
        dataField,
        dataFieldIsSorted);
    VISKORES_DEPRECATED_SUPPRESS_END

    branchDecompositionRoot->SimplifyToSize(numComp, usePersistenceSorter);

    int contourType = 0;
    branchDecompositionRoot->GetRelevantValues(contourType, eps, isoValues);

    std::sort(isoValues.begin(), isoValues.end());
    auto it = std::unique(isoValues.begin(), isoValues.end());
    isoValues.resize(std::distance(isoValues.begin(), it));

    if (branchDecompositionRoot)
    {
      delete branchDecompositionRoot;
    }
  }
#endif

  //
  //  Internal helper function to execute the contour tree and save repeat code in tests
  //
  // datSets: 0 -> 5x5.txt (2D), 1 -> 8x9test.txt (2D), 2-> 5b.txt (3D)
  //
  // When deprecatedBoundary is true the (now deprecated) boundary-augmentation level (2) is
  // requested via the deprecated SetComputeRegularStructure API; otherwise augmentTree selects
  // full (true) or no (false) augmentation through the supported bool API.
  viskores::cont::DataSet RunContourTree(bool useMarchingCubes,
                                         bool augmentTree,
                                         unsigned int dataSetNo,
                                         bool deprecatedBoundary = false) const
  {
    viskores::cont::DataSet dataSet;
    switch (dataSetNo)
    {
      case 0:
        dataSet = MakeTestDataSet().Make2DUniformDataSet1();
        break;
      case 1:
        dataSet = MakeTestDataSet().Make2DUniformDataSet3();
        break;
      case 2:
        dataSet = MakeTestDataSet().Make3DUniformDataSet1();
        break;
      case 3:
        dataSet = MakeTestDataSet().Make3DUniformDataSet4();
        break;
      default:
        VISKORES_TEST_ASSERT(false);
    }
    viskores::filter::scalar_topology::ContourTreeAugmented filter;
    filter.SetUseMarchingCubes(useMarchingCubes);
    if (deprecatedBoundary)
    {
      VISKORES_DEPRECATED_SUPPRESS_BEGIN
      filter.SetComputeRegularStructure(2);
      VISKORES_DEPRECATED_SUPPRESS_END
    }
    else
    {
      filter.SetAugmentTree(augmentTree);
    }
    filter.SetActiveField("pointvar");
    return filter.Execute(dataSet);
  }

public:
  //
  // Create a uniform 2D structured cell set as input with values for contours
  //
  void TestContourTree_Mesh2D_Freudenthal_SquareExtents(bool augmentTree = true,
                                                        bool deprecatedBoundary = false) const
  {
    std::cout << "Testing ContourTree_Augmented 2D Mesh. augmentTree=" << augmentTree
              << " deprecatedBoundary=" << deprecatedBoundary << std::endl;
    viskores::cont::DataSet result = RunContourTree(false,       // no marching cubes,
                                                    augmentTree, // augmentation level
                                                    0,           // use 5x5.txt
                                                    deprecatedBoundary);

    // Compute the saddle peaks to make sure the contour tree is correct
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      result, saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
    // Print the expected contour tree
    std::cout << "Expected Contour Tree" << std::endl;
    std::cout << "           0           12" << std::endl;
    std::cout << "           4           13" << std::endl;
    std::cout << "          12           13" << std::endl;
    std::cout << "          12           18" << std::endl;
    std::cout << "          12           20" << std::endl;
    std::cout << "          13           14" << std::endl;
    std::cout << "          13           19" << std::endl;

    VISKORES_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 7),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), viskores::make_Pair(0, 12)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), viskores::make_Pair(4, 13)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), viskores::make_Pair(12, 13)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), viskores::make_Pair(12, 18)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), viskores::make_Pair(12, 20)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), viskores::make_Pair(13, 14)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), viskores::make_Pair(13, 19)),
                         "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(bool augmentTree = true,
                                                           bool deprecatedBoundary = false) const
  {
    std::cout << "Testing ContourTree_Augmented 2D Mesh. augmentTree=" << augmentTree
              << " deprecatedBoundary=" << deprecatedBoundary << std::endl;
    viskores::cont::DataSet result = RunContourTree(false,       // no marching cubes,
                                                    augmentTree, // augmentation level
                                                    1,           // use 8x9test.txt
                                                    deprecatedBoundary);

    // Compute the saddle peaks to make sure the contour tree is correct
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      result, saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
    // Print the expected contour tree
    std::cout << "Expected Contour Tree" << std::endl;
    std::cout << "          10           20" << std::endl;
    std::cout << "          20           34" << std::endl;
    std::cout << "          20           38" << std::endl;
    std::cout << "          20           61" << std::endl;
    std::cout << "          23           34" << std::endl;
    std::cout << "          24           34" << std::endl;
    std::cout << "          50           61" << std::endl;
    std::cout << "          61           71" << std::endl;

    VISKORES_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 8),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), viskores::make_Pair(10, 20)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), viskores::make_Pair(20, 34)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), viskores::make_Pair(20, 38)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), viskores::make_Pair(20, 61)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), viskores::make_Pair(23, 34)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), viskores::make_Pair(24, 34)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), viskores::make_Pair(50, 61)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), viskores::make_Pair(61, 71)),
                         "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh3D_Freudenthal_CubicExtents(bool augmentTree = true,
                                                       bool deprecatedBoundary = false) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh. augmentTree=" << augmentTree
              << " deprecatedBoundary=" << deprecatedBoundary << std::endl;

    // Execute the filter
    viskores::cont::DataSet result = RunContourTree(false,       // no marching cubes,
                                                    augmentTree, // augmentation level
                                                    2,           // use 5b.txt (3D) mesh
                                                    deprecatedBoundary);

    // Compute the saddle peaks to make sure the contour tree is correct
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      result, saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
    // Print the expected contour tree
    std::cout << "Expected Contour Tree" << std::endl;
    std::cout << "           0           67" << std::endl;
    std::cout << "          31           42" << std::endl;
    std::cout << "          42           43" << std::endl;
    std::cout << "          42           56" << std::endl;
    std::cout << "          56           67" << std::endl;
    std::cout << "          56           92" << std::endl;
    std::cout << "          62           67" << std::endl;
    std::cout << "          81           92" << std::endl;
    std::cout << "          92           93" << std::endl;

    // Make sure the contour tree is correct
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 9),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), viskores::make_Pair(0, 67)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), viskores::make_Pair(31, 42)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), viskores::make_Pair(42, 43)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), viskores::make_Pair(42, 56)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), viskores::make_Pair(56, 67)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), viskores::make_Pair(56, 92)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), viskores::make_Pair(62, 67)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), viskores::make_Pair(81, 92)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(8), viskores::make_Pair(92, 93)),
                         "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(bool augmentTree = true,
                                                          bool deprecatedBoundary = false) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh. augmentTree=" << augmentTree
              << " deprecatedBoundary=" << deprecatedBoundary << std::endl;

    // Execute the filter
    viskores::cont::DataSet result = RunContourTree(false,       // no marching cubes,
                                                    augmentTree, // augmentation level
                                                    3, // use 5b.txt (3D) upsampled to 5x6x7 mesh
                                                    deprecatedBoundary);

    // Compute the saddle peaks to make sure the contour tree is correct
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      result, saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
    // Print the expected contour tree
    std::cout << "Expected Contour Tree" << std::endl;
    std::cout << "           0          112" << std::endl;
    std::cout << "          71           72" << std::endl;
    std::cout << "          72           78" << std::endl;
    std::cout << "          72          101" << std::endl;
    std::cout << "         101          112" << std::endl;
    std::cout << "         101          132" << std::endl;
    std::cout << "         107          112" << std::endl;
    std::cout << "         131          132" << std::endl;
    std::cout << "         132          138" << std::endl;

    // Make sure the contour tree is correct
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 9),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), viskores::make_Pair(0, 112)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), viskores::make_Pair(71, 72)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), viskores::make_Pair(72, 78)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), viskores::make_Pair(72, 101)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), viskores::make_Pair(101, 112)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), viskores::make_Pair(101, 132)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), viskores::make_Pair(107, 112)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), viskores::make_Pair(131, 132)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(8), viskores::make_Pair(132, 138)),
                         "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh3D_MarchingCubes_CubicExtents(bool augmentTree = true,
                                                         bool deprecatedBoundary = false) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh Marching Cubes. augmentTree=" << augmentTree
              << " deprecatedBoundary=" << deprecatedBoundary << std::endl;

    // Execute the filter
    viskores::cont::DataSet result = RunContourTree(true,        // marching cubes,
                                                    augmentTree, // augmentation level
                                                    2,           // use 5b.txt (3D) mesh
                                                    deprecatedBoundary);

    // Compute the saddle peaks to make sure the contour tree is correct
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      result, saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
    // Print the expected contour tree
    std::cout << "Expected Contour Tree" << std::endl;
    std::cout << "           0          118" << std::endl;
    std::cout << "          31           41" << std::endl;
    std::cout << "          41           43" << std::endl;
    std::cout << "          41           56" << std::endl;
    std::cout << "          56           67" << std::endl;
    std::cout << "          56           91" << std::endl;
    std::cout << "          62           67" << std::endl;
    std::cout << "          67          118" << std::endl;
    std::cout << "          81           91" << std::endl;
    std::cout << "          91           93" << std::endl;
    std::cout << "         118          124" << std::endl;

    VISKORES_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 11),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), viskores::make_Pair(0, 118)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), viskores::make_Pair(31, 41)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), viskores::make_Pair(41, 43)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), viskores::make_Pair(41, 56)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), viskores::make_Pair(56, 67)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), viskores::make_Pair(56, 91)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), viskores::make_Pair(62, 67)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), viskores::make_Pair(67, 118)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(8), viskores::make_Pair(81, 91)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(9), viskores::make_Pair(91, 93)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(
      test_equal(saddlePeak.WritePortal().Get(10), viskores::make_Pair(118, 124)),
      "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(bool augmentTree = true,
                                                            bool deprecatedBoundary = false) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh Marching Cubes. augmentTree=" << augmentTree
              << " deprecatedBoundary=" << deprecatedBoundary << std::endl;

    // Execute the filter
    viskores::cont::DataSet result = RunContourTree(true,        // marching cubes,
                                                    augmentTree, // augmentation level
                                                    3, // use 5b.txt (3D) upsampled to 5x6x7 mesh
                                                    deprecatedBoundary);

    // Compute the saddle peaks to make sure the contour tree is correct
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      result, saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
    // Print the expected contour tree
    std::cout << "Expected Contour Tree" << std::endl;
    std::cout << "           0          203" << std::endl;
    std::cout << "          71           72" << std::endl;
    std::cout << "          72           78" << std::endl;
    std::cout << "          72          101" << std::endl;
    std::cout << "         101          112" << std::endl;
    std::cout << "         101          132" << std::endl;
    std::cout << "         107          112" << std::endl;
    std::cout << "         112          203" << std::endl;
    std::cout << "         131          132" << std::endl;
    std::cout << "         132          138" << std::endl;
    std::cout << "         203          209" << std::endl;

    VISKORES_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 11),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), viskores::make_Pair(0, 203)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), viskores::make_Pair(71, 72)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), viskores::make_Pair(72, 78)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), viskores::make_Pair(72, 101)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), viskores::make_Pair(101, 112)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), viskores::make_Pair(101, 132)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), viskores::make_Pair(107, 112)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), viskores::make_Pair(112, 203)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(8), viskores::make_Pair(131, 132)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(9), viskores::make_Pair(132, 138)),
                         "Wrong result for ContourTree filter");
    VISKORES_TEST_ASSERT(
      test_equal(saddlePeak.WritePortal().Get(10), viskores::make_Pair(203, 209)),
      "Wrong result for ContourTree filter");
  }

  // Requesting branch decomposition must force a fully augmented tree at Execute() time,
  // regardless of the order relative to SetAugmentTree(false). The output must therefore
  // carry the augmentation and branch fields.
  void TestBranchDecompositionEnforcesAugmentation() const
  {
    std::cout << "Testing ContourTree_Augmented branch-decomposition augmentation enforcement"
              << std::endl;

    viskores::cont::DataSet dataSet = MakeTestDataSet().Make2DUniformDataSet1();
    viskores::filter::scalar_topology::ContourTreeAugmented filter;
    // Deliberately request branch decomposition first, then disable augmentation. Execute() must
    // still produce a fully augmented tree with branch-decomposition fields.
    filter.SetComputeBranchDecomposition(true);
    filter.SetAugmentTree(false);
    filter.SetActiveField("pointvar");
    viskores::cont::DataSet result = filter.Execute(dataSet);

    VISKORES_TEST_ASSERT(result.HasField("Superparents"),
                         "Branch decomposition did not force augmentation (Superparents missing)");
    VISKORES_TEST_ASSERT(result.HasField("WhichBranch"), "WhichBranch field missing");
    VISKORES_TEST_ASSERT(result.HasField("BranchMinimum"), "BranchMinimum field missing");
    VISKORES_TEST_ASSERT(result.HasField("BranchMaximum"), "BranchMaximum field missing");
    VISKORES_TEST_ASSERT(result.HasField("BranchSaddle"), "BranchSaddle field missing");
    VISKORES_TEST_ASSERT(result.HasField("BranchParent"), "BranchParent field missing");
  }

  // Verify CollectRegularVerticesPerSuperarc's grouping against the Supernodes/Superparents
  // fields it is derived from, and check that the nested-vector convenience overload
  // (and, with it, GroupedArrayToNestedVectors) matches the flat/offsets output exactly.
  void TestCollectRegularVerticesPerSuperarc() const
  {
    namespace cta = viskores::worklet::contourtree_augmented;

    std::cout << "Testing ContourTree_Augmented CollectRegularVerticesPerSuperarc" << std::endl;

    viskores::cont::DataSet dataSet = MakeTestDataSet().Make2DUniformDataSet1();
    viskores::filter::scalar_topology::ContourTreeAugmented filter;
    filter.SetAugmentTree(true);
    filter.SetActiveField("pointvar");
    viskores::cont::DataSet result = filter.Execute(dataSet);

    cta::IdArrayType supernodes, superparents;
    result.GetField(cta::FieldNameSupernodes).GetData().AsArrayHandle(supernodes);
    result.GetField(cta::FieldNameSuperparents).GetData().AsArrayHandle(superparents);
    auto supernodePortal = supernodes.ReadPortal();
    auto superparentPortal = superparents.ReadPortal();
    viskores::Id numSupernodes = supernodes.GetNumberOfValues();
    viskores::Id numMeshVertices = superparents.GetNumberOfValues();

    std::set<viskores::Id> supernodeVertices;
    for (viskores::Id s = 0; s < numSupernodes; ++s)
      supernodeVertices.insert(supernodePortal.Get(s));

    cta::IdArrayType arcVertsFlat, arcVertsOffsets;
    cta::ProcessContourTree::CollectRegularVerticesPerSuperarc(
      result, arcVertsFlat, arcVertsOffsets);
    auto flatPortal = arcVertsFlat.ReadPortal();
    auto offsetsPortal = arcVertsOffsets.ReadPortal();

    VISKORES_TEST_ASSERT(arcVertsOffsets.GetNumberOfValues() == numSupernodes + 1,
                         "arcVertsOffsets must have one entry per superarc plus a final total");
    VISKORES_TEST_ASSERT(offsetsPortal.Get(0) == 0, "arcVertsOffsets must start at 0");
    VISKORES_TEST_ASSERT(offsetsPortal.Get(numSupernodes) == arcVertsFlat.GetNumberOfValues(),
                         "the last offset must equal the total number of grouped vertices");
    VISKORES_TEST_ASSERT(arcVertsFlat.GetNumberOfValues() == numMeshVertices - numSupernodes,
                         "every non-supernode mesh vertex must appear exactly once");

    std::set<viskores::Id> seenVertices;
    for (viskores::Id i = 0; i < numSupernodes; ++i)
    {
      viskores::Id begin = offsetsPortal.Get(i);
      viskores::Id end = offsetsPortal.Get(i + 1);
      VISKORES_TEST_ASSERT(end >= begin, "arcVertsOffsets must be non-decreasing");

      viskores::Id previousVertex = -1;
      for (viskores::Id k = begin; k < end; ++k)
      {
        viskores::Id vertex = flatPortal.Get(k);
        VISKORES_TEST_ASSERT(supernodeVertices.find(vertex) == supernodeVertices.end(),
                             "a supernode's own mesh vertex must not appear as a regular vertex");
        VISKORES_TEST_ASSERT(cta::MaskedIndex(superparentPortal.Get(vertex)) == i,
                             "a regular vertex must be grouped under its own owning superarc");
        VISKORES_TEST_ASSERT(vertex > previousVertex,
                             "regular vertices within a superarc must be in ascending order");
        previousVertex = vertex;
        VISKORES_TEST_ASSERT(seenVertices.insert(vertex).second,
                             "each regular mesh vertex must appear in exactly one superarc group");
      }
    }
    VISKORES_TEST_ASSERT(static_cast<viskores::Id>(seenVertices.size()) ==
                           numMeshVertices - numSupernodes,
                         "all non-supernode mesh vertices must have been grouped");

    // The nested-vector convenience overload must reproduce the flat/offsets grouping exactly.
    std::vector<std::vector<viskores::Id>> arcVerts =
      cta::ProcessContourTree::CollectRegularVerticesPerSuperarc(result);
    VISKORES_TEST_ASSERT(arcVerts.size() == static_cast<size_t>(numSupernodes),
                         "the nested-vector overload must return one group per superarc");
    for (viskores::Id i = 0; i < numSupernodes; ++i)
    {
      viskores::Id begin = offsetsPortal.Get(i);
      viskores::Id end = offsetsPortal.Get(i + 1);
      const std::vector<viskores::Id>& group = arcVerts[static_cast<size_t>(i)];
      VISKORES_TEST_ASSERT(group.size() == static_cast<size_t>(end - begin),
                           "nested-vector group size must match the flat/offsets range");
      for (viskores::Id k = begin; k < end; ++k)
      {
        VISKORES_TEST_ASSERT(group[static_cast<size_t>(k - begin)] == flatPortal.Get(k),
                             "nested-vector group contents must match the flat array");
      }
    }
  }

  void TestAnalysis() const
  {
    std::cout << "Testing ContourTree_Augmented With Analysis" << std::endl;

    using ValueType = viskores::Float32;
    viskores::cont::DataSet ds = MakeTestDataSet().Make3DUniformDataSet1();

    std::string fieldName = "pointvar";
    std::vector<ValueType> isoValues;

#ifdef VISKORES_ENABLE_MPI
    // Deprecated multi-block path: full augmentation, branch decomposition is computed outside
    // the filter through the legacy getters (analysisLegacy), so it is not requested here.
    viskores::filter::scalar_topology::ContourTreeAugmented filter;
    filter.SetUseMarchingCubes(false);
    filter.SetAugmentTree(true);
    filter.SetActiveField(fieldName);

    viskores::cont::PartitionedDataSet pds;
    int mpiRank = 0;
    int mpiSize = 1;

    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    GetPartitionedDataSet(ds, fieldName, mpiSize, mpiRank, mpiSize, pds);
    ShiftLogicalOriginToZero(pds);
    ComputeGlobalPointSize(pds);
    auto pdsResult = filter.Execute(pds);

    if (mpiRank != 0)
      return;

    // Print the contour tree we computed (via the legacy getters for the deprecated path).
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    VISKORES_DEPRECATED_SUPPRESS_BEGIN
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);
    VISKORES_DEPRECATED_SUPPRESS_END
    std::cout << "Computed Contour Tree" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);

    if (mpiSize == 1)
    {
      analysisLegacy<ValueType>(
        filter, false, pds.GetPartitions()[0].GetField(fieldName).GetData(), 3, isoValues);
    }
    else
    {
      analysisLegacy<ValueType>(
        filter, true, pdsResult.GetPartitions()[0].GetField(0).GetData(), 3, isoValues);
    }
#else
    // Supported single-DataSet path: results (including branch decomposition) are read from the
    // output DataSet.
    viskores::filter::scalar_topology::ContourTreeAugmented filter;
    filter.SetUseMarchingCubes(false);
    filter.SetAugmentTree(true);
    filter.SetComputeBranchDecomposition(true);
    filter.SetActiveField(fieldName);
    viskores::cont::DataSet result = filter.Execute(ds);

    // Print the contour tree we computed
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      result, saddlePeak);
    std::cout << "Computed Contour Tree" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);

    analysis<ValueType>(result, fieldName, 3, isoValues);
#endif

    std::ostringstream os;
    os << "[" << isoValues[0] << "," << isoValues[1] << "," << isoValues[2] << "]";
    std::cout << "COMPUTED_ISOVALUES:" << os.str() << std::endl;
    std::cout << "EXPECTED ISOVALUES:"
              << "[40,75,87]" << std::endl;
    VISKORES_TEST_ASSERT(os.str() == "[40,75,87]");
  }

  void operator()() const
  {
    // For each mesh: full augmentation (true), no augmentation (false), and the deprecated
    // boundary augmentation (level 2). The contour tree (saddle peaks) is identical in all cases.
    this->TestContourTree_Mesh2D_Freudenthal_SquareExtents(true);
    this->TestContourTree_Mesh2D_Freudenthal_SquareExtents(false);
    this->TestContourTree_Mesh2D_Freudenthal_SquareExtents(false, /*deprecatedBoundary=*/true);

    this->TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(true);
    this->TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(false);
    this->TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(false, /*deprecatedBoundary=*/true);

    this->TestContourTree_Mesh3D_Freudenthal_CubicExtents(true);
    this->TestContourTree_Mesh3D_Freudenthal_CubicExtents(false);
    this->TestContourTree_Mesh3D_Freudenthal_CubicExtents(false, /*deprecatedBoundary=*/true);

    this->TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(true);
    this->TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(false);
    this->TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(false, /*deprecatedBoundary=*/true);

    this->TestContourTree_Mesh3D_MarchingCubes_CubicExtents(true);
    this->TestContourTree_Mesh3D_MarchingCubes_CubicExtents(false);
    this->TestContourTree_Mesh3D_MarchingCubes_CubicExtents(false, /*deprecatedBoundary=*/true);

    this->TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(true);
    this->TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(false);
    this->TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(false, /*deprecatedBoundary=*/true);

    // Branch decomposition must force full augmentation regardless of setter order.
    this->TestBranchDecompositionEnforcesAugmentation();

    this->TestCollectRegularVerticesPerSuperarc();

    // Test Analysis
    this->TestAnalysis();
  }
};
}

int UnitTestContourTreeUniformAugmentedFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestContourTreeUniformAugmented(), argc, argv);
}
