//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/Streamline.h>

namespace
{
}

void TestStreamline()
{
  //Create a datset.
  const vtkm::Id3 dims(5, 5, 5);
  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];

  std::vector<vtkm::Vec<vtkm::FloatDefault, 3>> vectorField(static_cast<std::size_t>(numPoints));
  for (std::size_t i = 0; i < static_cast<std::size_t>(numPoints); i++)
    vectorField[i] = vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0);

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSetFieldAdd dataSetField;

  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims);
  dataSetField.AddPointField(ds, "vector", vectorField);

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> seedArray;
  std::vector<vtkm::Vec<vtkm::FloatDefault, 3>> seeds(3);
  seeds[0] = vtkm::Vec<vtkm::FloatDefault, 3>(.2f, 1.0f, .2f);
  seeds[1] = vtkm::Vec<vtkm::FloatDefault, 3>(.2f, 2.0f, .2f);
  seeds[2] = vtkm::Vec<vtkm::FloatDefault, 3>(.2f, 3.0f, .2f);

  seedArray = vtkm::cont::make_ArrayHandle(seeds);

  vtkm::filter::Streamline streamline;

  streamline.SetStepSize(0.1);
  streamline.SetNumberOfSteps(20);
  streamline.SetSeeds(seedArray);
  vtkm::cont::Field vecField = ds.GetField("vector");

  streamline.SetActiveField("vector");
  auto output = streamline.Execute(ds);

  //Validate the result is correct.
  VTKM_TEST_ASSERT(output.GetNumberOfCellSets() == 1,
                   "Wrong number of cellsets in the output dataset");
  VTKM_TEST_ASSERT(output.GetNumberOfCoordinateSystems() == 1,
                   "Wrong number of coordinate systems in the output dataset");

  vtkm::cont::CoordinateSystem coords = output.GetCoordinateSystem();
  VTKM_TEST_ASSERT(coords.GetData().GetNumberOfValues() == 60, "Wrong number of coordinates");

  vtkm::cont::DynamicCellSet dcells = output.GetCellSet();
  VTKM_TEST_ASSERT(dcells.GetNumberOfCells() == 3, "Wrong number of cells");
}

int UnitTestStreamlineFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestStreamline);
}
