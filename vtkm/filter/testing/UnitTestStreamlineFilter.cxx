//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/ParticleAdvection.h>
#include <vtkm/filter/Pathline.h>
#include <vtkm/filter/Streamline.h>
#include <vtkm/io/VTKDataSetReader.h>
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

void AddVectorFields(vtkm::cont::PartitionedDataSet& pds,
                     const std::string& fieldName,
                     const vtkm::Vec3f& vec)
{
  for (auto& ds : pds)
    ds.AddPointField(fieldName, CreateConstantVectorField(ds.GetNumberOfPoints(), vec));
}

void TestStreamline()
{
  const vtkm::Id3 dims(5, 5, 5);
  const vtkm::Bounds bounds(0, 4, 0, 4, 0, 4);
  const vtkm::Vec3f vecX(1, 0, 0);
  std::string fieldName = "vec";

  auto dataSets = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, false);
  for (auto& ds : dataSets)
  {
    auto vecField = CreateConstantVectorField(ds.GetNumberOfPoints(), vecX);
    ds.AddPointField(fieldName, vecField);

    vtkm::cont::ArrayHandle<vtkm::Particle> seedArray =
      vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                     vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1),
                                     vtkm::Particle(vtkm::Vec3f(.2f, 3.0f, .2f), 2) });

    vtkm::filter::Streamline streamline;

    streamline.SetStepSize(0.1f);
    streamline.SetNumberOfSteps(20);
    streamline.SetSeeds(seedArray);

    streamline.SetActiveField(fieldName);
    auto output = streamline.Execute(ds);

    //Validate the result is correct.
    VTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                     "Wrong number of coordinate systems in the output dataset");

    vtkm::cont::CoordinateSystem coords = output.GetCoordinateSystem();
    VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 63, "Wrong number of coordinates");

    vtkm::cont::DynamicCellSet dcells = output.GetCellSet();
    VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 3, "Wrong number of cells");
  }
}

void TestPathline()
{
  const vtkm::Id3 dims(5, 5, 5);
  const vtkm::Vec3f vecX(1, 0, 0);
  const vtkm::Vec3f vecY(0, 1, 0);
  const vtkm::Bounds bounds(0, 4, 0, 4, 0, 4);
  std::string fieldName = "vec";

  auto dataSets1 = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, false);
  auto dataSets2 = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, false);

  std::size_t numDS = dataSets1.size();
  for (std::size_t i = 0; i < numDS; i++)
  {
    auto ds1 = dataSets1[i];
    auto ds2 = dataSets2[i];

    auto vecField1 = CreateConstantVectorField(ds1.GetNumberOfPoints(), vecX);
    auto vecField2 = CreateConstantVectorField(ds1.GetNumberOfPoints(), vecY);
    ds1.AddPointField(fieldName, vecField1);
    ds2.AddPointField(fieldName, vecField2);

    vtkm::cont::ArrayHandle<vtkm::Particle> seedArray =
      vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                     vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1),
                                     vtkm::Particle(vtkm::Vec3f(.2f, 3.0f, .2f), 2) });

    vtkm::filter::Pathline pathline;

    pathline.SetPreviousTime(0.0f);
    pathline.SetNextTime(1.0f);
    pathline.SetNextDataSet(ds2);
    pathline.SetStepSize(static_cast<vtkm::FloatDefault>(0.05f));
    pathline.SetNumberOfSteps(20);
    pathline.SetSeeds(seedArray);

    pathline.SetActiveField(fieldName);
    auto output = pathline.Execute(ds1);

    //Validate the result is correct.
    vtkm::cont::CoordinateSystem coords = output.GetCoordinateSystem();
    VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 63, "Wrong number of coordinates");

    vtkm::cont::DynamicCellSet dcells = output.GetCellSet();
    VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 3, "Wrong number of cells");
  }
}

void TestPartitionedDataSet(vtkm::Id num, bool useGhost, bool useSL)
{
  vtkm::Id numDims = 5;
  vtkm::FloatDefault x0 = 0;
  vtkm::FloatDefault x1 = x0 + static_cast<vtkm::FloatDefault>(numDims - 1);
  vtkm::FloatDefault dx = x1 - x0;
  vtkm::FloatDefault y0 = 0, y1 = static_cast<vtkm::FloatDefault>(numDims - 1);
  vtkm::FloatDefault z0 = 0, z1 = static_cast<vtkm::FloatDefault>(numDims - 1);

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
  for (vtkm::Id i = 0; i < num; i++)
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

      for (vtkm::Id i = 0; i < num; i++)
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

        vtkm::FloatDefault xMax =
          static_cast<vtkm::FloatDefault>(bounds[static_cast<std::size_t>(i)].X.Max);
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

      VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 1, "Wrong number of partitions in output");
      auto ds = out.GetPartition(0);
      //Validate the result is correct.
      VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset");

      vtkm::FloatDefault xMax = static_cast<vtkm::FloatDefault>(bounds[bounds.size() - 1].X.Max);
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
  }
}

