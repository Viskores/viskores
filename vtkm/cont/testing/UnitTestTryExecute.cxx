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
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/internal/DeviceAdapterError.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static const vtkm::Id ARRAY_SIZE = 10;

struct TryExecuteTestFunctor
{
  vtkm::IdComponent NumCalls;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> InArray;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> OutArray;

  VTKM_CONT
  TryExecuteTestFunctor(vtkm::cont::ArrayHandle<vtkm::FloatDefault> inArray,
                        vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArray)
    : NumCalls(0)
    , InArray(inArray)
    , OutArray(outArray)
  {
  }

  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<Device>;
    Algorithm::Copy(this->InArray, this->OutArray);
    this->NumCalls++;
    return true;
  }
};

template <typename DeviceList>
void TryExecuteWithList(DeviceList, bool expectSuccess)
{
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> inArray;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArray;

  inArray.Allocate(ARRAY_SIZE);
  SetPortal(inArray.GetPortalControl());

  TryExecuteTestFunctor functor(inArray, outArray);

  bool result = vtkm::cont::TryExecute(functor, DeviceList());

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
}

static void Run()
{
  using ValidDevice = vtkm::cont::DeviceAdapterTagSerial;
  using InvalidDevice = vtkm::cont::DeviceAdapterTagError;

  std::cout << "Try a list with a single entry." << std::endl;
  using SingleValidList = vtkm::ListTagBase<ValidDevice>;
  TryExecuteWithList(SingleValidList(), true);

  std::cout << "Try a list with two valid devices." << std::endl;
  using DoubleValidList = vtkm::ListTagBase<ValidDevice, ValidDevice>;
  TryExecuteWithList(DoubleValidList(), true);

  std::cout << "Try a list with only invalid device." << std::endl;
  using SingleInvalidList = vtkm::ListTagBase<InvalidDevice>;
  TryExecuteWithList(SingleInvalidList(), false);

  std::cout << "Try a list with an invalid and valid device." << std::endl;
  using InvalidAndValidList = vtkm::ListTagBase<InvalidDevice, ValidDevice>;
  TryExecuteWithList(InvalidAndValidList(), true);
}

} // anonymous namespace

int UnitTestTryExecute(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(Run);
}
