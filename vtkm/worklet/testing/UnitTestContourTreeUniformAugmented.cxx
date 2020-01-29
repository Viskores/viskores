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
#include <vtkm/worklet/contourtree_augmented/Types.h>

#include <typeinfo>
#include <utility>
#include <vector>
#include <vtkm/Types.h>

namespace
{

using vtkm::cont::testing::MakeTestDataSet;
using namespace vtkm::worklet::contourtree_augmented;

class TestContourTreeUniform
{
private:
  void AssertIdArrayHandles(IdArrayType& result, IdArrayType& expected, std::string arrayName) const
  {
    vtkm::cont::testing::TestEqualResult testResult =
      vtkm::cont::testing::test_equal_ArrayHandles(result, expected);
    if (!testResult)
    {
      std::cout << arrayName << " sizes; result=" << result.GetNumberOfValues()
                << " expected=" << expected.GetNumberOfValues() << std::endl;
      PrintIndices(arrayName + " result", result);
      PrintIndices(arrayName + " expected", expected);
    }
    VTKM_TEST_ASSERT(testResult, "Wrong result for " + arrayName);
  }

  struct ExpectedStepResults
  {
  public:
    ExpectedStepResults(IdArrayType& expectedSortOrder,
                        IdArrayType& expectedSortIndices,
                        IdArrayType& meshExtremaPeaksJoin,
                        IdArrayType& meshExtremaPitsJoin,
                        IdArrayType& meshExtremaPeaksBuildRegularChainsJoin,
                        IdArrayType& meshExtremaPitsBuildRegularChainsJoin)
      : SortOrder(expectedSortOrder)
      , SortIndices(expectedSortIndices)
      , MeshExtremaPeaksJoin(meshExtremaPeaksJoin)
      , MeshExtremaPitsJoin(meshExtremaPitsJoin)
      , MeshExtremaPeaksBuildRegularChainsJoin(meshExtremaPeaksBuildRegularChainsJoin)
      , MeshExtremaPitsBuildRegularChainsJoin(meshExtremaPitsBuildRegularChainsJoin)
    {
    }

    IdArrayType SortOrder;
    IdArrayType SortIndices;
    IdArrayType MeshExtremaPeaksJoin;
    IdArrayType MeshExtremaPitsJoin;
    IdArrayType MeshExtremaPeaksBuildRegularChainsJoin;
    IdArrayType MeshExtremaPitsBuildRegularChainsJoin;
  };

