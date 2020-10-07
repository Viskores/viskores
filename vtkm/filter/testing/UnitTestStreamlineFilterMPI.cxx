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
#include <vtkm/filter/Streamline.h>

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

void TestPartitionedDataSet(vtkm::Id nPerRank)
{
  const vtkm::Id3 dims(5, 5, 5);
  const vtkm::Vec3f spacing(1, 1, 1);
  const vtkm::Vec3f vecX(1, 0, 0);

  vtkm::cont::PartitionedDataSet pds;

  //Create nPerRank partitions per rank with X=4*rank.
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  std::vector<vtkm::Range> XPartitionRanges;
  for (vtkm::Id i = 0; i < nPerRank; i++)
  {
    vtkm::FloatDefault x = static_cast<vtkm::FloatDefault>(4 * (nPerRank * comm.rank() + i));
    vtkm::Vec3f origin(x, 0, 0);
    pds.AppendPartition(CreateDataSet(dims, origin, spacing, vecX));
    XPartitionRanges.push_back(vtkm::Range(x, x + 4));
  }

  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  seedArray = vtkm::cont::make_ArrayHandle({ vtkm::Particle(vtkm::Vec3f(.2f, 1.0f, .2f), 0),
                                             vtkm::Particle(vtkm::Vec3f(.2f, 2.0f, .2f), 1) });
  vtkm::Id numSeeds = seedArray.GetNumberOfValues();

  vtkm::filter::Streamline streamline;

  streamline.SetStepSize(0.1f);
  streamline.SetNumberOfSteps(100000);
  streamline.SetSeeds(seedArray);

  streamline.SetActiveField("vector");
  auto out = streamline.Execute(pds);

  for (vtkm::Id i = 0; i < nPerRank; i++)
  {
    auto inputDS = pds.GetPartition(i);
    auto inputBounds = inputDS.GetCoordinateSystem().GetBounds();
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

    for (vtkm::Id j = 0; j < numSeeds; j++)
    {
      vtkm::cont::ArrayHandle<vtkm::Id> indices;
      explicitCells.GetIndices(j, indices);
      vtkm::Id nPts = indices.GetNumberOfValues();
      auto iPortal = indices.ReadPortal();
      vtkm::Vec3f lastPt = ptPortal.Get(iPortal.Get(nPts - 1));
      VTKM_TEST_ASSERT(lastPt[0] > inputBounds.X.Max, "Wrong end point for seed");
    }
  }
}

void TestStreamlineFiltersMPI()
{
  for (int n = 1; n < 10; n++)
    TestPartitionedDataSet(n);
}
}

int UnitTestStreamlineFilterMPI(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamlineFiltersMPI, argc, argv);
}
