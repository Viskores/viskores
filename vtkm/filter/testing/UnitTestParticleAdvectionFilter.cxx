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
#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/ParticleAdvection.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/thirdparty/diy/environment.h>
#include <vtkm/worklet/testing/GenerateTestDataSets.h>

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

void TestBasic()
{
  std::cout << "Basic Tests" << std::endl;

  const vtkm::Id3 dims(5, 5, 5);
  const vtkm::Bounds bounds(0, dims[0] - 1, 0, dims[1] - 1, 0, dims[2] - 1);
  const vtkm::Vec3f vecX(1, 0, 0);

  //Test datasets with and without ghost cells.
  for (int ghostType = 0; ghostType < 2; ghostType++)
  {
    bool addGhost = (ghostType == 1);
    vtkm::Id3 useDims;
    vtkm::Bounds useBounds;
    if (addGhost)
    {
      useDims = dims + vtkm::Id3(2, 2, 2);
      useBounds.X.Min = bounds.X.Min - 1;
      useBounds.X.Max = bounds.X.Max + 1;
      useBounds.Y.Min = bounds.Y.Min - 1;
      useBounds.Y.Max = bounds.Y.Max + 1;
      useBounds.Z.Min = bounds.Z.Min - 1;
      useBounds.Z.Max = bounds.Z.Max + 1;
    }
    else
    {
      useDims = dims;
      useBounds = bounds;
    }

    auto dataSets = vtkm::worklet::testing::CreateAllDataSets(useBounds, useDims, addGhost);

    for (auto& ds : dataSets)
    {
      auto vecField = CreateConstantVectorField(ds.GetNumberOfPoints(), vecX);
      ds.AddPointField("vector", vecField);

      const vtkm::FloatDefault x0(0.2);
      std::vector<vtkm::Particle> seeds = { vtkm::Particle(vtkm::Vec3f(x0, 1, 1), 0),
                                            vtkm::Particle(vtkm::Vec3f(x0, 2, 1), 1),
                                            vtkm::Particle(vtkm::Vec3f(x0, 3, 1), 2),
                                            vtkm::Particle(vtkm::Vec3f(x0, 3, 2), 3) };

      auto seedArray = vtkm::cont::make_ArrayHandle(seeds);

      vtkm::filter::ParticleAdvection particleAdvection;
      particleAdvection.SetStepSize(0.1f);
      particleAdvection.SetNumberOfSteps(20);
      particleAdvection.SetSeeds(seedArray);

      particleAdvection.SetActiveField("vector");
      auto output = particleAdvection.Execute(ds);

      //Validate the result is correct.
      VTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");

      vtkm::cont::CoordinateSystem coords = output.GetCoordinateSystem();
      VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 4, "Wrong number of coordinates");

      vtkm::cont::DynamicCellSet dcells = output.GetCellSet();
      VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 4, "Wrong number of cells");
    }
  }
}

