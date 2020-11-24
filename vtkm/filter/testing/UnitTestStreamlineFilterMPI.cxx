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

void TestAMRStreamline(bool useSL, bool useThreaded)
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  if (comm.size() < 2)
    return;

  vtkm::Bounds outerBounds(0, 10, 0, 10, 0, 10);
  vtkm::Id3 outerDims(11, 11, 11);
  auto outerDataSets = vtkm::worklet::testing::CreateAllDataSets(outerBounds, outerDims, false);

  vtkm::Bounds innerBounds(3.8, 5.2, 3.8, 5.2, 3.8, 5.2);
  vtkm::Bounds innerBoundsNoGhost(4, 5, 4, 5, 4, 5);
  vtkm::Id3 innerDims(12, 12, 12);
  auto innerDataSets = vtkm::worklet::testing::CreateAllDataSets(innerBounds, innerDims, true);

  std::size_t numDS = outerDataSets.size();
  for (std::size_t d = 0; d < numDS; d++)
  {
    auto dsOuter = outerDataSets[d];
    auto dsInner = innerDataSets[d];

    //Add ghost cells for the outerDataSets.
    //One interior cell is a ghost.
    std::vector<vtkm::UInt8> ghosts;
    ghosts.resize(dsOuter.GetCellSet().GetNumberOfCells());
    vtkm::Id idx = 0;
    for (vtkm::Id i = 0; i < outerDims[0] - 1; i++)
      for (vtkm::Id j = 0; j < outerDims[1] - 1; j++)
        for (vtkm::Id k = 0; k < outerDims[2] - 1; k++)
        {
          //Mark the inner cell as ghost.
          if (i == 4 && j == 4 && k == 4)
            ghosts[idx] = vtkm::CellClassification::GHOST;
          else
            ghosts[idx] = vtkm::CellClassification::NORMAL;
          idx++;
        }
    dsOuter.AddCellField("vtkmGhostCells", ghosts);

    //Create a partitioned dataset with 1 inner and 1 outer.
    vtkm::cont::PartitionedDataSet pds;
    if (comm.rank() == 0)
      pds.AppendPartition(dsOuter);
    else if (comm.rank() == 1)
      pds.AppendPartition(dsInner);

    std::string fieldName = "vec";
    vtkm::Vec3f vecX(1, 0, 0);
    AddVectorFields(pds, fieldName, vecX);

    //seed 0 goes right through the center of the inner
    vtkm::Particle p0(vtkm::Vec3f(static_cast<vtkm::FloatDefault>(1),
                                  static_cast<vtkm::FloatDefault>(4.5),
                                  static_cast<vtkm::FloatDefault>(4.5)),
                      0);

    //seed 1 goes remains entirely in the outer
    vtkm::Particle p1(vtkm::Vec3f(static_cast<vtkm::FloatDefault>(1),
                                  static_cast<vtkm::FloatDefault>(3),
                                  static_cast<vtkm::FloatDefault>(3)),
                      1);

    vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
    seedArray = vtkm::cont::make_ArrayHandle({ p0, p1 });
    vtkm::Id numSeeds = seedArray.GetNumberOfValues();

    if (useSL)
    {
      vtkm::filter::Streamline filter;
      filter.SetUseThreadedAlgorithm(useThreaded);
      filter.SetStepSize(0.1f);
      filter.SetNumberOfSteps(100000);
      filter.SetSeeds(seedArray);

      filter.SetActiveField(fieldName);
      auto out = filter.Execute(pds);

      if (comm.rank() <= 1)
        VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 1, "Wrong number of partitions in output");
      else
        continue;

      auto ds = out.GetPartition(0);

      //validate the outer (rank 0)
      if (comm.rank() == 0)
      {
        VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                         "Wrong number of coordinate systems in the output dataset");
        auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();
        auto ptPortal = coords.ReadPortal();
        vtkm::cont::DynamicCellSet dcells = ds.GetCellSet();

        VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetExplicit<>>(), "Wrong cell type.");
        //The seed that goes through the inner is broken up into two polylines
        //the begining, and then the end.
        VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds + 1, "Wrong number of cells.");
        auto explicitCells = dcells.Cast<vtkm::cont::CellSetExplicit<>>();
        for (vtkm::Id j = 0; j < numSeeds; j++)
        {
          vtkm::cont::ArrayHandle<vtkm::Id> indices;
          explicitCells.GetIndices(j, indices);
          vtkm::Id nPts = indices.GetNumberOfValues();
          auto iPortal = indices.ReadPortal();
          vtkm::Vec3f lastPt = ptPortal.Get(iPortal.Get(nPts - 1));

          if (j == 0) //this is the seed that goes THROUGH inner.
          {
            VTKM_TEST_ASSERT(outerBounds.Contains(lastPt),
                             "End point is NOT inside the outer bounds.");
            VTKM_TEST_ASSERT(innerBounds.Contains(lastPt),
                             "End point is NOT inside the inner bounds.");
          }
          else
          {
            VTKM_TEST_ASSERT(!outerBounds.Contains(lastPt),
                             "Seed final location is INSIDE the dataset");
            VTKM_TEST_ASSERT(lastPt[0] > outerBounds.X.Max,
                             "Seed final location in wrong location");
          }
        }
      }

      //validate the inner (rank 1)
      else if (comm.rank() == 1)
      {
        VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                         "Wrong number of coordinate systems in the output dataset");
        auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();
        auto ptPortal = coords.ReadPortal();
        auto dcells = ds.GetCellSet();

        VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetExplicit<>>(), "Wrong cell type.");
        VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 1, "Wrong number of cells.");
        auto explicitCells = dcells.Cast<vtkm::cont::CellSetExplicit<>>();

        vtkm::cont::ArrayHandle<vtkm::Id> indices;
        explicitCells.GetIndices(0, indices);
        vtkm::Id nPts = indices.GetNumberOfValues();
        auto iPortal = indices.ReadPortal();
        vtkm::Vec3f lastPt = ptPortal.Get(iPortal.Get(nPts - 1));

        //The last point should be OUTSIDE innerBoundsNoGhost but inside innerBounds
        VTKM_TEST_ASSERT(!innerBoundsNoGhost.Contains(lastPt) && innerBounds.Contains(lastPt),
                         "Seed final location not contained in bounds correctly.");
        VTKM_TEST_ASSERT(lastPt[0] > innerBoundsNoGhost.X.Max,
                         "Seed final location in wrong location");
      }
    }
    else
    {
      vtkm::filter::ParticleAdvection filter;
      filter.SetUseThreadedAlgorithm(useThreaded);
      filter.SetStepSize(0.1f);
      filter.SetNumberOfSteps(100000);
      filter.SetSeeds(seedArray);

      filter.SetActiveField(fieldName);
      auto out = filter.Execute(pds);

      if (comm.rank() == 0)
      {
        VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 1, "Wrong number of partitions in output");
        auto ds = out.GetPartition(0);
        VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                         "Wrong number of coordinate systems in the output dataset");
        vtkm::cont::DynamicCellSet dcells = ds.GetCellSet();
        VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetSingleType<>>(), "Wrong cell type.");

        auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();
        auto ptPortal = coords.ReadPortal();
        VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == numSeeds, "Wrong number of coordinates");

        for (vtkm::Id i = 0; i < numSeeds; i++)
        {
          VTKM_TEST_ASSERT(!outerBounds.Contains(ptPortal.Get(i)),
                           "Seed final location is INSIDE the dataset");
          VTKM_TEST_ASSERT(ptPortal.Get(i)[0] > outerBounds.X.Max,
                           "Seed final location in wrong location");
        }
      }
    }
  }
}

void TestPartitionedDataSet(vtkm::Id nPerRank, bool useGhost, bool useSL, bool useThreaded)
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

      streamline.SetUseThreadedAlgorithm(useThreaded);
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

      particleAdvection.SetUseThreadedAlgorithm(useThreaded);
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
        for (auto useThreaded : flags)
          TestPartitionedDataSet(n, useGhost, useSL, useThreaded);
  }

  for (auto useSL : flags)
    for (auto useThreaded : flags)
      TestAMRStreamline(useSL, useThreaded);
}
}

int UnitTestStreamlineFilterMPI(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamlineFiltersMPI, argc, argv);
}
