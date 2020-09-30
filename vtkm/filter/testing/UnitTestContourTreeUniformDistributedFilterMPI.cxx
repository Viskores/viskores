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

#include "TestingContourTreeUniformDistributedFilter.h"

#include <vtkm/filter/ContourTreeUniformDistributed.h>
#include <vtkm/worklet/contourtree_distributed/TreeCompiler.h>

#include <vtkm/cont/CellSetList.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/io/VTKDataSetReader.h>

namespace
{
using vtkm::filter::testing::contourtree_uniform_distributed::ComputeBlockExtents;
using vtkm::filter::testing::contourtree_uniform_distributed::ComputeNumberOfBlocksPerAxis;
using vtkm::filter::testing::contourtree_uniform_distributed::CreateSubDataSet;
using vtkm::filter::testing::contourtree_uniform_distributed::ReadGroundTruthContourTree;

class TestContourTreeUniformDistributedFilterMPI
{
public:
  // numberOfBlocks should be a power of 2
  vtkm::cont::PartitionedDataSet RunContourTreeDUniformDistributed(const vtkm::cont::DataSet& ds,
                                                                   std::string fieldName,
                                                                   bool useMarchingCubes,
                                                                   int numberOfBlocks) const
  {
    // Get rank and size
    auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
    auto rank = comm.rank();
    auto numberOfRanks = comm.size();

    // Get dimensions of data set
    vtkm::Id3 globalSize;
    ds.GetCellSet().CastAndCall(vtkm::worklet::contourtree_augmented::GetPointDimensions(),
                                globalSize);

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

    vtkm::cont::PartitionedDataSet pds;
    vtkm::cont::ArrayHandle<vtkm::Id3> localBlockIndices;
    vtkm::cont::ArrayHandle<vtkm::Id3> localBlockOrigins;
    vtkm::cont::ArrayHandle<vtkm::Id3> localBlockSizes;
    localBlockIndices.Allocate(blocksOnThisRank);
    localBlockOrigins.Allocate(blocksOnThisRank);
    localBlockSizes.Allocate(blocksOnThisRank);
    auto localBlockIndicesPortal = localBlockIndices.WritePortal();
    auto localBlockOriginsPortal = localBlockOrigins.WritePortal();
    auto localBlockSizesPortal = localBlockSizes.WritePortal();

    for (vtkm::Id blockNo = 0; blockNo < blocksOnThisRank; ++blockNo)
    {
      vtkm::Id3 blockOrigin, blockSize, blockIndex;
      std::tie(blockIndex, blockOrigin, blockSize) =
        ComputeBlockExtents(globalSize, blocksPerAxis, startBlockNo + blockNo);
      pds.AppendPartition(CreateSubDataSet(ds, blockOrigin, blockSize, fieldName));
      localBlockOriginsPortal.Set(blockNo, blockOrigin);
      localBlockSizesPortal.Set(blockNo, blockSize);
      localBlockIndicesPortal.Set(blockNo, blockIndex);
    }

    // Execute the contour tree analysis
    vtkm::filter::ContourTreeUniformDistributed filter(blocksPerAxis,
                                                       globalSize,
                                                       localBlockIndices,
                                                       localBlockOrigins,
                                                       localBlockSizes,
                                                       useMarchingCubes);
    filter.SetActiveField(fieldName);
    auto result = filter.Execute(pds);

    using FieldTypeList = vtkm::List<vtkm::Float32, vtkm::Float64, vtkm::Id>;
    using DataSetWrapper =
      vtkm::cont::SerializableDataSet<FieldTypeList, vtkm::cont::CellSetListStructured>;

    // Communicate results to rank 0
    vtkmdiy::Master master(comm, 1);
    struct EmptyBlock
    {
    }; // Dummy block structure, since we need block data for DIY
    master.add(comm.rank(), new EmptyBlock, new vtkmdiy::Link);
    // .. Send data to rank 0
    master.foreach ([result, filter](void*, const vtkmdiy::Master::ProxyWithLink& p) {
      vtkmdiy::BlockID root{ 0, 0 }; // Rank 0
      p.enqueue(root, result.GetNumberOfPartitions());
      for (const vtkm::cont::DataSet& ds : result)
      {
        auto sds = DataSetWrapper(ds);
        p.enqueue(root, sds);
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
            auto sds = vtkm::filter::MakeSerializableDataSet(filter);
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

  void TestContourTreeUniformDistributed8x9(int nBlocks) const
  {
    std::cout << "Testing ContourTreeUniformDistributed on 2D 8x9 data set divided into " << nBlocks
              << " blocks." << std::endl;
    vtkm::cont::DataSet in_ds = vtkm::cont::testing::MakeTestDataSet().Make2DUniformDataSet3();
    vtkm::cont::PartitionedDataSet result =
      this->RunContourTreeDUniformDistributed(in_ds, "pointvar", false, nBlocks);

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

  void TestContourTreeUniformDistributed5x6x7(int nBlocks, bool marchingCubes) const
  {
    std::cout << "Testing ContourTreeUniformDistributed with "
              << (marchingCubes ? "marching cubes" : "Freudenthal")
              << " mesh connectivity on 3D 5x6x7 data set divided into " << nBlocks << " blocks."
              << std::endl;

    vtkm::cont::DataSet in_ds = vtkm::cont::testing::MakeTestDataSet().Make3DUniformDataSet4();
    vtkm::cont::PartitionedDataSet result =
      this->RunContourTreeDUniformDistributed(in_ds, "pointvar", marchingCubes, nBlocks);

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

  void TestContourTreeFile(std::string ds_filename,
                           std::string fieldName,
                           std::string gtct_filename,
                           int nBlocks,
                           bool marchingCubes = false) const
  {
    std::cout << "Testing ContourTreeUniformDistributed with "
              << (marchingCubes ? "marching cubes" : "Freudenthal") << " mesh connectivity on \""
              << ds_filename << "\" divided into " << nBlocks << " blocks." << std::endl;

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

    vtkm::cont::PartitionedDataSet result =
      this->RunContourTreeDUniformDistributed(ds, fieldName, marchingCubes, nBlocks);

    if (vtkm::cont::EnvironmentTracker::GetCommunicator().rank() == 0)
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

  void operator()() const
  {
    using vtkm::cont::testing::Testing;
    //this->TestContourTreeUniformDistributed8x9(3);
    this->TestContourTreeUniformDistributed8x9(4);
    this->TestContourTreeUniformDistributed8x9(8);
    this->TestContourTreeUniformDistributed8x9(16);
    this->TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                              "var",
                              Testing::DataPath("rectilinear/vanc.ct_txt"),
                              4);
    this->TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                              "var",
                              Testing::DataPath("rectilinear/vanc.ct_txt"),
                              8);
    this->TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                              "var",
                              Testing::DataPath("rectilinear/vanc.ct_txt"),
                              16);
    this->TestContourTreeUniformDistributed5x6x7(4, false);
    this->TestContourTreeUniformDistributed5x6x7(8, false);
    this->TestContourTreeUniformDistributed5x6x7(16, false);
    this->TestContourTreeUniformDistributed5x6x7(4, true);
    this->TestContourTreeUniformDistributed5x6x7(8, true);
    this->TestContourTreeUniformDistributed5x6x7(16, true);
  }
};
}

int UnitTestContourTreeUniformDistributedFilterMPI(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(
    TestContourTreeUniformDistributedFilterMPI(), argc, argv);
}
