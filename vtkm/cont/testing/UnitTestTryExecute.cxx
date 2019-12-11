//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Error.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorBadDevice.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/Testing.h>

#include <exception>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

class ErrorDeviceIndependent : public vtkm::cont::Error
{
public:
  ErrorDeviceIndependent(const std::string& msg)
    : vtkm::cont::Error(msg, true)
  {
  }
};

class ErrorDeviceDependent : public vtkm::cont::Error
{
public:
  ErrorDeviceDependent(const std::string& msg)
    : vtkm::cont::Error(msg, false)
  {
  }
};

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

template <typename ExceptionT>
struct TryExecuteTestErrorFunctor
{
  template <typename Device>
  VTKM_CONT bool operator()(Device)
  {
    throw ExceptionT("Test message");
  }
};

template <typename DeviceList>
void TryExecuteTests(DeviceList, bool expectSuccess)
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
  using SingleValidList = vtkm::List<ValidDevice>;

  std::cout << "TryExecute no Runtime, no Device, no parameters." << std::endl;
  vtkm::cont::TryExecute(EdgeCaseFunctor());

  std::cout << "TryExecute no Runtime, no Device, with parameters." << std::endl;
  vtkm::cont::TryExecute(EdgeCaseFunctor(), int{ 42 }, float{ 3.14f }, bool{ true });

  std::cout << "TryExecute with Device, no parameters." << std::endl;
  vtkm::cont::TryExecute(EdgeCaseFunctor(), SingleValidList());

  std::cout << "TryExecute with Device, with parameters." << std::endl;
  vtkm::cont::TryExecute(
    EdgeCaseFunctor(), SingleValidList(), int{ 42 }, float{ 3.14f }, bool{ true });
}

template <typename ExceptionType>
void RunErrorTest(bool shouldFail, bool shouldThrow, bool shouldDisable)
{
  using Device = vtkm::cont::DeviceAdapterTagSerial;
  using Functor = TryExecuteTestErrorFunctor<ExceptionType>;

  // Initialize this one to what we expect -- it won't get set if we throw.
  bool succeeded = !shouldFail;
  bool threw = false;
  bool disabled = false;

  vtkm::cont::ScopedRuntimeDeviceTracker scopedTracker(Device{});

  try
  {
    succeeded = vtkm::cont::TryExecute(Functor{});
    threw = false;
  }
  catch (...)
  {
    threw = true;
  }

  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  disabled = !tracker.CanRunOn(Device{});

  std::cout << "Failed: " << !succeeded << " "
            << "Threw: " << threw << " "
            << "Disabled: " << disabled << "\n"
            << std::endl;

  VTKM_TEST_ASSERT(shouldFail == !succeeded, "TryExecute return status incorrect.");
  VTKM_TEST_ASSERT(threw == shouldThrow, "TryExecute throw behavior incorrect.");
  VTKM_TEST_ASSERT(disabled == shouldDisable, "TryExecute device-disabling behavior incorrect.");
}

void TryExecuteErrorTests()
{
  std::cout << "Test ErrorBadAllocation." << std::endl;
  RunErrorTest<vtkm::cont::ErrorBadAllocation>(true, false, true);

  std::cout << "Test ErrorBadDevice." << std::endl;
  RunErrorTest<vtkm::cont::ErrorBadDevice>(true, false, true);

  std::cout << "Test ErrorBadType." << std::endl;
  RunErrorTest<vtkm::cont::ErrorBadType>(true, false, false);

  std::cout << "Test ErrorBadValue." << std::endl;
  RunErrorTest<vtkm::cont::ErrorBadValue>(true, true, false);

  std::cout << "Test custom vtkm Error (dev indep)." << std::endl;
  RunErrorTest<ErrorDeviceIndependent>(true, true, false);

  std::cout << "Test custom vtkm Error (dev dep)." << std::endl;
  RunErrorTest<ErrorDeviceDependent>(true, false, false);

  std::cout << "Test std::exception." << std::endl;
  RunErrorTest<std::runtime_error>(true, false, false);

  std::cout << "Test throw non-exception." << std::endl;
  RunErrorTest<std::string>(true, false, false);
}

static void Run()
{
  using ValidDevice = vtkm::cont::DeviceAdapterTagSerial;
  using InvalidDevice = vtkm::cont::DeviceAdapterTagUndefined;

  TryExecuteAllEdgeCases();

  std::cout << "Try a list with a single entry." << std::endl;
  using SingleValidList = vtkm::List<ValidDevice>;
  TryExecuteTests(SingleValidList(), true);

  std::cout << "Try a list with two valid devices." << std::endl;
  using DoubleValidList = vtkm::List<ValidDevice, ValidDevice>;
  TryExecuteTests(DoubleValidList(), true);

  std::cout << "Try a list with only invalid device." << std::endl;
  using SingleInvalidList = vtkm::List<InvalidDevice>;
  TryExecuteTests(SingleInvalidList(), false);

  std::cout << "Try a list with an invalid and valid device." << std::endl;
  using InvalidAndValidList = vtkm::List<InvalidDevice, ValidDevice>;
  TryExecuteTests(InvalidAndValidList(), true);

  TryExecuteErrorTests();
}

} // anonymous namespace

int UnitTestTryExecute(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
