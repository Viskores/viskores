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
#include <vtkm/testing/TestingAlgorithms.h>

#include <vtkm/cont/kokkos/DeviceAdapterKokkos.h>

int UnitTestKokkosAlgorithms(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(
    RunAlgorithmsTests<vtkm::cont::DeviceAdapterTagKokkos>, argc, argv);
}
