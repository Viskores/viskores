//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Assert.h>
#include <vtkm/StaticAssert.h>
#include <vtkm/TypeTraits.h>

#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/Error.h>
#include <vtkm/cont/ErrorBadValue.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

#include <chrono>
#include <thread>
#include <type_traits>

namespace ErrorHandlingNamespace
{

////
//// BEGIN-EXAMPLE CatchingErrors
////
int main(int argc, char** argv)
{
  //// PAUSE-EXAMPLE
  // Suppress unused argument warnings
  (void)argv;
  //// RESUME-EXAMPLE
  try
  {
    // Do something cool with VTK-m
    // ...
    //// PAUSE-EXAMPLE
    if (argc == 0)
      throw vtkm::cont::ErrorBadValue("Oh, no!");
    //// RESUME-EXAMPLE
  }
  catch (const vtkm::cont::Error& error)
  {
    std::cout << error.GetMessage() << std::endl;
    return 1;
  }
  return 0;
}
////
//// END-EXAMPLE CatchingErrors
////

////
//// BEGIN-EXAMPLE Assert
////
template<typename T>
VTKM_CONT T GetArrayValue(vtkm::cont::ArrayHandle<T> arrayHandle, vtkm::Id index)
{
  VTKM_ASSERT(index >= 0);
  VTKM_ASSERT(index < arrayHandle.GetNumberOfValues());
  ////
  //// END-EXAMPLE Assert
  ////
  return arrayHandle.ReadPortal().Get(index);
}

VTKM_CONT
void TryGetArrayValue()
{
  GetArrayValue(vtkm::cont::make_ArrayHandle({ 2.0, 5.0 }), 0);
  GetArrayValue(vtkm::cont::make_ArrayHandle({ 2.0, 5.0 }), 1);
}

////
//// BEGIN-EXAMPLE StaticAssert
////
template<typename T>
VTKM_EXEC_CONT void MyMathFunction(T& value)
{
  VTKM_STATIC_ASSERT((std::is_same<typename vtkm::TypeTraits<T>::DimensionalityTag,
                                   vtkm::TypeTraitsScalarTag>::value));

  VTKM_STATIC_ASSERT_MSG(sizeof(T) >= 4,
                         "MyMathFunction needs types with at least 32 bits.");
  ////
  //// END-EXAMPLE StaticAssert
  ////
  for (vtkm::IdComponent iteration = 0; iteration < 5; iteration++)
  {
    value = value * value;
  }
}

VTKM_EXEC_CONT
void TryMyMathFunction()
{
  vtkm::Id value(4);
  MyMathFunction(value);
}

////
//// BEGIN-EXAMPLE ExecutionErrors
////
struct SquareRoot : vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  template<typename T>
  VTKM_EXEC T operator()(T x) const
  {
    if (x < 0)
    {
      this->RaiseError("Cannot take the square root of a negative number.");
      return vtkm::Nan<T>();
    }
    return vtkm::Sqrt(x);
  }
};
////
//// END-EXAMPLE ExecutionErrors
////

VTKM_CONT
void TrySquareRoot()
{
  vtkm::cont::ArrayHandle<vtkm::Float32> output;

  vtkm::worklet::DispatcherMapField<SquareRoot> dispatcher;

  std::cout << "Trying valid input." << std::endl;
  vtkm::cont::ArrayHandleCounting<vtkm::Float32> validInput(0.0f, 1.0f, 10);
  dispatcher.Invoke(validInput, output);

  std::cout << "Trying invalid input." << std::endl;
  vtkm::cont::ArrayHandleCounting<vtkm::Float32> invalidInput(-2.0, 1.0f, 10);
  bool errorCaught = false;
  try
  {
    dispatcher.Invoke(invalidInput, output);
    // Some device adapters are launched asynchronously, and you won't get the error
    // until a follow-up call.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    dispatcher.Invoke(invalidInput, output);
  }
  catch (const vtkm::cont::ErrorExecution& error)
  {
    std::cout << "Caught this error:" << std::endl;
    std::cout << error.GetMessage() << std::endl;
    errorCaught = true;
  }
  VTKM_TEST_ASSERT(errorCaught, "Did not get expected error.");
}

void Test()
{
  VTKM_TEST_ASSERT(ErrorHandlingNamespace::main(0, NULL) != 0, "No error?");
  TryGetArrayValue();
  TryMyMathFunction();
  TrySquareRoot();
}

} // namespace ErrorHandlingNamespace

int GuideExampleErrorHandling(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(ErrorHandlingNamespace::Test, argc, argv);
}
