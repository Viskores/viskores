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

#include <vtkm/worklet/contourtree_augmented/meshtypes/ContourTreeMesh.h>

namespace
{

template <typename FieldType>
void TestContourTreeMeshCombine(const std::string& mesh1_filename,
                                const std::string& mesh2_filename,
                                const std::string& combined_filename)
{
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

void TestContourTreeUniformDistributed()
{
  using vtkm::cont::testing::Testing;
  TestContourTreeMeshCombine<vtkm::FloatDefault>(
    Testing::DataPath("misc/5x6_7_MC_Rank0_Block0_Round1_BeforeCombineMesh1.ctm"),
    Testing::DataPath("misc/5x6_7_MC_Rank0_Block0_Round1_BeforeCombineMesh2.ctm"),
    Testing::RegressionImagePath("5x6_7_MC_Rank0_Block0_Round1_CombinedMesh.ctm"));
}

} // anonymous namespace

int UnitTestContourTreeUniformDistributed(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourTreeUniformDistributed, argc, argv);
}
