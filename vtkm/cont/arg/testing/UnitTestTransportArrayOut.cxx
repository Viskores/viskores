//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/arg/TransportTagArrayOut.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

static const vtkm::Id ARRAY_SIZE = 10;

template<typename PortalType>
struct TestKernel : public vtkm::exec::FunctorBase
{
  PortalType Portal;

  VTKM_EXEC_EXPORT
  void operator()(vtkm::Id index) const
  {
    typedef typename PortalType::ValueType ValueType;
    this->Portal.Set(index, TestValue(index, ValueType()));
  }
};

template<typename Device>
struct TryArrayOutType
{
  template<typename T>
  void operator()(T) const
  {
    typedef vtkm::cont::ArrayHandle<T> ArrayHandleType;
    ArrayHandleType handle;

    typedef typename ArrayHandleType::
        template ExecutionTypes<Device>::Portal PortalType;

    vtkm::cont::arg::Transport<
        vtkm::cont::arg::TransportTagArrayOut, ArrayHandleType, Device>
        transport;

    TestKernel<PortalType> kernel;
    kernel.Portal = transport(handle, ARRAY_SIZE);

    VTKM_TEST_ASSERT(handle.GetNumberOfValues() == ARRAY_SIZE,
                     "ArrayOut transport did not allocate array correctly.");

    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(kernel, ARRAY_SIZE);

    CheckPortal(handle.GetPortalConstControl());
  }
};

template<typename Device>
void TryArrayOutTransport(Device)
{
  vtkm::testing::Testing::TryTypes(TryArrayOutType<Device>());
}

void TestArrayOutTransport()
{
  std::cout << "Trying ArrayOut transport with serial device." << std::endl;
  TryArrayOutTransport(vtkm::cont::DeviceAdapterTagSerial());
}

} // Anonymous namespace

int UnitTestTransportArrayOut(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayOutTransport);
}
