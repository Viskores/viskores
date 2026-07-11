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

// This file collects the tests for deprecated ContourTreeAugmented functionality:
// the boundary-augmentation level (2), reachable only through the deprecated
// constructor, and the deprecated multi-block (PartitionedDataSet/MPI) path with
// its legacy GetContourTree()/GetSortOrder()/GetNumIterations() getters.
// Delete this file when the deprecated functionality is removed.

#include <viskores/Deprecated.h>
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
//  Test deprecated ContourTreeAugmented functionality
//
class TestContourTreeUniformAugmentedDeprecated
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

#ifdef VISKORES_ENABLE_MPI
  // Analysis for the deprecated multi-block path. Results are accessed through the
  // deprecated GetContourTree()/GetNumIterations()/GetSortOrder() getters rather than
  // from the output DataSet.
  template <typename DataValueType>
  void analysis(viskores::filter::scalar_topology::ContourTreeAugmented& filter,
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
    // Compute the branch decomposition
    // Compute the volume for each hyperarc and superarc
    caugmented_ns::IdArrayType superarcIntrinsicWeight;
    caugmented_ns::IdArrayType superarcDependentWeight;
    caugmented_ns::IdArrayType supernodeTransferWeight;
    caugmented_ns::IdArrayType hyperarcDependentWeight;

    caugmented_ns::ProcessContourTree::ComputeVolumeWeightsSerial(
      filter.GetContourTree(),
      filter.GetNumIterations(),
      superarcIntrinsicWeight,  // (output)
      superarcDependentWeight,  // (output)
      supernodeTransferWeight,  // (output)
      hyperarcDependentWeight); // (output)

    // Compute the branch decomposition by volume
    caugmented_ns::IdArrayType whichBranch;
    caugmented_ns::IdArrayType branchMinimum;
    caugmented_ns::IdArrayType branchMaximum;
    caugmented_ns::IdArrayType branchSaddle;
    caugmented_ns::IdArrayType branchParent;

#ifdef DEBUG
    PrintArrayHandle(superarcIntrinsicWeight, "superarcIntrinsicWeight");
    PrintArrayHandle(superarcDependentWeight, "superarcDependentWeight");
    PrintArrayHandle(supernodeTransferWeight, "superarcDependentWeight");
    PrintArrayHandle(hyperarcDependentWeight, "hyperarcDependentWeight");
#endif // DEBUG


    caugmented_ns::ProcessContourTree::ComputeVolumeBranchDecompositionSerial(
      filter.GetContourTree(),
      superarcDependentWeight,
      superarcIntrinsicWeight,
      whichBranch,   // (output)
      branchMinimum, // (output)
      branchMaximum, // (output)
      branchSaddle,  // (output)
      branchParent); // (output)

    // Create explicit representation of the branch decompostion from the array representation
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

    // Simplify the contour tree of the branch decompostion
    branchDecompositionRoot->SimplifyToSize(numComp, usePersistenceSorter);

    int contourType = 0;

    branchDecompositionRoot->GetRelevantValues(contourType, eps, isoValues);

    // Print the compute iso values
    std::sort(isoValues.begin(), isoValues.end());

    // Unique isovalues
    auto it = std::unique(isoValues.begin(), isoValues.end());
    isoValues.resize(std::distance(isoValues.begin(), it));

    if (branchDecompositionRoot)
    {
      delete branchDecompositionRoot;
    }
  }
#endif

  //
  //  Internal helper function to execute the contour tree with the deprecated
  //  boundary-augmentation level (2), which is reachable only through the
  //  deprecated constructor.
  //
  // datSets: 0 -> 5x5.txt (2D), 1 -> 8x9test.txt (2D), 2-> 5b.txt (3D)
  viskores::cont::DataSet RunContourTreeBoundaryAugmentation(bool useMarchingCubes,
                                                             unsigned int dataSetNo) const
  {
    // Create the input uniform cell set with values to contour
    viskores::cont::DataSet dataSet = MakeInputDataSet(dataSetNo);
    VISKORES_DEPRECATED_SUPPRESS_BEGIN
    viskores::filter::scalar_topology::ContourTreeAugmented filter(useMarchingCubes,
                                                                   2u /* boundary augmentation */);
    VISKORES_DEPRECATED_SUPPRESS_END
    filter.SetActiveField("pointvar");
    return filter.Execute(dataSet);
  }

  // Reference run through the supported API (full augmentation)
  viskores::cont::DataSet RunContourTree(bool useMarchingCubes, unsigned int dataSetNo) const
  {
    viskores::cont::DataSet dataSet = MakeInputDataSet(dataSetNo);
    viskores::filter::scalar_topology::ContourTreeAugmented filter;
    filter.SetUseMarchingCubes(useMarchingCubes);
    filter.SetAugmentTree(true);
    filter.SetActiveField("pointvar");
    return filter.Execute(dataSet);
  }

  viskores::cont::DataSet MakeInputDataSet(unsigned int dataSetNo) const
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
    return dataSet;
  }

