//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/Assert.h>
#include <viskores/StaticAssert.h>
#include <viskores/TypeTraits.h>

#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/Error.h>
#include <viskores/cont/ErrorBadValue.h>

#include <viskores/worklet/DispatcherMapField.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

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
    // Do something cool with Viskores
    // ...
    //// PAUSE-EXAMPLE
    if (argc == 0)
      throw viskores::cont::ErrorBadValue("Oh, no!");
    //// RESUME-EXAMPLE
  }
  catch (const viskores::cont::Error& error)
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
VISKORES_CONT T GetArrayValue(viskores::cont::ArrayHandle<T> arrayHandle,
                              viskores::Id index)
{
  VISKORES_ASSERT(index >= 0);
  VISKORES_ASSERT(index < arrayHandle.GetNumberOfValues());
  ////
  //// END-EXAMPLE Assert
  ////
  return arrayHandle.ReadPortal().Get(index);
}

VISKORES_CONT
void TryGetArrayValue()
{
  GetArrayValue(viskores::cont::make_ArrayHandle({ 2.0, 5.0 }), 0);
  GetArrayValue(viskores::cont::make_ArrayHandle({ 2.0, 5.0 }), 1);
}

////
//// BEGIN-EXAMPLE StaticAssert
////
template<typename T>
VISKORES_EXEC_CONT void MyMathFunction(T& value)
{
  VISKORES_STATIC_ASSERT(
    (std::is_same<typename viskores::TypeTraits<T>::DimensionalityTag,
                  viskores::TypeTraitsScalarTag>::value));

  VISKORES_STATIC_ASSERT_MSG(sizeof(T) >= 4,
                             "MyMathFunction needs types with at least 32 bits.");
  ////
  //// END-EXAMPLE StaticAssert
  ////
  for (viskores::IdComponent iteration = 0; iteration < 5; iteration++)
  {
    value = value * value;
  }
}

VISKORES_EXEC_CONT
void TryMyMathFunction()
{
  viskores::Id value(4);
  MyMathFunction(value);
}

////
//// BEGIN-EXAMPLE ExecutionErrors
////
struct SquareRoot : viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  template<typename T>
  VISKORES_EXEC T operator()(T x) const
  {
    if (x < 0)
    {
      this->RaiseError("Cannot take the square root of a negative number.");
      return viskores::Nan<T>();
    }
    return viskores::Sqrt(x);
  }
};
////
//// END-EXAMPLE ExecutionErrors
////

VISKORES_CONT
void TrySquareRoot()
{
  viskores::cont::ArrayHandle<viskores::Float32> output;

  viskores::worklet::DispatcherMapField<SquareRoot> dispatcher;

  std::cout << "Trying valid input." << std::endl;
  viskores::cont::ArrayHandleCounting<viskores::Float32> validInput(0.0f, 1.0f, 10);
  dispatcher.Invoke(validInput, output);

  std::cout << "Trying invalid input." << std::endl;
  viskores::cont::ArrayHandleCounting<viskores::Float32> invalidInput(-2.0, 1.0f, 10);
  bool errorCaught = false;
  try
  {
    dispatcher.Invoke(invalidInput, output);
    // Some device adapters are launched asynchronously, and you won't get the error
    // until a follow-up call.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    dispatcher.Invoke(invalidInput, output);
  }
  catch (const viskores::cont::ErrorExecution& error)
  {
    std::cout << "Caught this error:" << std::endl;
    std::cout << error.GetMessage() << std::endl;
    errorCaught = true;
  }
  VISKORES_TEST_ASSERT(errorCaught, "Did not get expected error.");
}

void Test()
{
  VISKORES_TEST_ASSERT(ErrorHandlingNamespace::main(0, NULL) != 0, "No error?");
  TryGetArrayValue();
  TryMyMathFunction();
  TrySquareRoot();
}

} // namespace ErrorHandlingNamespace

int GuideExampleErrorHandling(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(ErrorHandlingNamespace::Test, argc, argv);
}
