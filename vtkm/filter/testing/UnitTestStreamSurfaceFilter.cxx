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
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/StreamSurface.h>

namespace
{

vtkm::cont::DataSet CreateDataSet(const vtkm::Id3& dims, const vtkm::Vec3f& vec)
{
  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];

  std::vector<vtkm::Vec3f> vectorField(static_cast<std::size_t>(numPoints));
  for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    vectorField[i] = vec;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSetFieldAdd dataSetField;

  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims);
  dataSetField.AddPointField(ds, "vector", vectorField);

  return ds;
}

void TestStreamSurface()
{
  std::cout << "Testing Stream Surface Filter" << std::endl;

  const vtkm::Id3 dims(5, 5, 5);
  const vtkm::Vec3f vecX(1, 0, 0);

  vtkm::cont::DataSet ds = CreateDataSet(dims, vecX);
  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  std::vector<vtkm::Particle> seeds(4);
  seeds[0] = vtkm::Particle(vtkm::Vec3f(.1f, 1.0f, .2f), 0);
  seeds[1] = vtkm::Particle(vtkm::Vec3f(.1f, 2.0f, .1f), 1);
  seeds[2] = vtkm::Particle(vtkm::Vec3f(.1f, 3.0f, .3f), 2);
  seeds[3] = vtkm::Particle(vtkm::Vec3f(.1f, 3.5f, .2f), 3);

  seedArray = vtkm::cont::make_ArrayHandle(seeds);

  vtkm::filter::StreamSurface streamSrf;

  streamSrf.SetStepSize(0.1f);
  streamSrf.SetNumberOfSteps(20);
  streamSrf.SetSeeds(seedArray);
  streamSrf.SetActiveField("vector");

  auto output = streamSrf.Execute(ds);

  //Validate the result is correct.
  VTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");

  vtkm::cont::CoordinateSystem coords = output.GetCoordinateSystem();
  VTKM_TEST_ASSERT(coords.GetNumberOfPoints() == 84, "Wrong number of coordinates");

  vtkm::cont::DynamicCellSet dcells = output.GetCellSet();
  VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 120, "Wrong number of cells");
}
}

int UnitTestStreamSurfaceFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamSurface, argc, argv);
}