void TestPartitionedDataSet()
{
  std::cout << "Partitioned data set" << std::endl;

  const vtkm::Id3 dimensions(5, 5, 5);
  const vtkm::Vec3f vecX(1, 0, 0);
  std::vector<vtkm::Bounds> bounds = { vtkm::Bounds(0, 4, 0, 4, 0, 4),
                                       vtkm::Bounds(4, 8, 0, 4, 0, 4),
                                       vtkm::Bounds(8, 12, 0, 4, 0, 4) };

  vtkm::Bounds globalBounds;
  for (auto& b : bounds)
    globalBounds.Include(b);

  const std::string fieldName = "vec";

  for (int gType = 1; gType < 2; gType++)
  {
    bool addGhost = (gType == 1);
    vtkm::Id3 useDims;
    std::vector<vtkm::Bounds> useBounds;
    vtkm::Bounds boundsWithGhosts;

    if (addGhost)
    {
      useDims = dimensions + vtkm::Id3(2, 2, 2);
      for (auto& b : bounds)
      {
        vtkm::Bounds b2;
        b2.X.Min = b.X.Min - 1;
        b2.X.Max = b.X.Max + 1;
        b2.Y.Min = b.Y.Min - 1;
        b2.Y.Max = b.Y.Max + 1;
        b2.Z.Min = b.Z.Min - 1;
        b2.Z.Max = b.Z.Max + 1;
        useBounds.push_back(b2);

        boundsWithGhosts.Include(b2);
      }
    }
    else
    {
      useDims = dimensions;
      useBounds = bounds;
    }
    auto allPDs = vtkm::worklet::testing::CreateAllDataSets(useBounds, useDims, addGhost);

    for (auto& pds : allPDs)
    {
      for (auto& ds : pds)
      {
        auto vecField = CreateConstantVectorField(ds.GetNumberOfPoints(), vecX);
        ds.AddPointField(fieldName, vecField);
      }

      std::cout << "RUN ON: " << std::endl << std::endl;
      pds.PrintSummary(std::cout);
      std::cout << std::endl << std::endl << std::endl << std::endl;

      vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
      seedArray = vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                                 vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1),
                                                 vtkm::Particle(vtkm::Vec3f(4.2f, 1.0f, .2f), 2),
                                                 vtkm::Particle(vtkm::Vec3f(8.2f, 1.0f, .2f), 3) });

      vtkm::Id numSeeds = seedArray.GetNumberOfValues();

      vtkm::filter::ParticleAdvection particleAdvection;

      particleAdvection.SetStepSize(0.1f);
      particleAdvection.SetNumberOfSteps(1000);
      particleAdvection.SetSeeds(seedArray);

      particleAdvection.SetActiveField(fieldName);
      auto out = particleAdvection.Execute(pds);

      VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 1, "Wrong number of partitions in output");
      auto ds = out.GetPartition(0);

      //Validate the result is correct.
      VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");

      auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();

      VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == numSeeds, "Wrong number of coordinates");
      auto ptPortal = coords.ReadPortal();
      vtkm::Id nPts = ptPortal.GetNumberOfValues();
      for (vtkm::Id j = 0; j < nPts; j++)
      {
        VTKM_TEST_ASSERT(!globalBounds.Contains(ptPortal.Get(j)), "End point not oustide bounds");
        if (addGhost)
          VTKM_TEST_ASSERT(boundsWithGhosts.Contains(ptPortal.Get(j)),
                           "End point not inside bounds with ghosts");
      }

      vtkm::cont::DynamicCellSet dcells = ds.GetCellSet();
      VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds, "Wrong number of cells");
    }
  }
}

void TestDataSet(const vtkm::cont::DataSet& dataset,
                 const std::vector<vtkm::Vec3f>& pts,
                 vtkm::FloatDefault stepSize,
                 vtkm::Id maxSteps,
                 const std::vector<vtkm::Vec3f>& endPts)
{
  vtkm::Id numPoints = static_cast<vtkm::Id>(pts.size());

  std::vector<vtkm::Particle> seeds;
  for (vtkm::Id i = 0; i < numPoints; i++)
    seeds.push_back(vtkm::Particle(pts[static_cast<std::size_t>(i)], i));
  auto seedArray = vtkm::cont::make_ArrayHandle(seeds, vtkm::CopyFlag::On);

  vtkm::filter::ParticleAdvection particleAdvection;
  particleAdvection.SetStepSize(stepSize);
  particleAdvection.SetNumberOfSteps(maxSteps);
  particleAdvection.SetSeeds(seedArray);

  particleAdvection.SetActiveField("vec");
  auto output = particleAdvection.Execute(dataset);

  auto coords = output.GetCoordinateSystem().GetDataAsMultiplexer();
  vtkm::cont::DynamicCellSet dcells = output.GetCellSet();
  VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numPoints, "Wrong number of cells");
  VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetSingleType<>>(), "Wrong cell type");
  vtkm::cont::ArrayHandle<vtkm::Vec3f> coordPts;
  auto cPortal = coords.ReadPortal();

  const vtkm::FloatDefault eps = static_cast<vtkm::FloatDefault>(1e-3);
  for (vtkm::Id i = 0; i < numPoints; i++)
  {
    vtkm::Vec3f e = endPts[static_cast<std::size_t>(i)];
    vtkm::Vec3f pt = cPortal.Get(i);
    VTKM_TEST_ASSERT(vtkm::Magnitude(pt - e) <= eps, "Particle advection point is wrong");
  }
}

