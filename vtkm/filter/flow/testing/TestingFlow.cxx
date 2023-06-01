//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include "TestingFlow.h"

#include <vtkm/CellClassification.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/flow/ParticleAdvection.h>
#include <vtkm/filter/flow/Pathline.h>
#include <vtkm/filter/flow/Streamline.h>
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/worklet/testing/GenerateTestDataSets.h>

vtkm::cont::ArrayHandle<vtkm::Vec3f> CreateConstantVectorField(vtkm::Id num, const vtkm::Vec3f& vec)
{
  vtkm::cont::ArrayHandleConstant<vtkm::Vec3f> vecConst;
  vecConst = vtkm::cont::make_ArrayHandleConstant(vec, num);

  vtkm::cont::ArrayHandle<vtkm::Vec3f> vecField;
  vtkm::cont::ArrayCopy(vecConst, vecField);
  return vecField;
}

void AddVectorFields(vtkm::cont::PartitionedDataSet& pds,
                     const std::string& fieldName,
                     const vtkm::Vec3f& vec)
{
  for (auto& ds : pds)
    ds.AddPointField(fieldName, CreateConstantVectorField(ds.GetNumberOfPoints(), vec));
}

std::vector<vtkm::cont::PartitionedDataSet> CreateAllDataSetBounds(vtkm::Id nPerRank, bool useGhost)
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();

  vtkm::Id totNumBlocks = nPerRank * comm.size();
  vtkm::Id numDims = 5;

  vtkm::FloatDefault x0 = 0;
  vtkm::FloatDefault x1 = x0 + static_cast<vtkm::FloatDefault>(numDims - 1);
  vtkm::FloatDefault dx = x1 - x0;
  vtkm::FloatDefault y0 = 0, y1 = numDims - 1, z0 = 0, z1 = numDims - 1;

  if (useGhost)
  {
    numDims = numDims + 2; //add 1 extra on each side
    x0 = x0 - 1;
    x1 = x1 + 1;
    dx = x1 - x0 - 2;
    y0 = y0 - 1;
    y1 = y1 + 1;
    z0 = z0 - 1;
    z1 = z1 + 1;
  }

  //Create ALL of the blocks.
  std::vector<vtkm::Bounds> bounds;
  for (vtkm::Id i = 0; i < totNumBlocks; i++)
  {
    bounds.push_back(vtkm::Bounds(x0, x1, y0, y1, z0, z1));
    x0 += dx;
    x1 += dx;
  }

  const vtkm::Id3 dims(numDims, numDims, numDims);
  auto allPDS = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, useGhost);

  return allPDS;
}

std::vector<vtkm::Range> ExtractMaxXRanges(const vtkm::cont::PartitionedDataSet& pds, bool useGhost)
{
  std::vector<vtkm::Range> xMaxRanges;
  for (const auto& ds : pds.GetPartitions())
  {
    auto bounds = ds.GetCoordinateSystem().GetBounds();
    auto xMax = bounds.X.Max;
    if (useGhost)
      xMax = xMax - 1;
    xMaxRanges.push_back(vtkm::Range(xMax, xMax + static_cast<vtkm::FloatDefault>(.5)));
  }

  return xMaxRanges;
}

void ValidateOutput(const vtkm::cont::DataSet& out,
                    vtkm::Id numSeeds,
                    const vtkm::Range& xMaxRange,
                    FilterType fType,
                    bool checkEndPoint,
                    bool blockDuplication)
{
  //Validate the result is correct.
  VTKM_TEST_ASSERT(out.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");

  vtkm::cont::UnknownCellSet dcells = out.GetCellSet();
  vtkm::Id numCells = out.GetNumberOfCells();

  if (!blockDuplication)
    VTKM_TEST_ASSERT(numCells == numSeeds, "Wrong number of cells");

  auto coords = out.GetCoordinateSystem().GetDataAsMultiplexer();
  auto ptPortal = coords.ReadPortal();

  if (fType == STREAMLINE || fType == PATHLINE)
  {
    vtkm::cont::CellSetExplicit<> explicitCells;
    VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetExplicit<>>(), "Wrong cell type.");
    explicitCells = dcells.AsCellSet<vtkm::cont::CellSetExplicit<>>();
    for (vtkm::Id j = 0; j < numCells; j++)
    {
      vtkm::cont::ArrayHandle<vtkm::Id> indices;
      explicitCells.GetIndices(j, indices);
      vtkm::Id nPts = indices.GetNumberOfValues();
      auto iPortal = indices.ReadPortal();
      vtkm::Vec3f lastPt = ptPortal.Get(iPortal.Get(nPts - 1));
      if (checkEndPoint)
        VTKM_TEST_ASSERT(xMaxRange.Contains(lastPt[0]), "Wrong end point for seed");
    }
  }
  else if (fType == PARTICLE_ADVECTION)
  {
    if (!blockDuplication)
      VTKM_TEST_ASSERT(out.GetNumberOfPoints() == numSeeds, "Wrong number of coordinates");
    if (checkEndPoint)
    {
      for (vtkm::Id i = 0; i < numCells; i++)
        VTKM_TEST_ASSERT(xMaxRange.Contains(ptPortal.Get(i)[0]), "Wrong end point for seed");
    }
  }
}

