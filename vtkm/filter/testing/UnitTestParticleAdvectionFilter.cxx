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
#include <vtkm/io/VTKDataSetReader.h>

namespace
{
vtkm::cont::DataSet CreateDataSet(const vtkm::Id3& dims, const vtkm::Vec3f& vec)
{
  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];

  std::vector<vtkm::Vec3f> vectorField(static_cast<std::size_t>(numPoints));
  for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    vectorField[i] = vec;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims);
  ds.AddPointField("vector", vectorField);

  return ds;
}

void TestBasic()
{
  const vtkm::Id3 dims(5, 5, 5);
  const vtkm::Vec3f vecX(1, 0, 0);

  vtkm::cont::DataSet ds = CreateDataSet(dims, vecX);
  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  std::vector<vtkm::Particle> seeds(3);
  seeds[0] = vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0);
  seeds[1] = vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1);
  seeds[2] = vtkm::Particle(vtkm::Vec3f(.2f, 3.0f, .2f), 2);

  seedArray = vtkm::cont::make_ArrayHandle(seeds);

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
  VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 3, "Wrong number of coordinates");

  vtkm::cont::DynamicCellSet dcells = output.GetCellSet();
  VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 3, "Wrong number of cells");
}

void TestFile(const std::string& fname,
              const std::vector<vtkm::Vec3f>& pts,
              vtkm::FloatDefault stepSize,
              vtkm::Id maxSteps,
              const std::vector<vtkm::Vec3f>& endPts)
{
  vtkm::io::VTKDataSetReader reader(fname);
  vtkm::cont::DataSet ds;
  try
  {
    ds = reader.ReadDataSet();
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
  auto seedArray = vtkm::cont::make_ArrayHandle(seeds);

  vtkm::filter::ParticleAdvection particleAdvection;
  particleAdvection.SetStepSize(stepSize);
  particleAdvection.SetNumberOfSteps(maxSteps);
  particleAdvection.SetSeeds(seedArray);

  particleAdvection.SetActiveField("vec");
  auto output = particleAdvection.Execute(ds);

  auto coords = output.GetCoordinateSystem().GetData();
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

void TestParticleAdvectionFilter()
{
  TestBasic();

  std::string basePath = vtkm::cont::testing::Testing::GetTestDataBasePath();

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
  std::string fusionFile = basePath + "/rectilinear/fusion.vtk";

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
  std::string fishFile = basePath + "/rectilinear/fishtank.vtk";

  TestFile(fishFile, fishPts, fishStep, 100, fishEndPts);
}
}

int UnitTestParticleAdvectionFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestParticleAdvectionFilter, argc, argv);
}
