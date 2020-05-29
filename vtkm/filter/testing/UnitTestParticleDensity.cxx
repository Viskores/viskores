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
#include <vtkm/filter/ParticleDensityNGP.h>

#include <vector>

void TestNGP()
{
  std::vector<vtkm::Vec3f> positions = { { 0.5, 0.5 } };
}

void TestParticleDensity()
{
  TestNGP();
}

int UnitTestParticleDensity(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestParticleDensity, argc, argv);
}