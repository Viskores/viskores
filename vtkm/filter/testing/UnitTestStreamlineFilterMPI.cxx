//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/ParticleAdvection.h>
#include <vtkm/filter/Streamline.h>
#include <vtkm/worklet/testing/GenerateTestDataSets.h>

#include <vtkm/thirdparty/diy/diy.h>

namespace
{

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

void TestPartitionedDataSet(vtkm::Id nPerRank, bool useGhost, bool useSL)
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();

  vtkm::Id numDims = 5;
  vtkm::FloatDefault x0 = static_cast<vtkm::FloatDefault>((numDims - 1) * (nPerRank * comm.rank()));
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

  std::vector<vtkm::Bounds> bounds;
  for (vtkm::Id i = 0; i < nPerRank; i++)
  {
    bounds.push_back(vtkm::Bounds(x0, x1, y0, y1, z0, z1));
    x0 += dx;
    x1 += dx;
  }

  std::vector<vtkm::cont::PartitionedDataSet> allPDs;
  const vtkm::Id3 dims(numDims, numDims, numDims);
  allPDs = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, useGhost);

  vtkm::Vec3f vecX(1, 0, 0);
  std::string fieldName = "vec";
  for (auto& pds : allPDs)
  {
    AddVectorFields(pds, fieldName, vecX);

    vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
    seedArray = vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                               vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1) });
    vtkm::Id numSeeds = seedArray.GetNumberOfValues();

    if (useSL)
    {
      vtkm::filter::Streamline streamline;

      streamline.SetStepSize(0.1f);
      streamline.SetNumberOfSteps(100000);
      streamline.SetSeeds(seedArray);

      streamline.SetActiveField(fieldName);
      auto out = streamline.Execute(pds);

      for (vtkm::Id i = 0; i < nPerRank; i++)
      {
        auto inputDS = pds.GetPartition(i);
        auto outputDS = out.GetPartition(i);
        VTKM_TEST_ASSERT(outputDS.GetNumberOfCoordinateSystems() == 1,
                         "Wrong number of coordinate systems in the output dataset");

        vtkm::cont::DynamicCellSet dcells = outputDS.GetCellSet();
        VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds, "Wrong number of cells");

        auto coords = outputDS.GetCoordinateSystem().GetDataAsMultiplexer();
        auto ptPortal = coords.ReadPortal();

        vtkm::cont::CellSetExplicit<> explicitCells;

        VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetExplicit<>>(), "Wrong cell type.");
        explicitCells = dcells.Cast<vtkm::cont::CellSetExplicit<>>();

        vtkm::FloatDefault xMax = bounds[static_cast<std::size_t>(i)].X.Max;
        if (useGhost)
          xMax = xMax - 1;
        vtkm::Range xMaxRange(xMax, xMax + static_cast<vtkm::FloatDefault>(.5));

        for (vtkm::Id j = 0; j < numSeeds; j++)
        {
          vtkm::cont::ArrayHandle<vtkm::Id> indices;
          explicitCells.GetIndices(j, indices);
          vtkm::Id nPts = indices.GetNumberOfValues();
          auto iPortal = indices.ReadPortal();
          vtkm::Vec3f lastPt = ptPortal.Get(iPortal.Get(nPts - 1));
          VTKM_TEST_ASSERT(xMaxRange.Contains(lastPt[0]), "Wrong end point for seed");
        }
      }
    }
    else
    {
      vtkm::filter::ParticleAdvection particleAdvection;

      particleAdvection.SetStepSize(0.1f);
      particleAdvection.SetNumberOfSteps(100000);
      particleAdvection.SetSeeds(seedArray);

      particleAdvection.SetActiveField(fieldName);
      auto out = particleAdvection.Execute(pds);

      //Particles end up in last rank.
      if (comm.rank() == comm.size() - 1)
      {
        VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 1, "Wrong number of partitions in output");
        auto ds = out.GetPartition(0);
        //Validate the result is correct.
        VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                         "Wrong number of coordinate systems in the output dataset");

        vtkm::FloatDefault xMax = bounds[bounds.size() - 1].X.Max;
        if (useGhost)
          xMax = xMax - 1;
        vtkm::Range xMaxRange(xMax, xMax + static_cast<vtkm::FloatDefault>(.5));

        auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();
        VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == numSeeds, "Wrong number of coordinates");
        auto ptPortal = coords.ReadPortal();
        for (vtkm::Id i = 0; i < numSeeds; i++)
          VTKM_TEST_ASSERT(xMaxRange.Contains(ptPortal.Get(i)[0]), "Wrong end point for seed");

        vtkm::cont::DynamicCellSet dcells = ds.GetCellSet();
        VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds, "Wrong number of cells");
      }
      else
        VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 0, "Wrong number of partitions in output");
    }
  }
}

void TestStreamlineFiltersMPI()
{
  std::vector<bool> flags = { true, false };
  for (int n = 1; n < 3; n++)
  {
    for (auto useGhost : flags)
      for (auto useSL : flags)
        TestPartitionedDataSet(n, useGhost, useSL);
  }
}
}

int UnitTestStreamlineFilterMPI(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamlineFiltersMPI, argc, argv);
}
