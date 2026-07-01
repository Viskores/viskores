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

#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/MakeTestDataSet.h>

#include <viskores/filter/scalar_topology/ContourTreeUniformAugmented.h>
#include <viskores/filter/scalar_topology/worklet/contourtree_augmented/ProcessContourTree.h>


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

  //
  //  Internal helper function to execute the contour tree and save repeat code in tests
  //
  // datSets: 0 -> 5x5.txt (2D), 1 -> 8x9test.txt (2D), 2-> 5b.txt (3D)
  viskores::cont::DataSet RunContourTree(
    bool useMarchingCubes,
    viskores::filter::scalar_topology::AugmentationType augmentTree,
    unsigned int dataSetNo) const
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
    filter.SetAugmentTree(augmentTree);
    filter.SetActiveField("pointvar");
    return filter.Execute(dataSet);
  }

public:
  //
  // Create a uniform 2D structured cell set as input with values for contours
  //
  void TestContourTree_Mesh2D_Freudenthal_SquareExtents(
    viskores::filter::scalar_topology::AugmentationType augmentTree =
      viskores::filter::scalar_topology::AugmentationType::FullAugmentation) const
  {
    std::cout << "Testing ContourTree_Augmented 2D Mesh. augmentTree="
              << static_cast<unsigned int>(augmentTree) << std::endl;
    viskores::cont::DataSet result = RunContourTree(false,       // no marching cubes,
                                                    augmentTree, // augmentation level
                                                    0            // use 5x5.txt
    );

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

  void TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(
    viskores::filter::scalar_topology::AugmentationType augmentTree =
      viskores::filter::scalar_topology::AugmentationType::FullAugmentation) const
  {
    std::cout << "Testing ContourTree_Augmented 2D Mesh. augmentTree="
              << static_cast<unsigned int>(augmentTree) << std::endl;
    viskores::cont::DataSet result = RunContourTree(false,       // no marching cubes,
                                                    augmentTree, // augmentation level
                                                    1            // use 8x9test.txt
    );

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

  void TestContourTree_Mesh3D_Freudenthal_CubicExtents(
    viskores::filter::scalar_topology::AugmentationType augmentTree =
      viskores::filter::scalar_topology::AugmentationType::FullAugmentation) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh. augmentTree="
              << static_cast<unsigned int>(augmentTree) << std::endl;

    // Execute the filter
    viskores::cont::DataSet result = RunContourTree(false,       // no marching cubes,
                                                    augmentTree, // augmentation level
                                                    2            // use 5b.txt (3D) mesh
    );

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

  void TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(
    viskores::filter::scalar_topology::AugmentationType augmentTree =
      viskores::filter::scalar_topology::AugmentationType::FullAugmentation) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh. augmentTree="
              << static_cast<unsigned int>(augmentTree) << std::endl;

    // Execute the filter
    viskores::cont::DataSet result = RunContourTree(false,       // no marching cubes,
                                                    augmentTree, // augmentation level
                                                    3 // use 5b.txt (3D) upsampled to 5x6x7 mesh
    );

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

  void TestContourTree_Mesh3D_MarchingCubes_CubicExtents(
    viskores::filter::scalar_topology::AugmentationType augmentTree =
      viskores::filter::scalar_topology::AugmentationType::FullAugmentation) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh Marching Cubes. augmentTree="
              << static_cast<unsigned int>(augmentTree) << std::endl;

    // Execute the filter
    viskores::cont::DataSet result = RunContourTree(true,        // marching cubes,
                                                    augmentTree, // augmentation level
                                                    2            // use 5b.txt (3D) mesh
    );

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

  void TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(
    viskores::filter::scalar_topology::AugmentationType augmentTree =
      viskores::filter::scalar_topology::AugmentationType::FullAugmentation) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh Marching Cubes. augmentTree="
              << static_cast<unsigned int>(augmentTree) << std::endl;

    // Execute the filter
    viskores::cont::DataSet result = RunContourTree(true,        // marching cubes,
                                                    augmentTree, // augmentation level
                                                    3 // use 5b.txt (3D) upsampled to 5x6x7 mesh
    );

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

  void TestAnalysis() const
  {
    std::cout << "Testing ContourTree_Augmented With Analysis" << std::endl;

    using ValueType = viskores::Float32;
    viskores::cont::DataSet ds = MakeTestDataSet().Make3DUniformDataSet1();

    std::string fieldName = "pointvar";
    viskores::filter::scalar_topology::ContourTreeAugmented filter;
    filter.SetUseMarchingCubes(false);
    filter.SetAugmentTree(viskores::filter::scalar_topology::AugmentationType::FullAugmentation);
    filter.SetComputeBranchDecomposition(true);
    filter.SetActiveField(fieldName);

#ifdef VISKORES_ENABLE_MPI
    viskores::cont::PartitionedDataSet pds;
    int mpiRank = 0;
    int mpiSize = 1;

    MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);
    GetPartitionedDataSet(ds, fieldName, mpiSize, mpiRank, mpiSize, pds);
    ShiftLogicalOriginToZero(pds);
    ComputeGlobalPointSize(pds);
    auto pdsResult = filter.Execute(pds);
    viskores::cont::DataSet result =
      (mpiRank == 0) ? pdsResult.GetPartitions()[0] : viskores::cont::DataSet{};
#else
    viskores::cont::DataSet result = filter.Execute(ds);
#endif

#ifdef VISKORES_ENABLE_MPI
    if (mpiRank != 0)
      return;
#endif

    // Print the contour tree we computed
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      result, saddlePeak);
    std::cout << "Computed Contour Tree" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);

    std::vector<ValueType> isoValues;
    analysis<ValueType>(result, fieldName, 3, isoValues);

    std::ostringstream os;

    os << "[" << isoValues[0] << "," << isoValues[1] << "," << isoValues[2] << "]";
    std::cout << "COMPUTED_ISOVALUES:" << os.str() << std::endl;
    std::cout << "EXPECTED ISOVALUES:"
              << "[40,75,87]" << std::endl;
    VISKORES_TEST_ASSERT(os.str() == "[40,75,87]");
  }

  void operator()() const
  {
    using AT = viskores::filter::scalar_topology::AugmentationType;

    this->TestContourTree_Mesh2D_Freudenthal_SquareExtents(AT::FullAugmentation);
    this->TestContourTree_Mesh2D_Freudenthal_SquareExtents(AT::NoAugmentation);
    this->TestContourTree_Mesh2D_Freudenthal_SquareExtents(AT::BoundaryAugmentation);

    this->TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(AT::FullAugmentation);
    this->TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(AT::NoAugmentation);
    this->TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(AT::BoundaryAugmentation);

    this->TestContourTree_Mesh3D_Freudenthal_CubicExtents(AT::FullAugmentation);
    this->TestContourTree_Mesh3D_Freudenthal_CubicExtents(AT::NoAugmentation);
    this->TestContourTree_Mesh3D_Freudenthal_CubicExtents(AT::BoundaryAugmentation);

    this->TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(AT::FullAugmentation);
    this->TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(AT::NoAugmentation);
    this->TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(AT::BoundaryAugmentation);

    this->TestContourTree_Mesh3D_MarchingCubes_CubicExtents(AT::FullAugmentation);
    this->TestContourTree_Mesh3D_MarchingCubes_CubicExtents(AT::NoAugmentation);
    this->TestContourTree_Mesh3D_MarchingCubes_CubicExtents(AT::BoundaryAugmentation);

    this->TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(AT::FullAugmentation);
    this->TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(AT::NoAugmentation);
    this->TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(AT::BoundaryAugmentation);

    // Test Analysis
    this->TestAnalysis();
  }
};
}

int UnitTestContourTreeUniformAugmentedFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestContourTreeUniformAugmented(), argc, argv);
}
