//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

// This tests a previous problem where code templated on the device adapter and
// used one of the device adapter algorithms (for example, the dispatcher) had
// to be declared after any device adapter it was ever used with.

#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR

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

int UnitTestDeviceAdapterAlgorithmDependency(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(CheckPostDefinedDeviceAdapter);
}
