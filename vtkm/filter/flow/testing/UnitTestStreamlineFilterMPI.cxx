//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/CellClassification.h>
#include <vtkm/Particle.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/flow/ParticleAdvection.h>
#include <vtkm/filter/flow/Pathline.h>
#include <vtkm/filter/flow/Streamline.h>
#include <vtkm/thirdparty/diy/diy.h>
#include <vtkm/worklet/testing/GenerateTestDataSets.h>

namespace
{

enum FilterType
{
  PARTICLE_ADVECTION,
  STREAMLINE,
  PATHLINE
};

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

template <typename FilterType>
void SetFilter(FilterType& filter,
               vtkm::FloatDefault stepSize,
               vtkm::Id numSteps,
               const std::string& fieldName,
               vtkm::cont::ArrayHandle<vtkm::Particle> seedArray,
               bool useThreaded)
{
  filter.SetStepSize(stepSize);
  filter.SetNumberOfSteps(numSteps);
  filter.SetSeeds(seedArray);
  filter.SetActiveField(fieldName);
  filter.SetUseThreadedAlgorithm(useThreaded);
}

void TestAMRStreamline(FilterType fType, bool useThreaded)
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
  if (useThreaded)
    std::cout << " - using threaded";
  std::cout << " - on an AMR data set" << std::endl;

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
            ghosts[idx] = vtkm::CellClassification::Ghost;
          else
            ghosts[idx] = vtkm::CellClassification::Normal;
          idx++;
        }
    dsOuter.SetGhostCellField(vtkm::cont::make_ArrayHandle(ghosts, vtkm::CopyFlag::On));

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

    vtkm::FloatDefault stepSize = 0.1f;
    vtkm::Id numSteps = 100000;

    if (fType == STREAMLINE || fType == PATHLINE)
    {
      vtkm::cont::PartitionedDataSet out;

      if (fType == STREAMLINE)
      {
        vtkm::filter::flow::Streamline streamline;
        SetFilter(streamline, stepSize, numSteps, fieldName, seedArray, useThreaded);
        out = streamline.Execute(pds);
      }
      else if (fType == PATHLINE)
      {
        vtkm::filter::flow::Pathline pathline;
        SetFilter(pathline, stepSize, numSteps, fieldName, seedArray, useThreaded);
        //Create timestep 2
        auto pds2 = vtkm::cont::PartitionedDataSet(pds);
        pathline.SetPreviousTime(0);
        pathline.SetNextTime(10);
        pathline.SetNextDataSet(pds2);
        out = pathline.Execute(pds);
      }

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
        vtkm::cont::UnknownCellSet dcells = ds.GetCellSet();

        VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetExplicit<>>(), "Wrong cell type.");
        //The seed that goes through the inner is broken up into two polylines
        //the begining, and then the end.
        VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds + 1, "Wrong number of cells.");
        auto explicitCells = dcells.AsCellSet<vtkm::cont::CellSetExplicit<>>();
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
        auto explicitCells = dcells.AsCellSet<vtkm::cont::CellSetExplicit<>>();

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
    else if (fType == PARTICLE_ADVECTION)
    {
      vtkm::filter::flow::ParticleAdvection filter;
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
        vtkm::cont::UnknownCellSet dcells = ds.GetCellSet();
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

void ValidateOutput(const vtkm::cont::DataSet& out,
                    vtkm::Id numSeeds,
                    vtkm::Range xMaxRange,
                    FilterType fType)
{
  //Validate the result is correct.
  VTKM_TEST_ASSERT(out.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");

  vtkm::cont::UnknownCellSet dcells = out.GetCellSet();
  out.PrintSummary(std::cout);
  std::cout << " nSeeds= " << numSeeds << std::endl;
  VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds, "Wrong number of cells");
  auto coords = out.GetCoordinateSystem().GetDataAsMultiplexer();
  auto ptPortal = coords.ReadPortal();

  if (fType == STREAMLINE || fType == PATHLINE)
  {
    vtkm::cont::CellSetExplicit<> explicitCells;
    VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetExplicit<>>(), "Wrong cell type.");
    explicitCells = dcells.AsCellSet<vtkm::cont::CellSetExplicit<>>();
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
  else if (fType == PARTICLE_ADVECTION)
  {
    VTKM_TEST_ASSERT(out.GetNumberOfPoints() == numSeeds, "Wrong number of coordinates");
    for (vtkm::Id i = 0; i < numSeeds; i++)
      VTKM_TEST_ASSERT(xMaxRange.Contains(ptPortal.Get(i)[0]), "Wrong end point for seed");
  }
}

void TestPartitionedDataSet(vtkm::Id nPerRank, bool useGhost, FilterType fType, bool useThreaded)
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
  if (useGhost)
    std::cout << " - using ghost cells";
  if (useThreaded)
    std::cout << " - using threaded";
  std::cout << " - on a partitioned data set" << std::endl;

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

  vtkm::FloatDefault time0 = 0;
  vtkm::FloatDefault time1 = (numDims - 1) * (nPerRank * comm.size());

  std::vector<vtkm::Bounds> bounds;
  for (vtkm::Id i = 0; i < nPerRank; i++)
  {
    bounds.push_back(vtkm::Bounds(x0, x1, y0, y1, z0, z1));
    x0 += dx;
    x1 += dx;
  }

  std::vector<vtkm::Range> xMaxRanges;
  for (const auto& b : bounds)
  {
    auto xMax = b.X.Max;
    if (useGhost)
      xMax = xMax - 1;
    xMaxRanges.push_back(vtkm::Range(xMax, xMax + static_cast<vtkm::FloatDefault>(.5)));
  }


  std::vector<vtkm::cont::PartitionedDataSet> allPDS, allPDS2;
  const vtkm::Id3 dims(numDims, numDims, numDims);
  allPDS = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, useGhost);
  allPDS2 = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, useGhost);

  vtkm::Vec3f vecX(1, 0, 0);
  std::string fieldName = "vec";
  vtkm::FloatDefault stepSize = 0.1f;
  vtkm::Id numSteps = 100000;
  for (std::size_t n = 0; n < allPDS.size(); n++)
  {
    auto pds = allPDS[n];
    AddVectorFields(pds, fieldName, vecX);

    vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
    seedArray = vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                               vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1) });
    vtkm::Id numSeeds = seedArray.GetNumberOfValues();

    if (fType == STREAMLINE)
    {
      vtkm::filter::flow::Streamline streamline;
      SetFilter(streamline, stepSize, numSteps, fieldName, seedArray, useThreaded);
      auto out = streamline.Execute(pds);

      for (vtkm::Id i = 0; i < nPerRank; i++)
        ValidateOutput(out.GetPartition(i), numSeeds, xMaxRanges[i], fType);
    }
    else if (fType == PARTICLE_ADVECTION)
    {
      vtkm::filter::flow::ParticleAdvection particleAdvection;
      SetFilter(particleAdvection, stepSize, numSteps, fieldName, seedArray, useThreaded);
      auto out = particleAdvection.Execute(pds);

      //Particles end up in last rank.
      if (comm.rank() == comm.size() - 1)
      {
        VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 1, "Wrong number of partitions in output");
        ValidateOutput(out.GetPartition(0), numSeeds, xMaxRanges[xMaxRanges.size() - 1], fType);
      }
      else
        VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 0, "Wrong number of partitions in output");
    }
    else if (fType == PATHLINE)
    {
      auto pds2 = allPDS2[n];
      AddVectorFields(pds2, fieldName, vecX);

      vtkm::filter::flow::Pathline pathline;
      SetFilter(pathline, stepSize, numSteps, fieldName, seedArray, useThreaded);

      pathline.SetPreviousTime(time0);
      pathline.SetNextTime(time1);
      pathline.SetNextDataSet(pds2);

      auto out = pathline.Execute(pds);
      for (vtkm::Id i = 0; i < nPerRank; i++)
        ValidateOutput(out.GetPartition(i), numSeeds, xMaxRanges[i], fType);
    }
  }
}

void TestStreamlineFiltersMPI()
{
  std::vector<bool> flags = { true, false };
  std::vector<FilterType> filterTypes = { PARTICLE_ADVECTION, STREAMLINE, PATHLINE };

  for (int n = 1; n < 3; n++)
  {
    for (auto useGhost : flags)
      for (auto fType : filterTypes)
        for (auto useThreaded : flags)
          TestPartitionedDataSet(n, useGhost, fType, useThreaded);
  }

  for (auto fType : filterTypes)
    for (auto useThreaded : flags)
      TestAMRStreamline(fType, useThreaded);
}
}

int UnitTestStreamlineFilterMPI(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamlineFiltersMPI, argc, argv);
}