void TestFile(const std::string& fname,
              const std::vector<vtkm::Vec3f>& pts,
              vtkm::FloatDefault stepSize,
              vtkm::Id maxSteps,
              const std::vector<vtkm::Vec3f>& endPts)
{
  std::cout << fname << std::endl;

  vtkm::io::VTKDataSetReader reader(fname);
  vtkm::cont::DataSet dataset;
  try
  {
    dataset = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }

  TestDataSet(dataset, pts, stepSize, maxSteps, endPts);

  std::cout << "  as explicit grid" << std::endl;
  vtkm::filter::CleanGrid clean;
  clean.SetCompactPointFields(false);
  clean.SetMergePoints(false);
  clean.SetRemoveDegenerateCells(false);
  vtkm::cont::DataSet explicitData = clean.Execute(dataset);

  TestDataSet(explicitData, pts, stepSize, maxSteps, endPts);
}

void TestParticleAdvectionFilter()
{
  TestBasic();
  TestPartitionedDataSet();

  //Fusion test.
  std::vector<vtkm::Vec3f> fusionPts, fusionEndPts;
  fusionPts.push_back(vtkm::Vec3f(0.8f, 0.6f, 0.6f));
  fusionPts.push_back(vtkm::Vec3f(0.8f, 0.8f, 0.6f));
  fusionPts.push_back(vtkm::Vec3f(0.8f, 0.8f, 0.3f));
  //End point values were generated in VisIt.
  fusionEndPts.push_back(vtkm::Vec3f(0.5335789918f, 0.87112802267f, 0.6723330020f));
  fusionEndPts.push_back(vtkm::Vec3f(0.5601879954f, 0.91389900446f, 0.43989110522f));
  fusionEndPts.push_back(vtkm::Vec3f(0.7004770041f, 0.63193398714f, 0.64524400234f));
  vtkm::FloatDefault fusionStep = 0.005f;
  std::string fusionFile = vtkm::cont::testing::Testing::DataPath("rectilinear/fusion.vtk");

  TestFile(fusionFile, fusionPts, fusionStep, 1000, fusionEndPts);

  //Fishtank test.
  std::vector<vtkm::Vec3f> fishPts, fishEndPts;
  fishPts.push_back(vtkm::Vec3f(0.75f, 0.5f, 0.01f));
  fishPts.push_back(vtkm::Vec3f(0.4f, 0.2f, 0.7f));
  fishPts.push_back(vtkm::Vec3f(0.5f, 0.3f, 0.8f));
  //End point values were generated in VisIt.
  fishEndPts.push_back(vtkm::Vec3f(0.7734669447f, 0.4870159328f, 0.8979591727f));
  fishEndPts.push_back(vtkm::Vec3f(0.7257543206f, 0.1277695596f, 0.7468645573f));
  fishEndPts.push_back(vtkm::Vec3f(0.8347796798f, 0.1276152730f, 0.4985143244f));
  vtkm::FloatDefault fishStep = 0.001f;
  std::string fishFile = vtkm::cont::testing::Testing::DataPath("rectilinear/fishtank.vtk");

  TestFile(fishFile, fishPts, fishStep, 100, fishEndPts);
}
}

int UnitTestParticleAdvectionFilter(int argc, char* argv[])
{
  // Setup MPI environment: This test is not intendent to be run in parallel
  // but filter does make MPI calls
  vtkmdiy::mpi::environment env(argc, argv);
  return vtkm::cont::testing::Testing::Run(TestParticleAdvectionFilter, argc, argv);
}