public:
  // Make sure the contour tree does not change when we use the deprecated boundary
  // augmentation: compare the saddle peaks of a boundary-augmented run against a run
  // with full augmentation through the supported API.
  void TestBoundaryAugmentation(bool useMarchingCubes, unsigned int dataSetNo) const
  {
    std::cout << "Testing deprecated boundary augmentation. useMarchingCubes=" << useMarchingCubes
              << " dataSetNo=" << dataSetNo << std::endl;

    viskores::cont::DataSet boundaryResult =
      RunContourTreeBoundaryAugmentation(useMarchingCubes, dataSetNo);
    viskores::cont::DataSet referenceResult = RunContourTree(useMarchingCubes, dataSetNo);

    // Compute the saddle peaks of both runs
    viskores::worklet::contourtree_augmented::EdgePairArray boundarySaddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      boundaryResult, boundarySaddlePeak);
    viskores::worklet::contourtree_augmented::EdgePairArray referenceSaddlePeak;
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      referenceResult, referenceSaddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree (boundary augmentation)" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(boundarySaddlePeak);

    VISKORES_TEST_ASSERT(boundarySaddlePeak.GetNumberOfValues() ==
                           referenceSaddlePeak.GetNumberOfValues(),
                         "Deprecated boundary augmentation changed the contour tree size");
    auto boundaryPortal = boundarySaddlePeak.ReadPortal();
    auto referencePortal = referenceSaddlePeak.ReadPortal();
    for (viskores::Id i = 0; i < boundarySaddlePeak.GetNumberOfValues(); ++i)
    {
      VISKORES_TEST_ASSERT(test_equal(boundaryPortal.Get(i), referencePortal.Get(i)),
                           "Deprecated boundary augmentation changed the contour tree");
    }
  }

#ifdef VISKORES_ENABLE_MPI
  // Test the deprecated multi-block path: compute the contour tree over a partitioned
  // data set and run the branch-decomposition analysis through the legacy getters.
  void TestAnalysisMultiBlock() const
  {
    std::cout << "Testing deprecated multi-block ContourTree_Augmented With Analysis" << std::endl;

    using ValueType = viskores::Float32;
    viskores::cont::DataSet ds = MakeTestDataSet().Make3DUniformDataSet1();

    std::string fieldName = "pointvar";
    std::vector<ValueType> isoValues;

    // Deprecated multi-block path: full augmentation, branch decomposition is computed
    // outside the filter through the legacy getters (analysis), so it is not requested here.
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

    // Compute the saddle peaks to make sure the contour tree is correct
    // (via the legacy getters for the deprecated path)
    viskores::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    VISKORES_DEPRECATED_SUPPRESS_BEGIN
    viskores::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);
    VISKORES_DEPRECATED_SUPPRESS_END

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    viskores::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);

    // Do Analysis.
    if (mpiSize == 1)
    {
      analysis<ValueType>(
        filter, false, pds.GetPartitions()[0].GetField(fieldName).GetData(), 3, isoValues);
    }
    else
    {
      analysis<ValueType>(
        filter, true, pdsResult.GetPartitions()[0].GetField(0).GetData(), 3, isoValues);
    }

    // Print the computed iso values
    std::ostringstream os;
    os << "[" << isoValues[0] << "," << isoValues[1] << "," << isoValues[2] << "]";
    std::cout << "COMPUTED_ISOVALUES:" << os.str() << std::endl;
    std::cout << "EXPECTED ISOVALUES:"
              << "[40,75,87]" << std::endl;
    VISKORES_TEST_ASSERT(os.str() == "[40,75,87]");
  }
#endif

  void operator()() const
  {
    // Make sure the contour tree does not change when we use the deprecated boundary
    // augmentation (level 2), for the same mesh/connectivity combinations as the tests
    // of the supported API.
    this->TestBoundaryAugmentation(false, 0); // 2D Freudenthal, square extents
    this->TestBoundaryAugmentation(false, 1); // 2D Freudenthal, non-square extents
    this->TestBoundaryAugmentation(false, 2); // 3D Freudenthal, cubic extents
    this->TestBoundaryAugmentation(false, 3); // 3D Freudenthal, non-cubic extents
    this->TestBoundaryAugmentation(true, 2);  // 3D marching cubes, cubic extents
    this->TestBoundaryAugmentation(true, 3);  // 3D marching cubes, non-cubic extents

#ifdef VISKORES_ENABLE_MPI
    // Test the deprecated multi-block path with analysis through the legacy getters
    this->TestAnalysisMultiBlock();
#endif
  }
};
}

int UnitTestContourTreeUniformAugmentedFilterDeprecated(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(
    TestContourTreeUniformAugmentedDeprecated(), argc, argv);
}
