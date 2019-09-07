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

#include <vtkm/worklet/ContourTreeUniformAugmented.h>
#include <vtkm/worklet/contourtree_augmented/ContourTree.h>
#include <vtkm/worklet/contourtree_augmented/Types.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/contourtree_augmented/PrintVectors.h>
#include <vtkm/worklet/contourtree_augmented/ProcessContourTree.h>

#include <utility>
#include <vector>
#include <vtkm/Types.h>

namespace
{

using vtkm::cont::testing::MakeTestDataSet;

class TestContourTreeUniform
{
public:
  //
  // Create a uniform 2D structured cell set as input with values for contours
  //
  void TestContourTree_Mesh2D_Freudenthal() const
  {
    std::cout << "Testing ContourTree_PPP2 2D Mesh" << std::endl;

    // Create the input uniform cell set with values to contour
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make2DUniformDataSet1();

    vtkm::cont::CellSetStructured<2> cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);

    vtkm::Id2 pointDimensions = cellSet.GetPointDimensions();
    vtkm::Id nRows = pointDimensions[0];
    vtkm::Id nCols = pointDimensions[1];
    vtkm::Id nSlices = 1;

    vtkm::cont::ArrayHandle<vtkm::Float32> field;
    dataSet.GetField("pointvar").GetData().CopyTo(field);

    // Create the worklet and run it
    vtkm::worklet::ContourTreePPP2 contourTreeWorklet;
    std::vector<std::pair<std::string, vtkm::Float64>> timings;
    vtkm::worklet::contourtree_augmented::ContourTree contourTree;
    vtkm::worklet::contourtree_augmented::IdArrayType meshSortOrder;
    vtkm::Id numIterations;
    const bool useMarchingCubes = false;
    const int computeRegularStructure = 1;

    contourTreeWorklet.Run(field,
                           timings,
                           contourTree,
                           meshSortOrder,
                           numIterations,
                           nRows,
                           nCols,
                           nSlices,
                           useMarchingCubes,
                           computeRegularStructure);

