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

#include <iostream>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/Lagrangian.h>

vtkm::cont::DataSet MakeTestUniformDataSet()
{
  vtkm::Float64 xmin, xmax, ymin, ymax, zmin, zmax;
  xmin = 0.0;
  ymin = 0.0;
  zmin = 0.0;

  xmax = 10.0;
  ymax = 10.0;
  zmax = 10.0;

  const vtkm::Id3 DIMS(16, 16, 16);

  vtkm::cont::DataSetBuilderUniform dsb;

  vtkm::Float64 xdiff = (xmax - xmin) / (static_cast<vtkm::Float64>(DIMS[0] - 1));
  vtkm::Float64 ydiff = (ymax - ymin) / (static_cast<vtkm::Float64>(DIMS[1] - 1));
  vtkm::Float64 zdiff = (zmax - zmin) / (static_cast<vtkm::Float64>(DIMS[2] - 1));

  vtkm::Vec<vtkm::Float64, 3> ORIGIN(0, 0, 0);
  vtkm::Vec<vtkm::Float64, 3> SPACING(xdiff, ydiff, zdiff);

  vtkm::cont::DataSet dataset = dsb.Create(DIMS, ORIGIN, SPACING);
  vtkm::cont::DataSetFieldAdd dsf;

  vtkm::Id numPoints = DIMS[0] * DIMS[1] * DIMS[2];

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>> velocityField;
  velocityField.Allocate(numPoints);

  vtkm::Id count = 0;
  for (vtkm::Id i = 0; i < DIMS[0]; i++)
  {
    for (vtkm::Id j = 0; j < DIMS[1]; j++)
    {
      for (vtkm::Id k = 0; k < DIMS[2]; k++)
      {
        velocityField.GetPortalControl().Set(count, vtkm::Vec<vtkm::Float64, 3>(0.1, 0.1, 0.1));
        count++;
      }
    }
  }
  dsf.AddPointField(dataset, "velocity", velocityField);
  return dataset;
}

void TestLagrangianFilterMultiStepInterval()
{
  std::cout << "Test: Lagrangian Analysis - Uniform Dataset - Write Interval > 1" << std::endl;
  vtkm::Id maxCycles = 10;
  vtkm::Id write_interval = 5;
  vtkm::filter::Lagrangian lagrangianFilter2;
  lagrangianFilter2.SetResetParticles(true);
  lagrangianFilter2.SetStepSize(0.1f);
  lagrangianFilter2.SetWriteFrequency(write_interval);
  for (vtkm::Id i = 1; i <= maxCycles; i++)
  {
    vtkm::cont::DataSet input = MakeTestUniformDataSet();
    lagrangianFilter2.SetActiveField("velocity");
    vtkm::cont::DataSet extractedBasisFlows = lagrangianFilter2.Execute(input);
    if (i % write_interval == 0)
    {
      VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCellSets() == 1,
                       "Wrong number of cell sets in the output dataset.");
      VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset.");
      VTKM_TEST_ASSERT(extractedBasisFlows.GetCellSet().GetNumberOfCells() == 3375,
                       "Wrong number of basis flows extracted.");
    }
    else
    {
      VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCellSets() == 0,
                       "Wrong number of cell sets in the output dataset.");
      VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCoordinateSystems() == 0,
                       "Wrong number of coordinate systems in the output dataset.");
    }
  }
}

void TestLagrangian()
{
  TestLagrangianFilterMultiStepInterval();
}

int UnitTestLagrangianFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestLagrangian);
}
