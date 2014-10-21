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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>

#include <vtkm/cont/testing/Testing.h>

namespace {

static const vtkm::Id ARRAY_SIZE = 10;

class TestWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn, FieldOut);
  typedef _2 ExecutionSignature(_1, WorkIndex);

  template<typename T>
  T operator()(T x, vtkm::Id workIndex) const
  {
    if (x != TestValue(workIndex, T()) + T(100))
    {
      this->RaiseError("Got wrong input value.");
    }
    return x - T(100);
  }
};

struct DoTestWorklet
{
  template<typename T>
  VTKM_CONT_EXPORT
  void operator()(T) const
  {
    std::cout << "Set up data." << std::endl;
    T inputArray[ARRAY_SIZE];

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      inputArray[index] = TestValue(index, T()) + T(100);
    }

    vtkm::cont::ArrayHandle<T> inputHandle =
        vtkm::cont::make_ArrayHandle(inputArray, ARRAY_SIZE);
    vtkm::cont::ArrayHandle<T> outputHandle;

    std::cout << "Create and run dispatcher." << std::endl;
    vtkm::worklet::DispatcherMapField<TestWorklet> dispatcher;
    dispatcher.Invoke(inputHandle, outputHandle);

    std::cout << "Check result." << std::endl;
    CheckPortal(outputHandle.GetPortalConstControl());

    // The following test is commented out because as of this writing
    // (10-21-2014) there is an issue with getting unexpected types when
    // casting dynamic arrays. In particular, this issue is with using dynamic
    // arrays. We know that both arrays will always be the same type, but the
    // arrays are cast independently. Thus, the compiler will generate code for
    // odd combinations that are incompatibile with each other. Thus, we need a
    // way to better specify the types expected by the worklet function. I can
    // think of two general ways (there might be more).
    //
    // 1. Specify the expected type in the ControlSignature. This would
    // probably be a template argument of the tag with a list of basic types
    // that could be in the array. The dynamic array casting would then take
    // that into account and only try those specified types. That should be
    // fairly straightforward to implement and handle many cases. However, it
    // still has the problem that all dynamic arrays are cast independently.
    // Thus, for example, if you have a worklet that can operate on vectors of
    // any size, you will likely get a compile error when trying to operate on
    // two vectors of different size when you expected them to be the same.
    // This particular general case might actually be quite rare, so in that
    // case the user has the onus to create a default template that handles
    // this exceptional case with failure.
    //
    // 2. Have a mechanism to identify when the type of two things is expected
    // to be the same. I'm not sure what the programming interface for that
    // would look though.
    //
//    std::cout << "Repeat with dynamic arrays." << std::endl;
//    // Clear out output array.
//    outputHandle = vtkm::cont::ArrayHandle<T>();
//    vtkm::cont::DynamicArrayHandle inputDynamic(inputHandle);
//    vtkm::cont::DynamicArrayHandle outputDynamic(outputHandle);
//    dispatcher.Invoke(inputDynamic, outputDynamic);
//    CheckPortal(outputHandle.GetPortalConstControl());
  }
};

void TestWorkletMapField()
{
  vtkm::testing::Testing::TryTypes(DoTestWorklet(), vtkm::TypeListTagCommon());
}

} // anonymous namespace

int UnitTestWorkletMapField(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestWorkletMapField);
}