    // Compute the saddle peaks to make sure the contour tree is correct
    vtkm::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    vtkm::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      contourTree, meshSortOrder, saddlePeak);
    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    vtkm::worklet::contourtree_augmented::printEdgePairArray(saddlePeak);
    // Print the expected contour tree
    std::cout << "Expected Contour Tree" << std::endl;
    std::cout << "           0           12" << std::endl;
    std::cout << "           4           13" << std::endl;
    std::cout << "          12           13" << std::endl;
    std::cout << "          12           18" << std::endl;
    std::cout << "          12           20" << std::endl;
    std::cout << "          13           14" << std::endl;
    std::cout << "          13           19" << std::endl;

    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 7),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(0), vtkm::make_Pair(0, 12)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(1), vtkm::make_Pair(4, 13)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(2), vtkm::make_Pair(12, 13)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(3), vtkm::make_Pair(12, 18)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(4), vtkm::make_Pair(12, 20)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(5), vtkm::make_Pair(13, 14)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(6), vtkm::make_Pair(13, 19)),
                     "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh3D_Freudenthal() const
  {
    std::cout << "Testing ContourTree_PPP2 3D Mesh" << std::endl;

    // Create the input uniform cell set with values to contour
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet1();

    vtkm::cont::CellSetStructured<3> cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);

    vtkm::Id3 pointDimensions = cellSet.GetPointDimensions();
    vtkm::Id nRows = pointDimensions[0];
    vtkm::Id nCols = pointDimensions[1];
    vtkm::Id nSlices = pointDimensions[2];

    vtkm::cont::ArrayHandle<vtkm::Float32> field;
    dataSet.GetField("pointvar").GetData().CopyTo(field);

    // Create the worklet and run it
    vtkm::worklet::ContourTreePPP2 contourTreeWorklet;
    std::vector<std::pair<std::string, vtkm::Float64>> timings;
    vtkm::worklet::contourtree_augmented::ContourTree contourTree;
    vtkm::worklet::contourtree_augmented::IdArrayType meshSortOrder;
    vtkm::Id numIterations;
    const bool useMarchingCubes = false;
    const int computeRegularStructure = 1;

    contourTreeWorklet.Run(field,
                           timings,
                           contourTree,
                           meshSortOrder,
                           numIterations,
                           nRows,
                           nCols,
                           nSlices,
                           useMarchingCubes,
                           computeRegularStructure);

    // Compute the saddle peaks to make sure the contour tree is correct
    vtkm::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    vtkm::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      contourTree, meshSortOrder, saddlePeak);
    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    vtkm::worklet::contourtree_augmented::printEdgePairArray(saddlePeak);
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
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 9),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(0), vtkm::make_Pair(0, 67)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(1), vtkm::make_Pair(31, 42)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(2), vtkm::make_Pair(42, 43)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(3), vtkm::make_Pair(42, 56)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(4), vtkm::make_Pair(56, 67)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(5), vtkm::make_Pair(56, 92)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(6), vtkm::make_Pair(62, 67)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(7), vtkm::make_Pair(81, 92)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(8), vtkm::make_Pair(92, 93)),
                     "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh3D_MarchingCubes() const
  {
    std::cout << "Testing ContourTree_PPP2 3D Mesh Marching Cubes" << std::endl;

    // Create the input uniform cell set with values to contour
    vtkm::cont::DataSet dataSet = MakeTestDataSet().Make3DUniformDataSet1();

    vtkm::cont::CellSetStructured<3> cellSet;
    dataSet.GetCellSet().CopyTo(cellSet);

    vtkm::Id3 pointDimensions = cellSet.GetPointDimensions();
    vtkm::Id nRows = pointDimensions[0];
    vtkm::Id nCols = pointDimensions[1];
    vtkm::Id nSlices = pointDimensions[2];

    vtkm::cont::ArrayHandle<vtkm::Float32> field;
    dataSet.GetField("pointvar").GetData().CopyTo(field);

    // Create the worklet and run it
    vtkm::worklet::ContourTreePPP2 contourTreeWorklet;
    std::vector<std::pair<std::string, vtkm::Float64>> timings;
    vtkm::worklet::contourtree_augmented::ContourTree contourTree;
    vtkm::worklet::contourtree_augmented::IdArrayType meshSortOrder;
    vtkm::Id numIterations;
    const bool useMarchingCubes = true;
    const int computeRegularStructure = 1;

    contourTreeWorklet.Run(field,
                           timings,
                           contourTree,
                           meshSortOrder,
                           numIterations,
                           nRows,
                           nCols,
                           nSlices,
                           useMarchingCubes,
                           computeRegularStructure);

    // Compute the saddle peaks to make sure the contour tree is correct
    vtkm::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    vtkm::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      contourTree, meshSortOrder, saddlePeak);
    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    vtkm::worklet::contourtree_augmented::printEdgePairArray(saddlePeak);
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

    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 11),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(0), vtkm::make_Pair(0, 118)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(1), vtkm::make_Pair(31, 41)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(2), vtkm::make_Pair(41, 43)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(3), vtkm::make_Pair(41, 56)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(4), vtkm::make_Pair(56, 67)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(5), vtkm::make_Pair(56, 91)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(6), vtkm::make_Pair(62, 67)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(7), vtkm::make_Pair(67, 118)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(8), vtkm::make_Pair(81, 91)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(9), vtkm::make_Pair(91, 93)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetPortalControl().Get(10), vtkm::make_Pair(118, 124)),
                     "Wrong result for ContourTree filter");
  }

  void operator()() const
  {
    this->TestContourTree_Mesh2D_Freudenthal();
    this->TestContourTree_Mesh3D_Freudenthal();
    this->TestContourTree_Mesh3D_MarchingCubes();
  }
};
}

int UnitTestContourTreeUniformAugmented(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourTreeUniform(), argc, argv);
}
