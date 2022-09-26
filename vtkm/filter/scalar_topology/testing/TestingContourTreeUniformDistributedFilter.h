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

#ifndef _vtk_m_filter_testing_TestingContourTreeUniformDistributedFilter_h_
#define _vtk_m_filter_testing_TestingContourTreeUniformDistributedFilter_h_

#include <vtkm/Types.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/Serialization.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/scalar_topology/ContourTreeUniformDistributed.h>
#include <vtkm/filter/scalar_topology/DistributedBranchDecompositionFilter.h>
#include <vtkm/filter/scalar_topology/testing/SuperArcHelper.h>
#include <vtkm/filter/scalar_topology/testing/VolumeHelper.h>
#include <vtkm/filter/scalar_topology/worklet/branch_decomposition/HierarchicalVolumetricBranchDecomposer.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/Types.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/BranchCompiler.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_distributed/TreeCompiler.h>
#include <vtkm/io/ErrorIO.h>
#include <vtkm/io/VTKDataSetReader.h>

namespace vtkm
{
namespace filter
{
namespace testing
{
namespace contourtree_uniform_distributed
{
inline vtkm::IdComponent FindSplitAxis(vtkm::Id3 globalSize)
{
  vtkm::IdComponent splitAxis = 0;
  for (vtkm::IdComponent d = 1; d < 3; ++d)
  {
    if (globalSize[d] > globalSize[splitAxis])
    {
      splitAxis = d;
    }
  }
  return splitAxis;
}

inline vtkm::Id3 ComputeNumberOfBlocksPerAxis(vtkm::Id3 globalSize, vtkm::Id numberOfBlocks)
{
  // Split numberOfBlocks into a power of two and a remainder
  vtkm::Id powerOfTwoPortion = 1;
  while (numberOfBlocks % 2 == 0)
  {
    powerOfTwoPortion *= 2;
    numberOfBlocks /= 2;
  }

  vtkm::Id3 blocksPerAxis{ 1, 1, 1 };
  if (numberOfBlocks > 1)
  {
    // Split the longest axis according to remainder
    vtkm::IdComponent splitAxis = FindSplitAxis(globalSize);
    blocksPerAxis[splitAxis] = numberOfBlocks;
    globalSize[splitAxis] /= numberOfBlocks;
  }

  // Now perform splits for the power of two remainder of numberOfBlocks
  while (powerOfTwoPortion > 1)
  {
    vtkm::IdComponent splitAxis = FindSplitAxis(globalSize);
    VTKM_ASSERT(globalSize[splitAxis] > 1);
    blocksPerAxis[splitAxis] *= 2;
    globalSize[splitAxis] /= 2;
    powerOfTwoPortion /= 2;
  }

  return blocksPerAxis;
}

inline std::tuple<vtkm::Id3, vtkm::Id3, vtkm::Id3> ComputeBlockExtents(vtkm::Id3 globalSize,
                                                                       vtkm::Id3 blocksPerAxis,
                                                                       vtkm::Id blockNo)
{
  // DEBUG: std::cout << "ComputeBlockExtents("<<globalSize <<", " << blocksPerAxis << ", " << blockNo << ")" << std::endl;
  // DEBUG: std::cout << "Block " << blockNo;

  vtkm::Id3 blockIndex, blockOrigin, blockSize;
  for (vtkm::IdComponent d = 0; d < 3; ++d)
  {
    blockIndex[d] = blockNo % blocksPerAxis[d];
    blockNo /= blocksPerAxis[d];

    float dx = float(globalSize[d] - 1) / float(blocksPerAxis[d]);
    blockOrigin[d] = vtkm::Id(blockIndex[d] * dx);
    vtkm::Id maxIdx =
      blockIndex[d] < blocksPerAxis[d] - 1 ? vtkm::Id((blockIndex[d] + 1) * dx) : globalSize[d] - 1;
    blockSize[d] = maxIdx - blockOrigin[d] + 1;
    // DEBUG: std::cout << " " << blockIndex[d] <<  dx << " " << blockOrigin[d] << " " << maxIdx << " " << blockSize[d] << "; ";
  }
  // DEBUG: std::cout << " -> " << blockIndex << " "  << blockOrigin << " " << blockSize << std::endl;
  return std::make_tuple(blockIndex, blockOrigin, blockSize);
}

inline vtkm::cont::DataSet CreateSubDataSet(const vtkm::cont::DataSet& ds,
                                            vtkm::Id3 blockOrigin,
                                            vtkm::Id3 blockSize,
                                            const std::string& fieldName)
{
  vtkm::Id3 globalSize;
  vtkm::cont::CastAndCall(
    ds.GetCellSet(), vtkm::worklet::contourtree_augmented::GetPointDimensions(), globalSize);
  const vtkm::Id nOutValues = blockSize[0] * blockSize[1] * blockSize[2];

  const auto inDataArrayHandle = ds.GetPointField(fieldName).GetData();

  vtkm::cont::ArrayHandle<vtkm::Id> copyIdsArray;
  copyIdsArray.Allocate(nOutValues);
  auto copyIdsPortal = copyIdsArray.WritePortal();

  vtkm::Id3 outArrIdx;
  for (outArrIdx[2] = 0; outArrIdx[2] < blockSize[2]; ++outArrIdx[2])
    for (outArrIdx[1] = 0; outArrIdx[1] < blockSize[1]; ++outArrIdx[1])
      for (outArrIdx[0] = 0; outArrIdx[0] < blockSize[0]; ++outArrIdx[0])
      {
        vtkm::Id3 inArrIdx = outArrIdx + blockOrigin;
        vtkm::Id inIdx = (inArrIdx[2] * globalSize[1] + inArrIdx[1]) * globalSize[0] + inArrIdx[0];
        vtkm::Id outIdx =
          (outArrIdx[2] * blockSize[1] + outArrIdx[1]) * blockSize[0] + outArrIdx[0];
        VTKM_ASSERT(inIdx >= 0 && inIdx < inDataArrayHandle.GetNumberOfValues());
        VTKM_ASSERT(outIdx >= 0 && outIdx < nOutValues);
        copyIdsPortal.Set(outIdx, inIdx);
      }
  // DEBUG: std::cout << copyIdsPortal.GetNumberOfValues() << std::endl;

  vtkm::cont::Field permutedField;
  bool success =
    vtkm::filter::MapFieldPermutation(ds.GetPointField(fieldName), copyIdsArray, permutedField);
  if (!success)
    throw vtkm::cont::ErrorBadType("Field copy failed (probably due to invalid type)");

  vtkm::cont::DataSetBuilderUniform dsb;
  if (globalSize[2] <= 1) // 2D Data Set
  {
    vtkm::Id2 dimensions{ blockSize[0], blockSize[1] };
    vtkm::cont::DataSet dataSet = dsb.Create(dimensions);
    vtkm::cont::CellSetStructured<2> cellSet;
    cellSet.SetPointDimensions(dimensions);
    cellSet.SetGlobalPointDimensions(vtkm::Id2{ globalSize[0], globalSize[1] });
    cellSet.SetGlobalPointIndexStart(vtkm::Id2{ blockOrigin[0], blockOrigin[1] });
    dataSet.SetCellSet(cellSet);
    dataSet.AddField(permutedField);
    return dataSet;
  }
  else
  {
    vtkm::cont::DataSet dataSet = dsb.Create(blockSize);
    vtkm::cont::CellSetStructured<3> cellSet;
    cellSet.SetPointDimensions(blockSize);
    cellSet.SetGlobalPointDimensions(globalSize);
    cellSet.SetGlobalPointIndexStart(blockOrigin);
    dataSet.SetCellSet(cellSet);
    dataSet.AddField(permutedField);
    return dataSet;
  }
}

inline std::vector<vtkm::worklet::contourtree_distributed::Edge> ReadGroundTruthContourTree(
  std::string filename)
{
  std::ifstream ct_file(filename);
  if (!ct_file.is_open())
  {
    throw vtkm::io::ErrorIO("Unable to open data file: " + filename);
  }
  vtkm::Id val1, val2;
  std::vector<vtkm::worklet::contourtree_distributed::Edge> result;
  while (ct_file >> val1 >> val2)
  {
    result.push_back(vtkm::worklet::contourtree_distributed::Edge(val1, val2));
  }
  std::sort(result.begin(), result.end());
  return result;
}

inline vtkm::cont::PartitionedDataSet RunContourTreeDUniformDistributed(
  const vtkm::cont::DataSet& ds,
  std::string fieldName,
  bool useMarchingCubes,
  int numberOfBlocks,
  int rank,
  int numberOfRanks,
  bool augmentHierarchicalTree,
  bool computeHierarchicalVolumetricBranchDecomposition,
  vtkm::Id3& globalSize,
  bool passBlockIndices = true)
{
  // Get dimensions of data set
  vtkm::cont::CastAndCall(
    ds.GetCellSet(), vtkm::worklet::contourtree_augmented::GetPointDimensions(), globalSize);

  // Determine split
  vtkm::Id3 blocksPerAxis = ComputeNumberOfBlocksPerAxis(globalSize, numberOfBlocks);
  vtkm::Id blocksPerRank = numberOfBlocks / numberOfRanks;
  vtkm::Id numRanksWithExtraBlock = numberOfBlocks % numberOfRanks;
  vtkm::Id blocksOnThisRank, startBlockNo;
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
  vtkm::cont::PartitionedDataSet pds;
  vtkm::cont::ArrayHandle<vtkm::Id3> localBlockIndices;
  localBlockIndices.Allocate(blocksOnThisRank);

  auto localBlockIndicesPortal = localBlockIndices.WritePortal();

  for (vtkm::Id blockNo = 0; blockNo < blocksOnThisRank; ++blockNo)
  {
    vtkm::Id3 blockOrigin, blockSize, blockIndex;
    std::tie(blockIndex, blockOrigin, blockSize) =
      ComputeBlockExtents(globalSize, blocksPerAxis, startBlockNo + blockNo);
    pds.AppendPartition(CreateSubDataSet(ds, blockOrigin, blockSize, fieldName));
    localBlockIndicesPortal.Set(blockNo, blockIndex);
  }

  // Run the contour tree analysis
  vtkm::filter::scalar_topology::ContourTreeUniformDistributed filter(
    vtkm::cont::LogLevel::UserVerboseLast, vtkm::cont::LogLevel::UserVerboseLast);

  if (passBlockIndices)
  {
    filter.SetBlockIndices(blocksPerAxis, localBlockIndices);
  }

  filter.SetUseMarchingCubes(useMarchingCubes);
  // Freudenthal: Only use boundary extrema; MC: use all points on boundary
  // TODO/FIXME: Figure out why MC does not work when only using boundary extrema
  filter.SetUseBoundaryExtremaOnly(!useMarchingCubes);
  filter.SetAugmentHierarchicalTree(augmentHierarchicalTree);
  filter.SetActiveField(fieldName);
  auto result = filter.Execute(pds);

  if (computeHierarchicalVolumetricBranchDecomposition)
  {
    using vtkm::filter::scalar_topology::DistributedBranchDecompositionFilter;

    DistributedBranchDecompositionFilter bd_filter;
    result = bd_filter.Execute(result);
  }

  if (numberOfRanks == 1)
  {
    // Serial or only one parallel rank -> Result is already
    // everything we need
    return result;
  }
  else
  {
    // Mutiple ranks -> Some assembly required. Collect data
    // on rank 0, all other ranks return empty data sets
    using FieldTypeList = vtkm::ListAppend<vtkm::TypeListScalarAll, vtkm::List<vtkm::Id>>;
    using DataSetWrapper =
      vtkm::cont::SerializableDataSet<FieldTypeList, vtkm::cont::CellSetListStructured>;

    // Communicate results to rank 0
    auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    vtkmdiy::Master master(comm, 1);
    struct EmptyBlock
    {
    }; // Dummy block structure, since we need block data for DIY
    master.add(comm.rank(), new EmptyBlock, new vtkmdiy::Link);
    // .. Send data to rank 0
    master.foreach ([result, filter](void*, const vtkmdiy::Master::ProxyWithLink& p) {
      vtkmdiy::BlockID root{ 0, 0 }; // Rank 0
      p.enqueue(root, result.GetNumberOfPartitions());
      for (const vtkm::cont::DataSet& curr_ds : result)
      {
        auto curr_sds = DataSetWrapper(curr_ds);
        p.enqueue(root, curr_sds);
      }
    });
    // Exchange data, i.e., send to rank 0 (pass "true" to exchange data between
    // *all* blocks, not just neighbors)
    master.exchange(true);

    if (comm.rank() == 0)
    {
      // Receive data on rank zero and return combined results
      vtkm::cont::PartitionedDataSet combined_result;
      master.foreach ([&combined_result, filter, numberOfRanks](
                        void*, const vtkmdiy::Master::ProxyWithLink& p) {
        for (int receiveFromRank = 0; receiveFromRank < numberOfRanks; ++receiveFromRank)
        {
          vtkm::Id numberOfDataSetsToReceive;
          p.dequeue({ receiveFromRank, receiveFromRank }, numberOfDataSetsToReceive);
          for (vtkm::Id currReceiveDataSetNo = 0; currReceiveDataSetNo < numberOfDataSetsToReceive;
               ++currReceiveDataSetNo)
          {
            vtkm::cont::SerializableDataSet<> sds;
            p.dequeue({ receiveFromRank, receiveFromRank }, sds);
            combined_result.AppendPartition(sds.DataSet);
          }
        }
      });
      return combined_result; // Return combined result on rank 0
    }
    else
    {
      // Return an empty data set on all other ranks
      return vtkm::cont::PartitionedDataSet{};
    }
  }
}

inline vtkm::cont::PartitionedDataSet RunContourTreeDUniformDistributed(
  const vtkm::cont::DataSet& ds,
  std::string fieldName,
  bool useMarchingCubes,
  int numberOfBlocks,
  int rank = 0,
  int numberOfRanks = 1,
  bool augmentHierarchicalTree = false,
  bool computeHierarchicalVolumetricBranchDecomposition = false,
  bool passBlockIndices = true)
{
  vtkm::Id3 globalSize;

  return RunContourTreeDUniformDistributed(ds,
                                           fieldName,
                                           useMarchingCubes,
                                           numberOfBlocks,
                                           rank,
                                           numberOfRanks,
                                           augmentHierarchicalTree,
                                           computeHierarchicalVolumetricBranchDecomposition,
                                           globalSize,
                                           passBlockIndices);
}

inline void TestContourTreeUniformDistributed8x9(int nBlocks, int rank = 0, int size = 1)
{
  if (rank == 0)
  {
    std::cout << "Testing ContourTreeUniformDistributed on 2D 8x9 data set divided into " << nBlocks
              << " blocks." << std::endl;
  }
  vtkm::cont::DataSet in_ds = vtkm::cont::testing::MakeTestDataSet().Make2DUniformDataSet3();
  vtkm::cont::PartitionedDataSet result =
    RunContourTreeDUniformDistributed(in_ds, "pointvar", false, nBlocks, rank, size);

  if (vtkm::cont::EnvironmentTracker::GetCommunicator().rank() == 0)
  {
    vtkm::worklet::contourtree_distributed::TreeCompiler treeCompiler;
    for (vtkm::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
    {
      treeCompiler.AddHierarchicalTree(result.GetPartition(ds_no));
    }
    treeCompiler.ComputeSuperarcs();

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    treeCompiler.PrintSuperarcs();

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

    using Edge = vtkm::worklet::contourtree_distributed::Edge;
    VTKM_TEST_ASSERT(test_equal(treeCompiler.superarcs.size(), 8),
                     "Wrong result for ContourTreeUniformDistributed filter");
    VTKM_TEST_ASSERT(treeCompiler.superarcs[0] == Edge{ 10, 20 },
                     "Wrong result for ContourTreeUniformDistributed filter");
    VTKM_TEST_ASSERT(treeCompiler.superarcs[1] == Edge{ 20, 34 },
                     "Wrong result for ContourTreeUniformDistributed filter");
    VTKM_TEST_ASSERT(treeCompiler.superarcs[2] == Edge{ 20, 38 },
                     "Wrong result for ContourTreeUniformDistributed filter");
    VTKM_TEST_ASSERT(treeCompiler.superarcs[3] == Edge{ 20, 61 },
                     "Wrong result for ContourTreeUniformDistributed filter");
    VTKM_TEST_ASSERT(treeCompiler.superarcs[4] == Edge{ 23, 34 },
                     "Wrong result for ContourTreeUniformDistributed filter");
    VTKM_TEST_ASSERT(treeCompiler.superarcs[5] == Edge{ 24, 34 },
                     "Wrong result for ContourTreeUniformDistributed filter");
    VTKM_TEST_ASSERT(treeCompiler.superarcs[6] == Edge{ 50, 61 },
                     "Wrong result for ContourTreeUniformDistributed filter");
    VTKM_TEST_ASSERT(treeCompiler.superarcs[7] == Edge{ 61, 71 },
                     "Wrong result for ContourTreeUniformDistributed filter");
  }
}

inline void TestContourTreeUniformDistributed5x6x7(int nBlocks,
                                                   bool marchingCubes,
                                                   int rank = 0,
                                                   int size = 1)
{
  if (rank == 0)
  {
    std::cout << "Testing ContourTreeUniformDistributed with "
              << (marchingCubes ? "marching cubes" : "Freudenthal")
              << " mesh connectivity on 3D 5x6x7 data set divided into " << nBlocks << " blocks."
              << std::endl;
  }

  vtkm::cont::DataSet in_ds = vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet4();
  vtkm::cont::PartitionedDataSet result =
    RunContourTreeDUniformDistributed(in_ds, "pointvar", marchingCubes, nBlocks, rank, size);

  if (rank == 0)
  {
    vtkm::worklet::contourtree_distributed::TreeCompiler treeCompiler;
    for (vtkm::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
    {
      treeCompiler.AddHierarchicalTree(result.GetPartition(ds_no));
    }
    treeCompiler.ComputeSuperarcs();

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    treeCompiler.PrintSuperarcs();

    // Print the expected contour tree
    using Edge = vtkm::worklet::contourtree_distributed::Edge;
    std::cout << "Expected Contour Tree" << std::endl;
    if (!marchingCubes)
    {
      std::cout << "           0          112" << std::endl;
      std::cout << "          71           72" << std::endl;
      std::cout << "          72           78" << std::endl;
      std::cout << "          72          101" << std::endl;
      std::cout << "         101          112" << std::endl;
      std::cout << "         101          132" << std::endl;
      std::cout << "         107          112" << std::endl;
      std::cout << "         131          132" << std::endl;
      std::cout << "         132          138" << std::endl;

      VTKM_TEST_ASSERT(test_equal(treeCompiler.superarcs.size(), 9),
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[0] == Edge{ 0, 112 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[1] == Edge{ 71, 72 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[2] == Edge{ 72, 78 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[3] == Edge{ 72, 101 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[4] == Edge{ 101, 112 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[5] == Edge{ 101, 132 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[6] == Edge{ 107, 112 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[7] == Edge{ 131, 132 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[8] == Edge{ 132, 138 },
                       "Wrong result for ContourTreeUniformDistributed filter");
    }
    else
    {
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

      VTKM_TEST_ASSERT(test_equal(treeCompiler.superarcs.size(), 11),
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[0] == Edge{ 0, 203 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[1] == Edge{ 71, 72 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[2] == Edge{ 72, 78 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[3] == Edge{ 72, 101 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[4] == Edge{ 101, 112 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[5] == Edge{ 101, 132 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[6] == Edge{ 107, 112 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[7] == Edge{ 112, 203 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[8] == Edge{ 131, 132 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[9] == Edge{ 132, 138 },
                       "Wrong result for ContourTreeUniformDistributed filter");
      VTKM_TEST_ASSERT(treeCompiler.superarcs[10] == Edge{ 203, 209 },
                       "Wrong result for ContourTreeUniformDistributed filter");
    }
  }
}

inline void TestContourTreeFile(std::string ds_filename,
                                std::string fieldName,
                                std::string gtct_filename,
                                int nBlocks,
                                bool marchingCubes = false,
                                int rank = 0,
                                int size = 1,
                                bool augmentHierarchicalTree = false,
                                bool computeHierarchicalVolumetricBranchDecomposition = false,
                                bool passBlockIndices = true)
{
  if (rank == 0)
  {
    std::cout << "Testing ContourTreeUniformDistributed with "
              << (marchingCubes ? "marching cubes" : "Freudenthal") << " mesh connectivity on \""
              << ds_filename << "\" divided into " << nBlocks << " blocks." << std::endl;
  }

  vtkm::io::VTKDataSetReader reader(ds_filename);
  vtkm::cont::DataSet ds;
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += ds_filename;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }

  vtkm::Id3 globalSize;

  vtkm::cont::PartitionedDataSet result =
    RunContourTreeDUniformDistributed(ds,
                                      fieldName,
                                      marchingCubes,
                                      nBlocks,
                                      rank,
                                      size,
                                      augmentHierarchicalTree,
                                      computeHierarchicalVolumetricBranchDecomposition,
                                      globalSize,
                                      passBlockIndices);

  if (rank == 0)
  {
    if (!augmentHierarchicalTree && computeHierarchicalVolumetricBranchDecomposition)
      augmentHierarchicalTree = true;

    if (augmentHierarchicalTree)
    {
      if (computeHierarchicalVolumetricBranchDecomposition)
      {
        SuperArcHelper helper;

        for (vtkm::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
        {
          auto lds = result.GetPartition(ds_no);

          using vtkm::filter::scalar_topology::HierarchicalVolumetricBranchDecomposer;
          helper.Parse(HierarchicalVolumetricBranchDecomposer::PrintBranches(lds));
        }

        std::stringstream out;

        helper.Print(out);

        std::stringstream in(out.str());
        vtkm::worklet::contourtree_distributed::BranchCompiler compiler1, compiler2;

        compiler1.Parse(in);
        compiler2.Load(gtct_filename);

        if (compiler1.branches != compiler2.branches)
        {
          std::cout << "Computed Branch Decomposition/BranchCompiler" << std::endl;
          compiler1.Print(std::cout);
          std::cout << "Expected Branch Decomposition/BranchCompiler" << std::endl;
          compiler2.Print(std::cout);
          VTKM_TEST_FAIL("Branch Decomposition/BranchCompiler FAILED");
        }
      }
      else
      {
        VolumeHelper volumeHelper1, volumeHelper2;

        for (vtkm::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
        {
          auto lds = result.GetPartition(ds_no);
          vtkm::worklet::contourtree_augmented::IdArrayType supernodes;
          lds.GetField("Supernodes").GetData().AsArrayHandle(supernodes);
          vtkm::worklet::contourtree_augmented::IdArrayType superarcs;
          lds.GetField("Superarcs").GetData().AsArrayHandle(superarcs);
          vtkm::worklet::contourtree_augmented::IdArrayType regularNodeGlobalIds;
          lds.GetField("RegularNodeGlobalIds").GetData().AsArrayHandle(regularNodeGlobalIds);
          vtkm::Id totalVolume = globalSize[0] * globalSize[1] * globalSize[2];
          vtkm::worklet::contourtree_augmented::IdArrayType intrinsicVolume;
          lds.GetField("IntrinsicVolume").GetData().AsArrayHandle(intrinsicVolume);
          vtkm::worklet::contourtree_augmented::IdArrayType dependentVolume;
          lds.GetField("DependentVolume").GetData().AsArrayHandle(dependentVolume);

          std::string dumpVolumesString =
            vtkm::worklet::contourtree_distributed::HierarchicalContourTree<
              vtkm::Float32>::DumpVolumes(supernodes,
                                          superarcs,
                                          regularNodeGlobalIds,
                                          totalVolume,
                                          intrinsicVolume,
                                          dependentVolume);

          volumeHelper1.Parse(dumpVolumesString);
        }

        volumeHelper2.Load(gtct_filename);

        if (volumeHelper1.volumes != volumeHelper2.volumes)
        {
          std::cout << "Computed AugmentHierarchicalTree:" << std::endl;
          volumeHelper1.Print(std::cout);
          std::cout << "Expected AugmentHierarchicalTree:" << std::endl;
          volumeHelper2.Print(std::cout);

          VTKM_TEST_FAIL("AugmentHierarchicalTree FAILED");
        }
      }
    }
    else
    {
      vtkm::worklet::contourtree_distributed::TreeCompiler treeCompiler;
      for (vtkm::Id ds_no = 0; ds_no < result.GetNumberOfPartitions(); ++ds_no)
      {
        treeCompiler.AddHierarchicalTree(result.GetPartition(ds_no));
      }
      treeCompiler.ComputeSuperarcs();

      std::vector<vtkm::worklet::contourtree_distributed::Edge> groundTruthSuperarcs =
        ReadGroundTruthContourTree(gtct_filename);
      if (groundTruthSuperarcs.size() < 50)
      {
        std::cout << "Computed Contour Tree" << std::endl;
        treeCompiler.PrintSuperarcs();

        // Print the expected contour tree
        std::cout << "Expected Contour Tree" << std::endl;
        vtkm::worklet::contourtree_distributed::TreeCompiler::PrintSuperarcArray(
          groundTruthSuperarcs);
      }
      else
      {
        std::cout << "Not printing computed and expected contour tree due to size." << std::endl;
      }

      VTKM_TEST_ASSERT(treeCompiler.superarcs == groundTruthSuperarcs,
                       "Test failed for data set " + ds_filename);
    }
  }
}

}
}
}
} // vtkm::filter::testing::contourtree_uniform_distributed

#endif
