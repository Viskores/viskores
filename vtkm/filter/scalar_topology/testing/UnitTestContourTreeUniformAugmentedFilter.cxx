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

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/filter/scalar_topology/ContourTreeUniformAugmented.h>
#include <vtkm/filter/scalar_topology/worklet/contourtree_augmented/ProcessContourTree.h>

namespace
{

using vtkm::cont::testing::MakeTestDataSet;

//
//  Test regular single block contour tree construction
//
class TestContourTreeUniformAugmented
{
private:
  //
  //  Internal helper function to execute the contour tree and save repeat code in tests
  //
  // datSets: 0 -> 5x5.txt (2D), 1 -> 8x9test.txt (2D), 2-> 5b.txt (3D)
  vtkm::filter::scalar_topology::ContourTreeAugmented RunContourTree(
    bool useMarchingCubes,
    unsigned int computeRegularStructure,
    unsigned int dataSetNo) const
  {
    // Create the input uniform cell set with values to contour
    vtkm::cont::DataSet dataSet;
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
        VTKM_TEST_ASSERT(false);
    }
    vtkm::filter::scalar_topology::ContourTreeAugmented filter(useMarchingCubes,
                                                               computeRegularStructure);
    filter.SetActiveField("pointvar");
    filter.Execute(dataSet);
    return filter;
  }

public:
  //
  // Create a uniform 2D structured cell set as input with values for contours
  //
  void TestContourTree_Mesh2D_Freudenthal_SquareExtents(
    unsigned int computeRegularStructure = 1) const
  {
    std::cout << "Testing ContourTree_Augmented 2D Mesh. computeRegularStructure="
              << computeRegularStructure << std::endl;
    vtkm::filter::scalar_topology::ContourTreeAugmented filter =
      RunContourTree(false,                   // no marching cubes,
                     computeRegularStructure, // compute regular structure
                     0                        // use 5x5.txt
      );

    // Compute the saddle peaks to make sure the contour tree is correct
    vtkm::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    vtkm::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    vtkm::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
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
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), vtkm::make_Pair(0, 12)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), vtkm::make_Pair(4, 13)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), vtkm::make_Pair(12, 13)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), vtkm::make_Pair(12, 18)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), vtkm::make_Pair(12, 20)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), vtkm::make_Pair(13, 14)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), vtkm::make_Pair(13, 19)),
                     "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(
    unsigned int computeRegularStructure = 1) const
  {
    std::cout << "Testing ContourTree_Augmented 2D Mesh. computeRegularStructure="
              << computeRegularStructure << std::endl;
    vtkm::filter::scalar_topology::ContourTreeAugmented filter =
      RunContourTree(false,                   // no marching cubes,
                     computeRegularStructure, // compute regular structure
                     1                        // use 8x9test.txt
      );

    // Compute the saddle peaks to make sure the contour tree is correct
    vtkm::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    vtkm::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    vtkm::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
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

    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 8),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), vtkm::make_Pair(10, 20)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), vtkm::make_Pair(20, 34)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), vtkm::make_Pair(20, 38)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), vtkm::make_Pair(20, 61)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), vtkm::make_Pair(23, 34)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), vtkm::make_Pair(24, 34)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), vtkm::make_Pair(50, 61)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), vtkm::make_Pair(61, 71)),
                     "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh3D_Freudenthal_CubicExtents(
    unsigned int computeRegularStructure = 1) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh. computeRegularStructure="
              << computeRegularStructure << std::endl;

    // Execute the filter
    vtkm::filter::scalar_topology::ContourTreeAugmented filter =
      RunContourTree(false,                   // no marching cubes,
                     computeRegularStructure, // compute regular structure
                     2                        // use 5b.txt (3D) mesh
      );

    // Compute the saddle peaks to make sure the contour tree is correct
    vtkm::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    vtkm::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    vtkm::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
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
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), vtkm::make_Pair(0, 67)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), vtkm::make_Pair(31, 42)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), vtkm::make_Pair(42, 43)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), vtkm::make_Pair(42, 56)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), vtkm::make_Pair(56, 67)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), vtkm::make_Pair(56, 92)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), vtkm::make_Pair(62, 67)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), vtkm::make_Pair(81, 92)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(8), vtkm::make_Pair(92, 93)),
                     "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(
    unsigned int computeRegularStructure = 1) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh. computeRegularStructure="
              << computeRegularStructure << std::endl;

    // Execute the filter
    vtkm::filter::scalar_topology::ContourTreeAugmented filter =
      RunContourTree(false,                   // no marching cubes,
                     computeRegularStructure, // compute regular structure
                     3                        // use 5b.txt (3D) upsampled to 5x6x7 mesh
      );

    // Compute the saddle peaks to make sure the contour tree is correct
    vtkm::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    vtkm::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    vtkm::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
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
    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 9),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), vtkm::make_Pair(0, 112)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), vtkm::make_Pair(71, 72)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), vtkm::make_Pair(72, 78)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), vtkm::make_Pair(72, 101)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), vtkm::make_Pair(101, 112)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), vtkm::make_Pair(101, 132)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), vtkm::make_Pair(107, 112)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), vtkm::make_Pair(131, 132)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(8), vtkm::make_Pair(132, 138)),
                     "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh3D_MarchingCubes_CubicExtents(
    unsigned int computeRegularStructure = 1) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh Marching Cubes. computeRegularStructure="
              << computeRegularStructure << std::endl;

    // Execute the filter
    vtkm::filter::scalar_topology::ContourTreeAugmented filter =
      RunContourTree(true,                    // no marching cubes,
                     computeRegularStructure, // compute regular structure
                     2                        // use 5b.txt (3D) mesh
      );

    // Compute the saddle peaks to make sure the contour tree is correct
    vtkm::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    vtkm::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    vtkm::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
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
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), vtkm::make_Pair(0, 118)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), vtkm::make_Pair(31, 41)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), vtkm::make_Pair(41, 43)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), vtkm::make_Pair(41, 56)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), vtkm::make_Pair(56, 67)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), vtkm::make_Pair(56, 91)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), vtkm::make_Pair(62, 67)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), vtkm::make_Pair(67, 118)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(8), vtkm::make_Pair(81, 91)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(9), vtkm::make_Pair(91, 93)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(10), vtkm::make_Pair(118, 124)),
                     "Wrong result for ContourTree filter");
  }

  void TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(
    unsigned int computeRegularStructure = 1) const
  {
    std::cout << "Testing ContourTree_Augmented 3D Mesh Marching Cubes. computeRegularStructure="
              << computeRegularStructure << std::endl;

    // Execute the filter
    vtkm::filter::scalar_topology::ContourTreeAugmented filter =
      RunContourTree(true,                    // no marching cubes,
                     computeRegularStructure, // compute regular structure
                     3                        // use 5b.txt (3D) upsampled to 5x6x7 mesh
      );

    // Compute the saddle peaks to make sure the contour tree is correct
    vtkm::worklet::contourtree_augmented::EdgePairArray saddlePeak;
    vtkm::worklet::contourtree_augmented::ProcessContourTree::CollectSortedSuperarcs(
      filter.GetContourTree(), filter.GetSortOrder(), saddlePeak);

    // Print the contour tree we computed
    std::cout << "Computed Contour Tree" << std::endl;
    vtkm::worklet::contourtree_augmented::PrintEdgePairArrayColumnLayout(saddlePeak);
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

    VTKM_TEST_ASSERT(test_equal(saddlePeak.GetNumberOfValues(), 11),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(0), vtkm::make_Pair(0, 203)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(1), vtkm::make_Pair(71, 72)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(2), vtkm::make_Pair(72, 78)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(3), vtkm::make_Pair(72, 101)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(4), vtkm::make_Pair(101, 112)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(5), vtkm::make_Pair(101, 132)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(6), vtkm::make_Pair(107, 112)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(7), vtkm::make_Pair(112, 203)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(8), vtkm::make_Pair(131, 132)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(9), vtkm::make_Pair(132, 138)),
                     "Wrong result for ContourTree filter");
    VTKM_TEST_ASSERT(test_equal(saddlePeak.WritePortal().Get(10), vtkm::make_Pair(203, 209)),
                     "Wrong result for ContourTree filter");
  }

  void operator()() const
  {
    // Test 2D Freudenthal with augmentation
    this->TestContourTree_Mesh2D_Freudenthal_SquareExtents(1);
    // Make sure the contour tree does not change when we disable augmentation
    this->TestContourTree_Mesh2D_Freudenthal_SquareExtents(0);
    // Make sure the contour tree does not change when we use boundary augmentation
    this->TestContourTree_Mesh2D_Freudenthal_SquareExtents(2);

    // Test 2D Freudenthal with augmentation
    this->TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(1);
    // Make sure the contour tree does not change when we disable augmentation
    this->TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(0);
    // Make sure the contour tree does not change when we use boundary augmentation
    this->TestContourTree_Mesh2D_Freudenthal_NonSquareExtents(2);

    // Test 3D Freudenthal with augmentation
    this->TestContourTree_Mesh3D_Freudenthal_CubicExtents(1);
    // Make sure the contour tree does not change when we disable augmentation
    this->TestContourTree_Mesh3D_Freudenthal_CubicExtents(0);
    // Make sure the contour tree does not change when we use boundary augmentation
    this->TestContourTree_Mesh3D_Freudenthal_CubicExtents(2);

    // Test 3D Freudenthal with augmentation
    this->TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(1);
    // Make sure the contour tree does not change when we disable augmentation
    this->TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(0);
    // Make sure the contour tree does not change when we use boundary augmentation
    this->TestContourTree_Mesh3D_Freudenthal_NonCubicExtents(2);

    // Test 3D marching cubes with augmentation
    this->TestContourTree_Mesh3D_MarchingCubes_CubicExtents(1);
    // Make sure the contour tree does not change when we disable augmentation
    this->TestContourTree_Mesh3D_MarchingCubes_CubicExtents(0);
    // Make sure the contour tree does not change when we use boundary augmentation
    this->TestContourTree_Mesh3D_MarchingCubes_CubicExtents(2);

    // Test 3D marching cubes with augmentation
    this->TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(1);
    // Make sure the contour tree does not change when we disable augmentation
    this->TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(0);
    // Make sure the contour tree does not change when we use boundary augmentation
    this->TestContourTree_Mesh3D_MarchingCubes_NonCubicExtents(2);
  }
};
}

int UnitTestContourTreeUniformAugmentedFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourTreeUniformAugmented(), argc, argv);
}
