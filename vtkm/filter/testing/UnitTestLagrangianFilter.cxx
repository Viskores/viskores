//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <iostream>
#include <vtkm/cont/CellLocatorBoundingIntervalHierarchy.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
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

  vtkm::Vec3f_64 ORIGIN(0, 0, 0);
  vtkm::Vec3f_64 SPACING(xdiff, ydiff, zdiff);

  vtkm::cont::DataSet dataset = dsb.Create(DIMS, ORIGIN, SPACING);

  vtkm::Id numPoints = DIMS[0] * DIMS[1] * DIMS[2];

  vtkm::cont::ArrayHandle<vtkm::Vec3f_64> velocityField;
  velocityField.Allocate(numPoints);

  auto velocityPortal = velocityField.WritePortal();
  vtkm::Id count = 0;
  for (vtkm::Id i = 0; i < DIMS[0]; i++)
  {
    for (vtkm::Id j = 0; j < DIMS[1]; j++)
    {
      for (vtkm::Id k = 0; k < DIMS[2]; k++)
      {
        velocityPortal.Set(count, vtkm::Vec3f_64(0.1, 0.1, 0.1));
        count++;
      }
    }
  }
  dataset.AddPointField("velocity", velocityField);
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
      VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCoordinateSystems() == 1,
                       "Wrong number of coordinate systems in the output dataset.");
      VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfPoints() == 4096,
                       "Wrong number of basis flows extracted.");
      VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfFields() == 2, "Wrong number of fields.");
    }
    else
    {
      VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfPoints() == 0,
                       "Output dataset should have no points.");
      VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCoordinateSystems() == 0,
                       "Wrong number of coordinate systems in the output dataset.");
      VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfFields() == 0, "Wrong number of fields.");
    }
  }
}

void TestLagrangian()
{
  TestLagrangianFilterMultiStepInterval();

  // This gets around a bug where the LagrangianFilter allows VTK-m to crash during the program
  // exit handlers. The problem is that vtkm/filter/Lagrangian.hxx declares several static
  // ArrayHandles. The developers have been warned that this is a terrible idea for many reasons
  // (c.f. https://gitlab.kitware.com/vtk/vtk-m/-/merge_requests/1945), but this has not been
  // fixed yet. One of the bad things that can happen is that during the C++ exit handler,
  // the static ArrayHandles could be closed after the device APIs, which could lead to errors
  // when it tries to free the memory. This has been seen for this test. This hack gets
  // around it, but eventually these static declarations should really, really, really, really
  // be removed.
  BasisParticles.ReleaseResources();
  BasisParticlesOriginal.ReleaseResources();
  BasisParticlesValidity.ReleaseResources();
}

int UnitTestLagrangianFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestLagrangian, argc, argv);
}