  //
  // Internal helper function to run the individual steps of the ContourTreeAugmented worklet
  // locally here to be able to test intermediarry results. This function sets up the mesh
  // structure needed so we can all our detailed test
  template <typename FieldType, typename StorageType>
  void CallTestContourTreeAugmentedSteps(
    const vtkm::cont::ArrayHandle<FieldType, StorageType> fieldArray,
    const vtkm::Id nRows,
    const vtkm::Id nCols,
    const vtkm::Id nSlices,
    bool useMarchingCubes,
    unsigned int computeRegularStructure,
    ExpectedStepResults& expectedResults) const
  {
    using namespace vtkm::worklet::contourtree_augmented;
    // 2D Contour Tree
    if (nSlices == 1)
    {
      // Build the mesh and fill in the values
      Mesh_DEM_Triangulation_2D_Freudenthal<FieldType, StorageType> mesh(nRows, nCols);
      // Run the contour tree on the mesh
      RunTestContourTreeAugmentedSteps(fieldArray,
                                       mesh,
                                       computeRegularStructure,
                                       mesh.GetMeshBoundaryExecutionObject(),
                                       expectedResults);
      return;
    }
    // 3D Contour Tree using marching cubes
    else if (useMarchingCubes)
    {
      // Build the mesh and fill in the values
      Mesh_DEM_Triangulation_3D_MarchingCubes<FieldType, StorageType> mesh(nRows, nCols, nSlices);
      // Run the contour tree on the mesh
      RunTestContourTreeAugmentedSteps(fieldArray,
                                       mesh,
                                       computeRegularStructure,
                                       mesh.GetMeshBoundaryExecutionObject(),
                                       expectedResults);
      return;
    }
    // 3D Contour Tree with Freudenthal
    else
    {
      // Build the mesh and fill in the values
      Mesh_DEM_Triangulation_3D_Freudenthal<FieldType, StorageType> mesh(nRows, nCols, nSlices);
      // Run the contour tree on the mesh
      RunTestContourTreeAugmentedSteps(fieldArray,
                                       mesh,
                                       computeRegularStructure,
                                       mesh.GetMeshBoundaryExecutionObject(),
                                       expectedResults);
      return;
    }
  }

public:
  //
  // Create a uniform 2D structured cell set as input with values for contours
  //
  void TestContourTree_Mesh2D_Freudenthal() const
  {
    std::cout << "Testing ContourTree_Augmented 2D Mesh" << std::endl;

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
    vtkm::worklet::ContourTreeAugmented contourTreeWorklet;
    vtkm::worklet::contourtree_augmented::ContourTree contourTree;
    vtkm::worklet::contourtree_augmented::IdArrayType meshSortOrder;
    vtkm::Id numIterations;
    const bool useMarchingCubes = false;
    const int computeRegularStructure = 1;

    contourTreeWorklet.Run(field,
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
    vtkm::worklet::contourtree_augmented::PrintEdgePairArray(saddlePeak);
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
    std::cout << "Testing ContourTree_Augmented 3D Mesh" << std::endl;

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
    vtkm::worklet::ContourTreeAugmented contourTreeWorklet;
    vtkm::worklet::contourtree_augmented::ContourTree contourTree;
    vtkm::worklet::contourtree_augmented::IdArrayType meshSortOrder;
    vtkm::Id numIterations;
    const bool useMarchingCubes = false;
    const int computeRegularStructure = 1;

    contourTreeWorklet.Run(field,
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
    vtkm::worklet::contourtree_augmented::PrintEdgePairArray(saddlePeak);
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
    std::cout << "Testing ContourTree_Augmented 3D Mesh Marching Cubes" << std::endl;

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
    vtkm::worklet::ContourTreeAugmented contourTreeWorklet;
    vtkm::worklet::contourtree_augmented::ContourTree contourTree;
    vtkm::worklet::contourtree_augmented::IdArrayType meshSortOrder;
    vtkm::Id numIterations;
    const bool useMarchingCubes = true;
    const int computeRegularStructure = 1;

    contourTreeWorklet.Run(field,
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
    vtkm::worklet::contourtree_augmented::PrintEdgePairArray(saddlePeak);
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


  void TestContourTreeAugmentedStepsFreudenthal3DAugmented() const
  {

    // Create the expected results
    vtkm::Id expectedSortOrderArr[125] = {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,  15,  16,  17,
      18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  34,  35,  39,  40,  44,
      45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  59,  60,  64,  65,  69,  70,  71,
      72,  73,  74,  75,  76,  77,  78,  79,  80,  84,  85,  89,  90,  94,  95,  96,  97,  98,
      99,  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
      117, 118, 119, 120, 121, 122, 123, 124, 62,  67,  63,  57,  61,  66,  58,  68,  56,  87,
      37,  83,  91,  33,  41,  82,  92,  32,  42,  86,  88,  36,  38,  81,  93,  31,  43
    };
    IdArrayType expectedSortOrder = vtkm::cont::make_ArrayHandle(expectedSortOrderArr, 125);

    vtkm::Id expectedSortIndicesArr[125] = {
      0,   1,   2,   3,   4,   5,   6,   7,   8,  9,   10,  11,  12,  13,  14,  15,  16,  17,
      18,  19,  20,  21,  22,  23,  24,  25,  26, 27,  28,  29,  30,  123, 115, 111, 31,  32,
      119, 108, 120, 33,  34,  112, 116, 124, 35, 36,  37,  38,  39,  40,  41,  42,  43,  44,
      45,  46,  106, 101, 104, 47,  48,  102, 98, 100, 49,  50,  103, 99,  105, 51,  52,  53,
      54,  55,  56,  57,  58,  59,  60,  61,  62, 121, 113, 109, 63,  64,  117, 107, 118, 65,
      66,  110, 114, 122, 67,  68,  69,  70,  71, 72,  73,  74,  75,  76,  77,  78,  79,  80,
      81,  82,  83,  84,  85,  86,  87,  88,  89, 90,  91,  92,  93,  94,  95,  96,  97
    };
    IdArrayType expectedSortIndices = vtkm::cont::make_ArrayHandle(expectedSortIndicesArr, 125);

    vtkm::Id expectedMeshExtremaPeaksArr[125] = {
      1,   2,   3,   4,   9,   6,   7,   8,   9,   14,  11,  12,  13,  14,  19,  16,  17,  18,
      19,  24,  21,  22,  23,  24,  40,  26,  27,  28,  29,  31,  123, 111, 119, 120, 112, 124,
      37,  112, 116, 124, 124, 42,  43,  44,  45,  47,  106, 111, 102, 111, 103, 120, 53,  103,
      112, 116, 124, 58,  59,  60,  61,  63,  121, 104, 117, 104, 110, 100, 69,  110, 103, 99,
      105, 74,  75,  76,  77,  82,  79,  121, 113, 109, 109, 84,  121, 121, 113, 109, 89,  117,
      117, 107, 118, 94,  110, 110, 114, 122, 123, 119, 115, 115, 106, 119, 111, 108, 123, 113,
      115, 113, 117, 115, 119, 121, 117, 123, 119, 121, 122, 123, 124, 121, 122, 123, 124
    };
    for (vtkm::Id i = 124; i > 120; i--)
    {
      expectedMeshExtremaPeaksArr[i] = expectedMeshExtremaPeaksArr[i] | TERMINAL_ELEMENT;
    }
    IdArrayType expectedMeshExtremaPeaksJoin =
      vtkm::cont::make_ArrayHandle(expectedMeshExtremaPeaksArr, 125);
    IdArrayType expectedMeshExtremaPitsJoin;
    vtkm::cont::Algorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, 125),
                                expectedMeshExtremaPitsJoin);

    vtkm::Id meshExtremaPeaksBuildRegularChainsJoinArr[125] = {
      124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124, 124,
      124, 124, 124, 124, 124, 124, 124, 123, 123, 123, 123, 123, 123, 123, 123, 124, 123, 124,
      123, 123, 123, 124, 124, 123, 123, 123, 123, 123, 123, 123, 123, 123, 123, 124, 123, 123,
      123, 123, 124, 123, 123, 123, 123, 123, 121, 123, 121, 123, 121, 123, 121, 121, 123, 123,
      123, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121, 121,
      121, 121, 122, 121, 121, 121, 121, 122, 123, 123, 123, 123, 123, 123, 123, 123, 123, 121,
      123, 121, 121, 123, 123, 121, 121, 123, 123, 121, 122, 123, 124, 121, 122, 123, 124
    };
    for (vtkm::Id i = 0; i < 125; i++)
    {
      meshExtremaPeaksBuildRegularChainsJoinArr[i] =
        meshExtremaPeaksBuildRegularChainsJoinArr[i] | TERMINAL_ELEMENT;
    }
    IdArrayType meshExtremaPeaksBuildRegularChainsJoin =
      vtkm::cont::make_ArrayHandle(meshExtremaPeaksBuildRegularChainsJoinArr, 125);

    IdArrayType meshExtremaPitsBuildRegularChainsJoin =
      expectedMeshExtremaPitsJoin; // should remain all at 0

    ExpectedStepResults expectedResults(expectedSortOrder,
                                        expectedSortIndices,
                                        expectedMeshExtremaPeaksJoin,
                                        expectedMeshExtremaPitsJoin,
                                        meshExtremaPeaksBuildRegularChainsJoin,
                                        meshExtremaPitsBuildRegularChainsJoin);

    TestContourTreeAugmentedSteps(false, // don't use marchin cubes
                                  1,     // fully augment the tree
                                  expectedResults);
  }

  void TestContourTreeAugmentedSteps(bool useMarchingCubes,
                                     unsigned int computeRegularStructure,
                                     ExpectedStepResults& expectedResults) const
  {
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

    // Run the specific test
    CallTestContourTreeAugmentedSteps(
      field, nRows, nCols, nSlices, useMarchingCubes, computeRegularStructure, expectedResults);
  }


  template <typename FieldType,
            typename StorageType,
            typename MeshClass,
            typename MeshBoundaryClass>
  void RunTestContourTreeAugmentedSteps(
    const vtkm::cont::ArrayHandle<FieldType, StorageType> fieldArray,
    MeshClass& mesh,
    unsigned int computeRegularStructure,
    const MeshBoundaryClass& meshBoundary,
    ExpectedStepResults& expectedResults) const
  {
    std::cout << "Testing contour tree steps with computeRegularStructure="
              << computeRegularStructure << " meshtype=" << typeid(MeshClass).name() << std::endl;

    using namespace vtkm::worklet::contourtree_augmented;
    vtkm::worklet::contourtree_augmented::IdArrayType sortOrder;
    vtkm::worklet::contourtree_augmented::ContourTree contourTree;


    // Stage 1: Load the data into the mesh. This is done in the Run() method above and accessible
    //          here via the mesh parameter. The actual data load is performed outside of the
    //          worklet in the example contour tree app (or whoever uses the worklet)

    // Stage 2 : Sort the data on the mesh to initialize sortIndex & indexReverse on the mesh
    // Sort the mesh data
    mesh.SortData(fieldArray);
    // Test that the sort is correct
    AssertIdArrayHandles(mesh.SortOrder, expectedResults.SortOrder, "mesh.SortOrder");
    AssertIdArrayHandles(mesh.SortOrder, expectedResults.SortOrder, "mesh.SortOrder");

    // Stage 3: Assign every mesh vertex to a peak
    MeshExtrema extrema(mesh.NumVertices);
    extrema.SetStarts(mesh, true);
    AssertIdArrayHandles(extrema.Peaks, expectedResults.MeshExtremaPeaksJoin, "extrema.Peaks");
    AssertIdArrayHandles(extrema.Pits, expectedResults.MeshExtremaPitsJoin, "extrema.Pits");
    extrema.BuildRegularChains(true);
    AssertIdArrayHandles(
      extrema.Peaks, expectedResults.MeshExtremaPeaksBuildRegularChainsJoin, "extrema.Peaks");
    AssertIdArrayHandles(
      extrema.Pits, expectedResults.MeshExtremaPitsBuildRegularChainsJoin, "extrema.Pits");

    // Stage 4: Identify join saddles & construct Active Join Graph
    MergeTree joinTree(mesh.NumVertices, true);
    ActiveGraph joinGraph(true);
    joinGraph.Initialise(mesh, extrema);
    // TODO Add asserts for joinGraph.Initalise

    // Stage 5: Compute Join Tree Hyperarcs from Active Join Graph
    joinGraph.MakeMergeTree(joinTree, extrema);
    // TODO Add asserts for joinGraph.MakeMergeTree

    // Stage 6: Assign every mesh vertex to a pit
    extrema.SetStarts(mesh, false);
    // TODO Add asserts for extream.SetStarts
    extrema.BuildRegularChains(false);
    // TODO Add asserts for extrema.BuildRegularChains

    // Stage 7:     Identify split saddles & construct Active Split Graph
    MergeTree splitTree(mesh.NumVertices, false);
    ActiveGraph splitGraph(false);
    splitGraph.Initialise(mesh, extrema);
    // TODO Add asserts for splitGraph.Initialise

    // Stage 8: Compute Split Tree Hyperarcs from Active Split Graph
    splitGraph.MakeMergeTree(splitTree, extrema);
    // TODO Add asserts for splitGraph.MakeMergeTree

    // Stage 9: Join & Split Tree are Augmented, then combined to construct Contour Tree
    contourTree.Init(mesh.NumVertices);
    // TODO Add asserts for contourTree.Init
    ContourTreeMaker treeMaker(contourTree, joinTree, splitTree);
    // 9.1 First we compute the hyper- and super- structure
    treeMaker.ComputeHyperAndSuperStructure();
    // TODO Add asserts for treeMaker.ComputeHyperAndSuperStructure

    // 9.2 Then we compute the regular structure
    if (computeRegularStructure == 1) // augment with all vertices
    {
      treeMaker.ComputeRegularStructure(extrema);
    }
    else if (computeRegularStructure == 2) // augment by the mesh boundary
    {
      treeMaker.ComputeBoundaryRegularStructure(extrema, mesh, meshBoundary);
    }
    // TODO Add asserts for treeMaker.ComputeRegularStructure / treeMaker.ComputeBoundaryRegularStructure
  }

  void operator()() const
  {
    this->TestContourTree_Mesh2D_Freudenthal();
    this->TestContourTree_Mesh3D_Freudenthal();
    this->TestContourTree_Mesh3D_MarchingCubes();
    this->TestContourTreeAugmentedStepsFreudenthal3DAugmented();
  }
};
}

int UnitTestContourTreeUniformAugmented(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourTreeUniform(), argc, argv);
}
