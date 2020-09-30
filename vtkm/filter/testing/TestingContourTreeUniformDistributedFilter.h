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
#include <vtkm/worklet/contourtree_augmented/Types.h>
#include <vtkm/worklet/contourtree_distributed/TreeCompiler.h>

namespace vtkm
{
namespace filter
{
namespace testing
{
namespace contourtree_uniform_distributed
{
// numberOf Blocks must be a power of 2
vtkm::Id3 ComputeNumberOfBlocksPerAxis(vtkm::Id3 globalSize, vtkm::Id numberOfBlocks)
{
  // DEBUG: std::cout << "GlobalSize: " << globalSize << " numberOfBlocks:" << numberOfBlocks << " -> ";
  // Inefficient way to compute log2 of numberOfBlocks, i.e., number of total splits
  vtkm::Id numSplits = 0;
  vtkm::Id currNumberOfBlock = numberOfBlocks;
  bool isPowerOfTwo = true;
  while (currNumberOfBlock > 1)
  {
    if (currNumberOfBlock % 2 != 0)
    {
      isPowerOfTwo = false;
      break;
    }
    currNumberOfBlock /= 2;
    ++numSplits;
  }

  if (isPowerOfTwo)
  {
    vtkm::Id3 splitsPerAxis{ 0, 0, 0 };
    while (numSplits > 0)
    {
      // Find split axis as axis with largest extent
      vtkm::Id splitAxis = 0;
      for (vtkm::Id d = 1; d < 3; ++d)
        if (globalSize[d] > globalSize[splitAxis])
          splitAxis = d;
      // Split in half along that axis
      // DEBUG: std::cout << splitAxis << " " << globalSize << std::endl;
      VTKM_ASSERT(globalSize[splitAxis] > 1);
      ++splitsPerAxis[splitAxis];
      globalSize[splitAxis] /= 2;
      --numSplits;
    }
    // DEBUG: std::cout << "splitsPerAxis: " << splitsPerAxis;
    vtkm::Id3 blocksPerAxis;
    for (vtkm::Id d = 0; d < 3; ++d)
      blocksPerAxis[d] = 1 << splitsPerAxis[d];
    // DEBUG: std::cout << " blocksPerAxis: " << blocksPerAxis << std::endl;
    return blocksPerAxis;
  }
  else
  {
    std::cout << "numberOfBlocks is not a power of two. Splitting along longest axis" << std::endl;
    vtkm::Id splitAxis = 0;
    for (vtkm::Id d = 1; d < 3; ++d)
      if (globalSize[d] > globalSize[splitAxis])
        splitAxis = d;
    vtkm::Id3 blocksPerAxis{ 1, 1, 1 };
    blocksPerAxis[splitAxis] = numberOfBlocks;
    // DEBUG: std::cout << " blocksPerAxis: " << blocksPerAxis << std::endl;
    return blocksPerAxis;
  }
}

std::tuple<vtkm::Id3, vtkm::Id3, vtkm::Id3> ComputeBlockExtents(vtkm::Id3 globalSize,
                                                                vtkm::Id3 blocksPerAxis,
                                                                vtkm::Id blockNo)
{
  // DEBUG: std::cout << "ComputeBlockExtents("<<globalSize <<", " << blocksPerAxis << ", " << blockNo << ")" << std::endl;
  // DEBUG: std::cout << "Block " << blockNo;

  vtkm::Id3 blockIndex, blockOrigin, blockSize;
  for (vtkm::Id d = 0; d < 3; ++d)
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

vtkm::cont::DataSet CreateSubDataSet(const vtkm::cont::DataSet& ds,
                                     vtkm::Id3 blockOrigin,
                                     vtkm::Id3 blockSize,
                                     const std::string& fieldName)
{
  vtkm::Id3 globalSize;
  ds.GetCellSet().CastAndCall(vtkm::worklet::contourtree_augmented::GetPointDimensions(),
                              globalSize);
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

  vtkm::cont::ArrayHandle<vtkm::Float32> inputArrayHandle;
  ds.GetPointField(fieldName).GetData().CopyTo(inputArrayHandle);
  auto permutedInArray = make_ArrayHandlePermutation(copyIdsArray, inputArrayHandle);
  vtkm::cont::ArrayHandle<vtkm::Float32> outputArrayHandle;
  vtkm::cont::ArrayCopy(permutedInArray, outputArrayHandle);
  outputArrayHandle.SyncControlArray();
  VTKM_ASSERT(outputArrayHandle.GetNumberOfValues() == nOutValues);
  // DEBUG: auto rp = outputArrayHandle.ReadPortal();
  // DEBUG: for (vtkm::Id i = 0; i < nOutValues; ++i) std::cout << rp.Get(i) << " ";
  // DEBUG: std::cout << std::endl;

  vtkm::cont::DataSetBuilderUniform dsb;
  if (globalSize[2] <= 1) // 2D Data Set
  {
    vtkm::Id2 dimensions{ blockSize[0], blockSize[1] };
    vtkm::cont::DataSet dataSet = dsb.Create(dimensions);
    dataSet.AddPointField(fieldName, outputArrayHandle);
    return dataSet;
  }
  else
  {
    vtkm::cont::DataSet dataSet = dsb.Create(blockSize);
    dataSet.AddPointField(fieldName, outputArrayHandle);
    return dataSet;
  }
}

std::vector<vtkm::worklet::contourtree_distributed::Edge> ReadGroundTruthContourTree(
  std::string filename)
{
  std::ifstream ct_file(filename);
  vtkm::Id val1, val2;
  std::vector<vtkm::worklet::contourtree_distributed::Edge> result;
  while (ct_file >> val1 >> val2)
  {
    result.push_back(vtkm::worklet::contourtree_distributed::Edge(val1, val2));
  }
  std::sort(result.begin(), result.end());
  return result;
}

}
}
}
} // vtkm::filter::testing::contourtree_uniform_distributed

#endif
