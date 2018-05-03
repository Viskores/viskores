//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/internal/DeviceAdapterError.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

struct TryExecuteTestFunctor
{
  vtkm::IdComponent NumCalls;

  VTKM_CONT
  TryExecuteTestFunctor()
    : NumCalls(0)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device,
                            const vtkm::cont::ArrayHandle<vtkm::FloatDefault>& in,
                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>& out)
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;
    Algorithm::Copy(in, out);
    this->NumCalls++;
    return true;
  }
};

template <typename DeviceList>
void TryExecuteWithDevice(DeviceList, bool expectSuccess)
{
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> inArray;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArray;

  inArray.Allocate(ARRAY_SIZE);
  SetPortal(inArray.GetPortalControl());

  TryExecuteTestFunctor functor;

  bool result = vtkm::cont::TryExecute(functor, DeviceList(), inArray, outArray);

  if (expectSuccess)
  {
    VTKM_TEST_ASSERT(result, "Call returned failure when expected success.");
    VTKM_TEST_ASSERT(functor.NumCalls == 1, "Bad number of calls");
    CheckPortal(outArray.GetPortalConstControl());
  }
  else
  {
    VTKM_TEST_ASSERT(!result, "Call returned true when expected failure.");
  }

  //verify the ability to pass rvalue functors
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArray2;
  result = vtkm::cont::TryExecute(TryExecuteTestFunctor(), DeviceList(), inArray, outArray2);
  if (expectSuccess)
  {
    VTKM_TEST_ASSERT(result, "Call returned failure when expected success.");
    CheckPortal(outArray2.GetPortalConstControl());
  }
  else
  {
    VTKM_TEST_ASSERT(!result, "Call returned true when expected failure.");
  }
}

template <typename DeviceList>
void TryExecuteAllExplicit(DeviceList, bool expectSuccess)
{
  vtkm::cont::RuntimeDeviceTracker tracker;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> inArray;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArray;

  inArray.Allocate(ARRAY_SIZE);
  SetPortal(inArray.GetPortalControl());

  bool result =
    vtkm::cont::TryExecute(TryExecuteTestFunctor(), tracker, DeviceList(), inArray, outArray);
  if (expectSuccess)
  {
    VTKM_TEST_ASSERT(result, "Call returned failure when expected success.");
    CheckPortal(outArray.GetPortalConstControl());
  }
  else
  {
    VTKM_TEST_ASSERT(!result, "Call returned true when expected failure.");
  }
}

struct EdgeCaseFunctor
{
  template <typename DeviceList>
  bool operator()(DeviceList, int, float, bool) const
  {
    return true;
  }
  template <typename DeviceList>
  bool operator()(DeviceList) const
  {
    return true;
  }
};

void TryExecuteAllEdgeCases()
{
  using ValidDevice = vtkm::cont::DeviceAdapterTagSerial;
  using SingleValidList = vtkm::ListTagBase<ValidDevice>;
  auto tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker();

  std::cout << "TryExecute no Runtime, no Device, no parameters." << std::endl;
  vtkm::cont::TryExecute(EdgeCaseFunctor());

  std::cout << "TryExecute no Runtime, no Device, with parameters." << std::endl;
  vtkm::cont::TryExecute(EdgeCaseFunctor(), int{ 42 }, float{ 3.14f }, bool{ true });

  std::cout << "TryExecute with Runtime, no Device, no parameters." << std::endl;
  vtkm::cont::TryExecute(EdgeCaseFunctor(), tracker);

  std::cout << "TryExecute with Runtime, no Device, with parameters." << std::endl;
  vtkm::cont::TryExecute(EdgeCaseFunctor(), tracker, int{ 42 }, float{ 3.14f }, bool{ true });

  std::cout << "TryExecute no Runtime, with Device, no parameters." << std::endl;
  vtkm::cont::TryExecute(EdgeCaseFunctor(), SingleValidList());

  std::cout << "TryExecute no Runtime, with Device, with parameters." << std::endl;
  vtkm::cont::TryExecute(
    EdgeCaseFunctor(), SingleValidList(), int{ 42 }, float{ 3.14f }, bool{ true });

  std::cout << "TryExecute with Runtime, with Device, no parameters." << std::endl;
  vtkm::cont::TryExecute(EdgeCaseFunctor(), tracker, SingleValidList());

  std::cout << "TryExecute with Runtime, with Device, with parameters." << std::endl;
  vtkm::cont::TryExecute(
    EdgeCaseFunctor(), tracker, SingleValidList(), int{ 42 }, float{ 3.14f }, bool{ true });
}

template <typename DeviceList>
void TryExecuteTests(DeviceList list, bool expectSuccess)
{
  TryExecuteAllExplicit(list, expectSuccess);
  TryExecuteWithDevice(list, expectSuccess);
}

static void Run()
{
  using ValidDevice = vtkm::cont::DeviceAdapterTagSerial;
  using InvalidDevice = vtkm::cont::DeviceAdapterTagError;


  TryExecuteAllEdgeCases();

  std::cout << "Try a list with a single entry." << std::endl;
  using SingleValidList = vtkm::ListTagBase<ValidDevice>;
  TryExecuteTests(SingleValidList(), true);

  std::cout << "Try a list with two valid devices." << std::endl;
  using DoubleValidList = vtkm::ListTagBase<ValidDevice, ValidDevice>;
  TryExecuteTests(DoubleValidList(), true);

  std::cout << "Try a list with only invalid device." << std::endl;
  using SingleInvalidList = vtkm::ListTagBase<InvalidDevice>;
  TryExecuteTests(SingleInvalidList(), false);

  std::cout << "Try a list with an invalid and valid device." << std::endl;
  using InvalidAndValidList = vtkm::ListTagBase<InvalidDevice, ValidDevice>;
  TryExecuteTests(InvalidAndValidList(), true);
}

} // anonymous namespace

int UnitTestTryExecute(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(Run);
}
