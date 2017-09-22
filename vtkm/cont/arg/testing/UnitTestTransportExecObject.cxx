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

#include <vtkm/cont/arg/TransportTagExecObject.h>

#include <vtkm/exec/ExecutionObjectBase.h>
#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/Testing.h>

#define EXPECTED_NUMBER 42

namespace
{

struct TestExecutionObject : public vtkm::exec::ExecutionObjectBase
{
  vtkm::Int32 Number;
};

struct TestKernel : public vtkm::exec::FunctorBase
{
  TestExecutionObject Object;

  VTKM_EXEC
  void operator()(vtkm::Id) const
  {
    if (this->Object.Number != EXPECTED_NUMBER)
    {
      this->RaiseError("Got bad execution object.");
    }
  }
};

template <typename Device>
void TryExecObjectTransport(Device)
{
  TestExecutionObject contObject;
  contObject.Number = EXPECTED_NUMBER;

  vtkm::cont::arg::Transport<vtkm::cont::arg::TransportTagExecObject, TestExecutionObject, Device>
    transport;

  TestKernel kernel;
  kernel.Object = transport(contObject, nullptr, 1, 1);

  vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, 1);
}

void TestExecObjectTransport()
{
  std::cout << "Trying ExecObject transport with serial device." << std::endl;
  TryExecObjectTransport(vtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportExecObject(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestExecObjectTransport);
}
