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
#include <vtkm/worklet/testing/GenerateTestDataSets.h>

namespace
{

std::vector<vtkm::cont::DataSet> MakeDataSets()
{
  vtkm::Bounds bounds(0, 10, 0, 10, 0, 10);
  const vtkm::Id3 dims(8, 8, 8);
  auto dataSets = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, false);

  vtkm::Id numPoints = dims[0] * dims[1] * dims[2];

  for (auto& ds : dataSets)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec3f_64> velocityField;
    velocityField.Allocate(numPoints);

    auto velocityPortal = velocityField.WritePortal();
    vtkm::Id count = 0;
    for (vtkm::Id i = 0; i < dims[0]; i++)
      for (vtkm::Id j = 0; j < dims[1]; j++)
        for (vtkm::Id k = 0; k < dims[2]; k++)
        {
          velocityPortal.Set(count, vtkm::Vec3f_64(0.1, 0.1, 0.1));
          count++;
        }
    ds.AddPointField("velocity", velocityField);
  }

  return dataSets;
}

void TestLagrangianFilterMultiStepInterval()
{
  vtkm::Id maxCycles = 5;
  vtkm::Id write_interval = 5;
  vtkm::filter::Lagrangian lagrangianFilter2;
  lagrangianFilter2.SetResetParticles(true);
  lagrangianFilter2.SetStepSize(0.1f);
  lagrangianFilter2.SetWriteFrequency(write_interval);

  auto dataSets = MakeDataSets();
  for (auto& input : dataSets)
  {
    for (vtkm::Id i = 1; i <= maxCycles; i++)
    {
      lagrangianFilter2.SetActiveField("velocity");
      vtkm::cont::DataSet extractedBasisFlows = lagrangianFilter2.Execute(input);
      if (i % write_interval == 0)
      {
        VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfCoordinateSystems() == 1,
                         "Wrong number of coordinate systems in the output dataset.");
        VTKM_TEST_ASSERT(extractedBasisFlows.GetNumberOfPoints() == 512,
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
}

} //namespace

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
