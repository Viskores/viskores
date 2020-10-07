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
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/ParticleAdvection.h>

#include <vtkm/thirdparty/diy/diy.h>

namespace
{
vtkm::cont::DataSet CreateDataSet(const vtkm::Id3& dims,
                                  const vtkm::Vec3f& origin,
                                  const vtkm::Vec3f& spacing,
                                  const vtkm::Vec3f& vec)
{
  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];

  std::vector<vtkm::Vec3f> vectorField(static_cast<std::size_t>(numPoints));
  for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    vectorField[i] = vec;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims, origin, spacing);
  ds.AddPointField("vector", vectorField);

  return ds;
}

void TestPartitionedDataSet(int nPerRank)
{
  const vtkm::Id3 dims(5, 5, 5);
  const vtkm::Vec3f spacing(1, 1, 1);
  const vtkm::Vec3f vecX(1, 0, 0);

  vtkm::cont::PartitionedDataSet pds;

  //Create nPerRank partitions per rank with X=4*rank.
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  for (int i = 0; i < nPerRank; i++)
  {
    vtkm::FloatDefault x = static_cast<vtkm::FloatDefault>(4 * (nPerRank * comm.rank() + i));
    vtkm::Vec3f origin(x, 0, 0);
    pds.AppendPartition(CreateDataSet(dims, origin, spacing, vecX));
  }

  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  seedArray = vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                             vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1) });


  vtkm::Id numSeeds = seedArray.GetNumberOfValues();

  vtkm::filter::ParticleAdvection particleAdvection;

  particleAdvection.SetStepSize(0.1f);
  particleAdvection.SetNumberOfSteps(100000);
  particleAdvection.SetSeeds(seedArray);

  particleAdvection.SetActiveField("vector");
  auto out = particleAdvection.Execute(pds);

  //Particles end up in last rank.
  if (comm.rank() == comm.size() - 1)
  {
    vtkm::FloatDefault globalMaxX = static_cast<vtkm::FloatDefault>(nPerRank * comm.size() * 4);
    VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 1, "Wrong number of partitions in output");
    auto ds = out.GetPartition(0);
    //Validate the result is correct.
    VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1,
                     "Wrong number of coordinate systems in the output dataset");

    auto coords = ds.GetCoordinateSystem().GetDataAsMultiplexer();
    VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == numSeeds, "Wrong number of coordinates");
    auto ptPortal = coords.ReadPortal();
    for (vtkm::Id i = 0; i < numSeeds; i++)
      VTKM_TEST_ASSERT(ptPortal.Get(i)[0] >= globalMaxX);

    vtkm::cont::DynamicCellSet dcells = ds.GetCellSet();
    VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == numSeeds, "Wrong number of cells");
  }
  else
    VTKM_TEST_ASSERT(out.GetNumberOfPartitions() == 0, "Wrong number of partitions in output");
}

void TestParticleAdvectionFilterMPI()
{
  for (int n = 1; n < 10; n++)
    TestPartitionedDataSet(n);
}
}

int UnitTestParticleAdvectionFilterMPI(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestParticleAdvectionFilterMPI, argc, argv);
}
