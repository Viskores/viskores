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
               bool useThreaded,
               bool useAsyncComm,
               bool useBlockIds,
               const std::vector<vtkm::Id>& blockIds)
{
  filter.SetStepSize(stepSize);
  filter.SetNumberOfSteps(numSteps);
  filter.SetSeeds(seedArray);
  filter.SetActiveField(fieldName);
  filter.SetUseThreadedAlgorithm(useThreaded);
  if (useAsyncComm)
    filter.SetUseAsynchronousCommunication();
  else
    filter.SetUseSynchronousCommunication();

  if (useBlockIds)
    filter.SetBlockIDs(blockIds);
}

void TestAMRStreamline(FilterType fType, bool useThreaded, bool useAsyncComm)
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
    if (useThreaded)
      std::cout << " - using threaded";
    if (useAsyncComm)
      std::cout << " - usingAsyncComm";
    else
      std::cout << " - usingSyncComm";

    std::cout << " - on an AMR data set" << std::endl;
  }

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
        SetFilter(streamline,
                  stepSize,
                  numSteps,
                  fieldName,
                  seedArray,
                  useThreaded,
                  useAsyncComm,
                  false,
                  {});
        out = streamline.Execute(pds);
      }
      else if (fType == PATHLINE)
      {
        vtkm::filter::flow::Pathline pathline;
        SetFilter(
          pathline, stepSize, numSteps, fieldName, seedArray, useThreaded, useAsyncComm, false, {});
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

void DoTest()
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  if (comm.rank() == 0)
  {
    std::cout << std::endl << "*** TestStreamlineAMRMPI" << std::endl;
  }

  for (auto fType : { PARTICLE_ADVECTION, STREAMLINE, PATHLINE })
  {
    for (auto useThreaded : { true, false })
    {
      for (auto useAsyncComm : { true, false })
      {
        TestAMRStreamline(fType, useThreaded, useAsyncComm);
      }
    }
  }
}

} // anonymous namespace

int UnitTestStreamlineAMRMPI(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