void TestPartitionedDataSet(vtkm::Id nPerRank,
                            bool useGhost,
                            FilterType fType,
                            bool useThreaded,
                            bool useAsyncComm,
                            bool useBlockIds,
                            bool duplicateBlocks)
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  if (comm.rank() == 0)
  {
    switch (fType)
    {
      case PARTICLE_ADVECTION:
        std::cout << "Particle advection";
        break;
      case STREAMLINE:
        std::cout << "Streamline";
        break;
      case PATHLINE:
        std::cout << "Pathline";
        break;
    }
    std::cout << " blocksPerRank= " << nPerRank;
    if (useGhost)
      std::cout << " - using ghost cells";
    if (useThreaded)
      std::cout << " - using threaded";
    if (useAsyncComm)
      std::cout << " - usingAsyncComm";
    else
      std::cout << " - usingSyncComm";

    if (useBlockIds)
      std::cout << " - using block IDs";
    if (duplicateBlocks)
      std::cout << " - with duplicate blocks";
    std::cout << " - on a partitioned data set" << std::endl;
  }

  std::vector<vtkm::Id> blockIds;
  //Uniform assignment.
  for (vtkm::Id i = 0; i < nPerRank; i++)
    blockIds.push_back(comm.rank() * nPerRank + i);

  //For block duplication, give everyone the 2nd to last block.
  //We want to keep the last block on the last rank for validation.
  if (duplicateBlocks && blockIds.size() > 1)
  {
    vtkm::Id totNumBlocks = comm.size() * nPerRank;
    vtkm::Id dupBlock = totNumBlocks - 2;
    for (int r = 0; r < comm.size(); r++)
    {
      if (std::find(blockIds.begin(), blockIds.end(), dupBlock) == blockIds.end())
        blockIds.push_back(dupBlock);
    }
  }

  std::vector<vtkm::cont::PartitionedDataSet> allPDS, allPDS2;
  allPDS = CreateAllDataSetBounds(nPerRank, useGhost);
  allPDS2 = CreateAllDataSetBounds(nPerRank, useGhost);
  auto xMaxRanges = ExtractMaxXRanges(allPDS[0], useGhost);

  vtkm::FloatDefault time0 = 0;
  vtkm::FloatDefault time1 = xMaxRanges[xMaxRanges.size() - 1].Max;

  vtkm::Vec3f vecX(1, 0, 0);
  std::string fieldName = "vec";
  vtkm::FloatDefault stepSize = 0.1f;
  vtkm::Id numSteps = 100000;
  for (std::size_t n = 0; n < allPDS.size(); n++)
  {
    vtkm::cont::PartitionedDataSet pds;
    for (const auto& bid : blockIds)
      pds.AppendPartition(allPDS[n].GetPartition(bid));
    AddVectorFields(pds, fieldName, vecX);

    vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
    seedArray = vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                               vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1) });
    vtkm::Id numSeeds = seedArray.GetNumberOfValues();

    if (fType == STREAMLINE)
    {
      vtkm::filter::flow::Streamline streamline;
      SetFilter(streamline,
                stepSize,
                numSteps,
                fieldName,
                seedArray,
                useThreaded,
                useAsyncComm,
                useBlockIds,
                blockIds);
      auto out = streamline.Execute(pds);

      vtkm::Id numOutputs = out.GetNumberOfPartitions();
      bool checkEnds = numOutputs == static_cast<vtkm::Id>(blockIds.size());
      for (vtkm::Id i = 0; i < numOutputs; i++)
      {
        ValidateOutput(out.GetPartition(i),
                       numSeeds,
                       xMaxRanges[blockIds[i]],
                       fType,
                       checkEnds,
                       duplicateBlocks);
      }
    }
    else if (fType == PARTICLE_ADVECTION)
    {
      vtkm::filter::flow::ParticleAdvection particleAdvection;
      SetFilter(particleAdvection,
                stepSize,
                numSteps,
                fieldName,
                seedArray,
                useThreaded,
                useAsyncComm,
                useBlockIds,
                blockIds);

      auto out = particleAdvection.Execute(pds);

      //Particles end up in last rank.
      if (comm.rank() == comm.size() - 1)
      {
        bool checkEnds = out.GetNumberOfPartitions() == static_cast<vtkm::Id>(blockIds.size());
        VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 1, "Wrong number of partitions in output");
        ValidateOutput(out.GetPartition(0),
                       numSeeds,
                       xMaxRanges[xMaxRanges.size() - 1],
                       fType,
                       checkEnds,
                       duplicateBlocks);
      }
      else
        VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 0, "Wrong number of partitions in output");
    }
    else if (fType == PATHLINE)
    {
      vtkm::cont::PartitionedDataSet pds2;
      for (const auto& bid : blockIds)
        pds2.AppendPartition(allPDS2[n].GetPartition(bid));
      AddVectorFields(pds2, fieldName, vecX);

      vtkm::filter::flow::Pathline pathline;
      SetFilter(pathline,
                stepSize,
                numSteps,
                fieldName,
                seedArray,
                useThreaded,
                useAsyncComm,
                useBlockIds,
                blockIds);

      pathline.SetPreviousTime(time0);
      pathline.SetNextTime(time1);
      pathline.SetNextDataSet(pds2);

      auto out = pathline.Execute(pds);
      vtkm::Id numOutputs = out.GetNumberOfPartitions();
      bool checkEnds = numOutputs == static_cast<vtkm::Id>(blockIds.size());
      for (vtkm::Id i = 0; i < numOutputs; i++)
        ValidateOutput(out.GetPartition(i),
                       numSeeds,
                       xMaxRanges[blockIds[i]],
                       fType,
                       checkEnds,
                       duplicateBlocks);
    }
  }
}
