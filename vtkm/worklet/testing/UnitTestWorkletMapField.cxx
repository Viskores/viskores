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
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>

#include <vtkm/cont/VariantArrayHandle.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

class TestMapFieldWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, FieldInOut);
  using ExecutionSignature = _3(_1, _2, _3, WorkIndex);

  template <typename T>
  VTKM_EXEC T operator()(const T& in, T& out, T& inout, vtkm::Id workIndex) const
  {
    if (!test_equal(in, TestValue(workIndex, T()) + T(100)))
    {
      this->RaiseError("Got wrong input value.");
    }
    out = in - T(100);
    if (!test_equal(inout, TestValue(workIndex, T()) + T(100)))
    {
      this->RaiseError("Got wrong in-out value.");
    }

    // We return the new value of inout. Since _3 is both an arg and return,
    // this tests that the return value is set after updating the arg values.
    return inout - T(100);
  }

  template <typename T1, typename T2, typename T3>
  VTKM_EXEC T3 operator()(const T1&, const T2&, const T3&, vtkm::Id) const
  {
    this->RaiseError("Cannot call this worklet with different types.");
    return vtkm::TypeTraits<T3>::ZeroInitialization();
  }
};

class TestMapFieldWorkletLimitedTypes : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, FieldInOut);
  using ExecutionSignature = _2(_1, _3, WorkIndex);

  template <typename T1, typename T3>
  VTKM_EXEC T1 operator()(const T1& in, T3& inout, vtkm::Id workIndex) const
  {
    if (!test_equal(in, TestValue(workIndex, T1()) + T1(100)))
    {
      this->RaiseError("Got wrong input value.");
    }

    if (!test_equal(inout, TestValue(workIndex, T3()) + T3(100)))
    {
      this->RaiseError("Got wrong in-out value.");
    }
    inout = inout - T3(100);

    return in - T1(100);
  }
};

namespace mapfield
{
static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename WorkletType>
struct DoStaticTestWorklet
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inputArray[ARRAY_SIZE];

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inputArray[index] = TestValue(index, T()) + T(100);
    }

    vtkm::cont::ArrayHandle<T> inputHandle = vtkm::cont::make_ArrayHandle(inputArray, ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> outputHandle, outputHandleAsPtr;
    vtkm::cont::ArrayHandle<T> inoutHandle, inoutHandleAsPtr;

    vtkm::cont::ArrayCopy(inputHandle, inoutHandle);
    vtkm::cont::ArrayCopy(inputHandle, inoutHandleAsPtr);

    std::cout << "Create and run dispatchers." << std::endl;
    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;
    dispatcher.Invoke(inputHandle, outputHandle, inoutHandle);
    dispatcher.Invoke(&inputHandle, &outputHandleAsPtr, &inoutHandleAsPtr);

    std::cout << "Check results." << std::endl;
    CheckPortal(outputHandle.GetPortalConstControl());
    CheckPortal(inoutHandle.GetPortalConstControl());
    CheckPortal(outputHandleAsPtr.GetPortalConstControl());
    CheckPortal(inoutHandleAsPtr.GetPortalConstControl());

    std::cout << "Try to invoke with an input array of the wrong size." << std::endl;
    inputHandle.Shrink(ARRAY_SIZE / 2);
    bool exceptionThrown = false;
    try
    {
      dispatcher.Invoke(inputHandle, outputHandle, inoutHandle);
    }
    catch (vtkm::cont::ErrorBadValue& error)
    {
      std::cout << "  Caught expected error: " << error.GetMessage() << std::endl;
      exceptionThrown = true;
    }
    VTKM_TEST_ASSERT(exceptionThrown, "Dispatcher did not throw expected exception.");
  }
};

template <typename WorkletType>
struct DoVariantTestWorklet
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inputArray[ARRAY_SIZE];

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inputArray[index] = TestValue(index, T()) + T(100);
    }

    vtkm::cont::ArrayHandle<T> inputHandle = vtkm::cont::make_ArrayHandle(inputArray, ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> outputHandle;
    vtkm::cont::ArrayHandle<T> inoutHandle;


    std::cout << "Create and run dispatcher with variant arrays." << std::endl;
    vtkm::worklet::DispatcherMapField<WorkletType> dispatcher;

    vtkm::cont::VariantArrayHandle inputVariant(inputHandle);

    { //Verify we can pass by value
      vtkm::cont::ArrayCopy(inputHandle, inoutHandle);
      vtkm::cont::VariantArrayHandle outputVariant(outputHandle);
      vtkm::cont::VariantArrayHandle inoutVariant(inoutHandle);
      dispatcher.Invoke(inputVariant, outputVariant, inoutVariant);
      CheckPortal(outputHandle.GetPortalConstControl());
      CheckPortal(inoutHandle.GetPortalConstControl());
    }

    { //Verify we can pass by pointer
      vtkm::cont::ArrayCopy(inputHandle, inoutHandle);
      vtkm::cont::VariantArrayHandle outputVariant(outputHandle);
      vtkm::cont::VariantArrayHandle inoutVariant(inoutHandle);
      dispatcher.Invoke(&inputVariant, &outputVariant, &inoutVariant);
      CheckPortal(outputHandle.GetPortalConstControl());
      CheckPortal(inoutHandle.GetPortalConstControl());
    }
  }
};

template <typename WorkletType>
struct DoTestWorklet
{
  template <typename T>
  VTKM_CONT void operator()(T t) const
  {
    DoStaticTestWorklet<WorkletType> sw;
    sw(t);
    DoVariantTestWorklet<WorkletType> dw;
    dw(t);
  }
};

void TestWorkletMapField(vtkm::cont::DeviceAdapterId id)
{
  std::cout << "Testing Map Field on device adapter: " << id.GetName() << std::endl;

  std::cout << "--- Worklet accepting all types." << std::endl;
  vtkm::testing::Testing::TryTypes(mapfield::DoTestWorklet<TestMapFieldWorklet>(),
                                   vtkm::TypeListTagCommon());

  std::cout << "--- Worklet accepting some types." << std::endl;
  vtkm::testing::Testing::TryTypes(mapfield::DoTestWorklet<TestMapFieldWorkletLimitedTypes>(),
                                   vtkm::TypeListTagFieldScalar());

  std::cout << "--- Sending bad type to worklet." << std::endl;
  try
  {
    //can only test with variant arrays, as static arrays will fail to compile
    DoVariantTestWorklet<TestMapFieldWorkletLimitedTypes> badWorkletTest;
    badWorkletTest(vtkm::Vec<vtkm::Float32, 3>());
    VTKM_TEST_FAIL("Did not throw expected error.");
  }
  catch (vtkm::cont::ErrorBadType& error)
  {
    std::cout << "Got expected error: " << error.GetMessage() << std::endl;
  }
}

} // mapfield namespace

int UnitTestWorkletMapField(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::RunOnDevice(mapfield::TestWorkletMapField, argc, argv);
}