template <typename CellSetType, typename CoordsType>
void ValidateEndPoints(const CellSetType& cellSet,
                       const CoordsType& coords,
                       vtkm::Id numPoints,
                       const std::vector<vtkm::Vec3f>& endPts)
{
  const vtkm::FloatDefault eps = static_cast<vtkm::FloatDefault>(1e-3);
  auto cPortal = coords.ReadPortal();

  for (vtkm::Id i = 0; i < numPoints; i++)
  {
    vtkm::Id numPts = cellSet.GetNumberOfPointsInCell(i);
    std::vector<vtkm::Id> ids(static_cast<std::size_t>(numPts));
    cellSet.GetCellPointIds(i, ids.data());

    vtkm::Vec3f e = endPts[static_cast<std::size_t>(i)];
    vtkm::Vec3f pt = cPortal.Get(ids[ids.size() - 1]);
    VTKM_TEST_ASSERT(vtkm::Magnitude(pt - e) <= eps, "Particle advection point is wrong");
  }
}

void TestStreamlineFile(const std::string& fname,
                        const std::vector<vtkm::Vec3f>& pts,
                        vtkm::FloatDefault stepSize,
                        vtkm::Id maxSteps,
                        const std::vector<vtkm::Vec3f>& endPts,
                        bool useSL)
{
  vtkm::io::VTKDataSetReader reader(fname);
  vtkm::cont::DataSet ds;
  try
  {
    ds = reader.ReadDataSet();
    VTKM_TEST_ASSERT(ds.HasField("vec"));
  }
  catch (vtkm::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }
  vtkm::Id numPoints = static_cast<vtkm::Id>(pts.size());

  std::vector<vtkm::Particle> seeds;
  for (vtkm::Id i = 0; i < numPoints; i++)
    seeds.push_back(vtkm::Particle(pts[static_cast<std::size_t>(i)], i));
  auto seedArray = vtkm::cont::make_ArrayHandle(seeds, vtkm::CopyFlag::Off);

  vtkm::cont::DataSet output;
  if (useSL)
  {
    vtkm::filter::Streamline streamline;
    streamline.SetStepSize(stepSize);
    streamline.SetNumberOfSteps(maxSteps);
    streamline.SetSeeds(seedArray);
    streamline.SetActiveField("vec");
    output = streamline.Execute(ds);
  }
  else
  {
    vtkm::filter::ParticleAdvection particleAdvection;
    particleAdvection.SetStepSize(stepSize);
    particleAdvection.SetNumberOfSteps(maxSteps);
    particleAdvection.SetSeeds(seedArray);
    particleAdvection.SetActiveField("vec");
    output = particleAdvection.Execute(ds);
  }

  auto coords = output.GetCoordinateSystem().GetDataAsMultiplexer();
  vtkm::cont::DynamicCellSet dcells = output.GetCellSet();
  VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numPoints, "Wrong number of cells");

  if (useSL)
  {
    VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetExplicit<>>(), "Wrong cell type");
    auto cells = dcells.Cast<vtkm::cont::CellSetExplicit<>>();
    ValidateEndPoints(cells, coords, numPoints, endPts);
  }
  else
  {
    VTKM_TEST_ASSERT(dcells.IsType<vtkm::cont::CellSetSingleType<>>(), "Wrong cell type");
    auto cells = dcells.Cast<vtkm::cont::CellSetSingleType<>>();
    ValidateEndPoints(cells, coords, numPoints, endPts);
  }
}

void TestStreamlineFilters()
{
  std::vector<bool> flags = { true, false };
  for (int n = 1; n < 3; n++)
  {
    for (auto useGhost : flags)
      for (auto useSL : flags)
        TestPartitionedDataSet(n, useGhost, useSL);
  }

  TestStreamline();
  TestPathline();

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

  for (auto useSL : flags)
    TestStreamlineFile(fusionFile, fusionPts, fusionStep, 1000, fusionEndPts, useSL);

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

  for (auto useSL : flags)
    TestStreamlineFile(fishFile, fishPts, fishStep, 100, fishEndPts, useSL);
}
}

int UnitTestStreamlineFilter(int argc, char* argv[])
{
  // Setup MPI environment: This test is not intendent to be run in parallel
  // but filter does make MPI calls
  vtkmdiy::mpi::environment env(argc, argv);
  return vtkm::cont::testing::Testing::Run(TestStreamlineFilters, argc, argv);
}
