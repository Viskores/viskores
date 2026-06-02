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

#ifndef _viskores_filter_testing_TestingContourTreeUniformDistributedFilter_h_
#define _viskores_filter_testing_TestingContourTreeUniformDistributedFilter_h_

#include <viskores/Types.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/Serialization.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/MapFieldPermutation.h>
#include <viskores/filter/scalar_topology/ContourTreeUniformDistributed.h>
#include <viskores/filter/scalar_topology/DistributedBranchDecompositionFilter.h>
#include <viskores/filter/scalar_topology/ExtractTopVolumeContoursFilter.h>
#include <viskores/filter/scalar_topology/SelectTopVolumeBranchesFilter.h>
#include <viskores/filter/scalar_topology/testing/SuperArcHelper.h>
#include <viskores/filter/scalar_topology/testing/VolumeHelper.h>
#include <viskores/io/ErrorIO.h>
#include <viskores/io/VTKDataSetReader.h>

namespace viskores
{
namespace filter
{
namespace testing
{
namespace contourtree_uniform_distributed
{
inline viskores::IdComponent FindSplitAxis(viskores::Id3 globalSize)
{
  viskores::IdComponent splitAxis = 0;
  for (viskores::IdComponent d = 1; d < 3; ++d)
  {
    if (globalSize[d] > globalSize[splitAxis])
    {
      splitAxis = d;
    }
  }
  return splitAxis;
}

inline viskores::Id3 ComputeNumberOfBlocksPerAxis(viskores::Id3 globalSize,
                                                  viskores::Id numberOfBlocks)
{
  // Split numberOfBlocks into a power of two and a remainder
  viskores::Id powerOfTwoPortion = 1;
  while (numberOfBlocks % 2 == 0)
  {
    powerOfTwoPortion *= 2;
    numberOfBlocks /= 2;
  }

  viskores::Id3 blocksPerAxis{ 1, 1, 1 };
  if (numberOfBlocks > 1)
  {
    // Split the longest axis according to remainder
    viskores::IdComponent splitAxis = FindSplitAxis(globalSize);
    blocksPerAxis[splitAxis] = numberOfBlocks;
    globalSize[splitAxis] /= numberOfBlocks;
  }

  // Now perform splits for the power of two remainder of numberOfBlocks
  while (powerOfTwoPortion > 1)
  {
    viskores::IdComponent splitAxis = FindSplitAxis(globalSize);
    VISKORES_ASSERT(globalSize[splitAxis] > 1);
    blocksPerAxis[splitAxis] *= 2;
    globalSize[splitAxis] /= 2;
    powerOfTwoPortion /= 2;
  }

  return blocksPerAxis;
}

inline std::tuple<viskores::Id3, viskores::Id3, viskores::Id3>
ComputeBlockExtents(viskores::Id3 globalSize, viskores::Id3 blocksPerAxis, viskores::Id blockNo)
{
  // DEBUG: std::cout << "ComputeBlockExtents("<<globalSize <<", " << blocksPerAxis << ", " << blockNo << ")" << std::endl;
  // DEBUG: std::cout << "Block " << blockNo;

  viskores::Id3 blockIndex, blockOrigin, blockSize;
  for (viskores::IdComponent d = 0; d < 3; ++d)
  {
    blockIndex[d] = blockNo % blocksPerAxis[d];
    blockNo /= blocksPerAxis[d];

    float dx = float(globalSize[d] - 1) / float(blocksPerAxis[d]);
    blockOrigin[d] = viskores::Id(blockIndex[d] * dx);
    viskores::Id maxIdx = blockIndex[d] < blocksPerAxis[d] - 1
      ? viskores::Id((blockIndex[d] + 1) * dx)
      : globalSize[d] - 1;
    blockSize[d] = maxIdx - blockOrigin[d] + 1;
    // DEBUG: std::cout << " " << blockIndex[d] <<  dx << " " << blockOrigin[d] << " " << maxIdx << " " << blockSize[d] << "; ";
  }
  // DEBUG: std::cout << " -> " << blockIndex << " "  << blockOrigin << " " << blockSize << std::endl;
  return std::make_tuple(blockIndex, blockOrigin, blockSize);
}

viskores::cont::DataSet CreateSubDataSet(const viskores::cont::DataSet& ds,
                                         viskores::Id3 blockOrigin,
                                         viskores::Id3 blockSize,
                                         const std::string& fieldName);

inline void ReadGroundTruthBranchVolume(std::string filename,
                                        std::vector<viskores::Id>& branchDirections,
                                        std::vector<viskores::Id>& branchInnerEnds,
                                        std::vector<viskores::Id>& branchVolumes)
{
  std::ifstream ct_file(filename);
  if (!ct_file.is_open())
  {
    throw viskores::io::ErrorIO("Unable to open ground truth data file: " + filename);
  }
  // read the branch information line by line
  viskores::Id branchDirection;
  while (ct_file >> branchDirection)
  {
    branchDirections.push_back(branchDirection);
    viskores::Id branchInnerEnd, branchVolume;
    if (branchDirection != 0)
    {
      ct_file >> branchInnerEnd >> branchVolume;
      branchInnerEnds.push_back(branchInnerEnd);
      branchVolumes.push_back(branchVolume);
    }
    else
    {
      viskores::Id branchLowerEnd, branchUpperEnd;
      ct_file >> branchLowerEnd >> branchUpperEnd >> branchVolume;
      // we do not store the main branch in the current check
    }
  }
}

viskores::cont::PartitionedDataSet RunContourTreeDUniformDistributed(
  const viskores::cont::DataSet& ds,
  std::string fieldName,
  bool useMarchingCubes,
  int numberOfBlocks,
  int rank,
  int numberOfRanks,
  bool augmentHierarchicalTree,
  bool computeHierarchicalVolumetricBranchDecomposition,
  viskores::Id3& globalSize,
  bool passBlockIndices = true,
  const viskores::Id presimplifyThreshold = 0);

inline viskores::cont::PartitionedDataSet RunContourTreeDUniformDistributed(
  const viskores::cont::DataSet& ds,
  std::string fieldName,
  bool useMarchingCubes,
  int numberOfBlocks,
  int rank = 0,
  int numberOfRanks = 1,
  bool augmentHierarchicalTree = false,
  bool computeHierarchicalVolumetricBranchDecomposition = false,
  bool passBlockIndices = true)
{
  viskores::Id3 globalSize;

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

void TestContourTreeUniformDistributed8x9(int nBlocks, int rank = 0, int size = 1);

void TestContourTreeUniformDistributedBranchDecomposition8x9(int nBlocks,
                                                             int rank = 0,
                                                             int size = 1);

void TestContourTreeUniformDistributed5x6x7(int nBlocks,
                                            bool marchingCubes,
                                            int rank = 0,
                                            int size = 1);

void TestContourTreeFile(std::string ds_filename,
                         std::string fieldName,
                         std::string gtct_filename,
                         int nBlocks,
                         bool marchingCubes = false,
                         int rank = 0,
                         int size = 1,
                         bool augmentHierarchicalTree = false,
                         bool computeHierarchicalVolumetricBranchDecomposition = false,
                         bool passBlockIndices = true);

// Routine to verify the branch decomposition results from presimplification.
inline void VerifyContourTreePresimplificationOutput(std::string datasetName,
                                                     viskores::cont::PartitionedDataSet& tp_result,
                                                     std::vector<viskores::Id>& gtBranchDirections,
                                                     std::vector<viskores::Id>& gtBranchInnerEnds,
                                                     std::vector<viskores::Id>& gtBranchVolumes,
                                                     int rank = 0,
                                                     const viskores::Id presimplifyThreshold = 1)
{
  if (rank == 0)
  {
    // the top branches by volume are consistent across all blocks
    auto tp_ds = tp_result.GetPartition(0);
    auto topVolBranchUpperEndGRIds = tp_ds.GetField("TopVolumeBranchUpperEnd")
                                       .GetData()
                                       .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                       .ReadPortal();
    auto topVolBranchLowerEndGRIds = tp_ds.GetField("TopVolumeBranchLowerEnd")
                                       .GetData()
                                       .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                       .ReadPortal();
    auto topVolBranchVolume = tp_ds.GetField("TopVolumeBranchVolume")
                                .GetData()
                                .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                .ReadPortal();
    auto topVolBranchSaddleEpsilon = tp_ds.GetField("TopVolumeBranchSaddleEpsilon")
                                       .GetData()
                                       .AsArrayHandle<viskores::cont::ArrayHandle<viskores::Id>>()
                                       .ReadPortal();
    viskores::Id nSelectedBranches = topVolBranchUpperEndGRIds.GetNumberOfValues();
    std::vector<viskores::Id> gtSortedOrder, sortedOrder;
    std::vector<viskores::Id> topVolBranchInnerEnds;
    for (viskores::Id branch = 0; branch < nSelectedBranches; branch++)
    {
      if (topVolBranchVolume.Get(branch) > presimplifyThreshold)
      {
        sortedOrder.push_back(branch);
      }
      if (topVolBranchSaddleEpsilon.Get(branch) < 0)
        topVolBranchInnerEnds.push_back(topVolBranchUpperEndGRIds.Get(branch));
      else
        topVolBranchInnerEnds.push_back(topVolBranchLowerEndGRIds.Get(branch));
    }
    for (size_t branch = 0; branch < gtBranchVolumes.size(); branch++)
    {
      if (gtBranchVolumes.at(branch) > presimplifyThreshold)
        gtSortedOrder.push_back(branch);
    }
    VISKORES_TEST_ASSERT(sortedOrder.size() == gtSortedOrder.size(),
                         "Test failed: number of branches does not match for data set " +
                           datasetName);
    std::sort(sortedOrder.begin(),
              sortedOrder.end(),
              [topVolBranchInnerEnds, topVolBranchVolume, topVolBranchSaddleEpsilon](
                const viskores::Id& lhs, const viskores::Id& rhs)
              {
                if (topVolBranchInnerEnds.at(lhs) < topVolBranchInnerEnds.at(rhs))
                  return true;
                if (topVolBranchInnerEnds.at(lhs) > topVolBranchInnerEnds.at(rhs))
                  return false;
                if (topVolBranchVolume.Get(lhs) < topVolBranchVolume.Get(rhs))
                  return true;
                if (topVolBranchVolume.Get(lhs) > topVolBranchVolume.Get(rhs))
                  return false;
                return (topVolBranchSaddleEpsilon.Get(lhs) < topVolBranchSaddleEpsilon.Get(rhs));
              });
    std::sort(gtSortedOrder.begin(),
              gtSortedOrder.end(),
              [gtBranchInnerEnds, gtBranchVolumes, gtBranchDirections](const viskores::Id& lhs,
                                                                       const viskores::Id& rhs)
              {
                if (gtBranchInnerEnds.at(lhs) < gtBranchInnerEnds.at(rhs))
                  return true;
                if (gtBranchInnerEnds.at(lhs) > gtBranchInnerEnds.at(rhs))
                  return false;
                if (gtBranchVolumes.at(lhs) < gtBranchVolumes.at(rhs))
                  return true;
                if (gtBranchVolumes.at(lhs) > gtBranchVolumes.at(rhs))
                  return false;
                return (gtBranchDirections.at(lhs) < gtBranchDirections.at(rhs));
              });

    for (size_t branch = 0; branch < sortedOrder.size(); branch++)
    {
      VISKORES_TEST_ASSERT(topVolBranchInnerEnds.at(sortedOrder.at(branch)) ==
                             gtBranchInnerEnds.at(gtSortedOrder.at(branch)),
                           "Test failed: branch inner end does not match for data set " +
                             datasetName);

      VISKORES_TEST_ASSERT(topVolBranchVolume.Get(sortedOrder.at(branch)) ==
                             gtBranchVolumes.at(gtSortedOrder.at(branch)),
                           "Test failed: branch volume does not match for data set " + datasetName);

      VISKORES_TEST_ASSERT(topVolBranchSaddleEpsilon.Get(sortedOrder.at(branch)) ==
                             gtBranchDirections.at(gtSortedOrder.at(branch)),
                           "Test failed: branch direction does not match for data set " +
                             datasetName);
    }
  }
}

// routine to run distributed contour tree and presimplification
inline void RunContourTreePresimplification(std::string fieldName,
                                            viskores::cont::DataSet& ds,
                                            viskores::cont::PartitionedDataSet& tp_result,
                                            int nBlocks,
                                            bool marchingCubes = false,
                                            int rank = 0,
                                            int size = 1,
                                            bool passBlockIndices = true,
                                            const viskores::Id presimplifyThreshold = 1)
{
  viskores::Id3 globalSize;

  viskores::cont::PartitionedDataSet result =
    RunContourTreeDUniformDistributed(ds,
                                      fieldName,
                                      marchingCubes,
                                      nBlocks,
                                      rank,
                                      size,
                                      true,
                                      true,
                                      globalSize,
                                      passBlockIndices,
                                      presimplifyThreshold);

  // Compute branch decomposition
  viskores::cont::PartitionedDataSet bd_result;

  viskores::filter::scalar_topology::DistributedBranchDecompositionFilter bd_filter;
  bd_result = bd_filter.Execute(result);

  // Compute SelectTopVolumeBranches
  viskores::filter::scalar_topology::SelectTopVolumeBranchesFilter tp_filter;

  // numBranches needs to be large enough to include all branches
  // numBranches < numSuperarcs < globalSize
  tp_filter.SetSavedBranches(globalSize[0] * globalSize[1] *
                             (globalSize[2] > 1 ? globalSize[2] : 1));
  tp_filter.SetPresimplifyThreshold(presimplifyThreshold);
  tp_result = tp_filter.Execute(bd_result);
}

// routine to test contour tree presimplification when data set is
// already in memory
inline void TestContourTreePresimplification(std::string datasetName,
                                             std::string fieldName,
                                             std::string gtbr_filename,
                                             int nBlocks,
                                             viskores::cont::DataSet input_ds,
                                             const viskores::Id presimplifyThreshold = 1,
                                             bool marchingCubes = false,
                                             int rank = 0,
                                             int size = 1,
                                             bool passBlockIndices = true)
{
  if (rank == 0)
  {
    std::cout << "Testing ContourTreeUniformDistributed with "
              << (marchingCubes ? "marching cubes" : "Freudenthal") << " mesh connectivity on \""
              << datasetName << "\" divided into " << nBlocks
              << " blocks. Using presimplification threshold = " << presimplifyThreshold
              << std::endl;
  }

  // get the output of contour tree + presimplification
  viskores::cont::PartitionedDataSet tp_result;
  RunContourTreePresimplification(fieldName,
                                  input_ds,
                                  tp_result,
                                  nBlocks,
                                  marchingCubes,
                                  rank,
                                  size,
                                  passBlockIndices,
                                  presimplifyThreshold);

  // get the ground truth from file
  if (rank == 0)
  {
    std::vector<viskores::Id> gtBranchDirections;
    std::vector<viskores::Id> gtBranchInnerEnds;
    std::vector<viskores::Id> gtBranchVolumes;

    // load the ground truth branch decomposition by volume from file
    ReadGroundTruthBranchVolume(
      gtbr_filename, gtBranchDirections, gtBranchInnerEnds, gtBranchVolumes);

    // verify the contour tree presimplification output with ground truth
    VerifyContourTreePresimplificationOutput(datasetName,
                                             tp_result,
                                             gtBranchDirections,
                                             gtBranchInnerEnds,
                                             gtBranchVolumes,
                                             rank,
                                             presimplifyThreshold);
  }
}

inline void TestContourTreePresimplification(std::string datasetName,
                                             std::string fieldName,
                                             std::string gtbr_filename,
                                             int nBlocks,
                                             std::string ds_filename, // dataset file name
                                             const viskores::Id presimplifyThreshold = 1,
                                             bool marchingCubes = false,
                                             int rank = 0,
                                             int size = 1,
                                             bool passBlockIndices = true)
{
  viskores::cont::DataSet ds;
  if (rank == 0)
    std::cout << "Loading data from " << ds_filename << std::endl;
  viskores::io::VTKDataSetReader reader(ds_filename);
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (viskores::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += ds_filename;
    message += ", ";
    message += e.GetMessage();

    VISKORES_TEST_FAIL(message.c_str());
  }

  TestContourTreePresimplification(datasetName,
                                   fieldName,
                                   gtbr_filename,
                                   nBlocks,
                                   ds,
                                   presimplifyThreshold,
                                   marchingCubes,
                                   rank,
                                   size,
                                   passBlockIndices);
}

}
}
}
} // viskores::filter::testing::contourtree_uniform_distributed

#endif
