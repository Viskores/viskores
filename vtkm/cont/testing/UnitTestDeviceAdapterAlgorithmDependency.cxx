//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This tests a previous problem where code templated on the device adapter and
// used one of the device adapter algorithms (for example, the dispatcher) had
// to be declared after any device adapter it was ever used with.
#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/cont/ArrayHandle.h>

// Important for this test!
//This file must be included after ArrayHandle.h
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

namespace
{

struct ExampleWorklet
{
  template <typename T>
  void operator()(T vtkmNotUsed(v)) const
  {
  }
};

void CheckPostDefinedDeviceAdapter()
{
  // Nothing to really check. If this compiles, then the test is probably
  // successful.
  vtkm::cont::ArrayHandle<vtkm::Id> test;
}

} // anonymous namespace

int UnitTestDeviceAdapterAlgorithmDependency(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(CheckPostDefinedDeviceAdapter, argc, argv);
}
