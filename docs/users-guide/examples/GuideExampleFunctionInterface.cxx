//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/internal/FunctionInterface.h>

#include <viskores/StaticAssert.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DeviceAdapter.h>
#include <viskores/cont/ErrorBadType.h>

#include <viskores/cont/testing/Testing.h>

#include <string.h>

namespace
{

void BasicFunctionInterface()
{
  ////
  //// BEGIN-EXAMPLE DefineFunctionInterface
  ////
  // FunctionInterfaces matching some common POSIX functions.
  viskores::internal::FunctionInterface<size_t(const char*)> strlenInterface;

  viskores::internal::FunctionInterface<char*(char*, const char* s2, size_t)>
    strncpyInterface;
  ////
  //// END-EXAMPLE DefineFunctionInterface
  ////

  ////
  //// BEGIN-EXAMPLE UseMakeFunctionInterface
  ////
  const char* s = "Hello World";
  static const size_t BUFFER_SIZE = 100;
  char* buffer = (char*)malloc(BUFFER_SIZE);

  strlenInterface = viskores::internal::make_FunctionInterface<size_t>(s);

  strncpyInterface =
    viskores::internal::make_FunctionInterface<char*>(buffer, s, BUFFER_SIZE);
  ////
  //// END-EXAMPLE UseMakeFunctionInterface
  ////

  std::cout << "Trying interfaces." << std::endl;

  ////
  //// BEGIN-EXAMPLE FunctionInterfaceArity
  ////
  VISKORES_STATIC_ASSERT(
    viskores::internal::FunctionInterface<size_t(const char*)>::ARITY == 1);

  viskores::IdComponent arity = strncpyInterface.GetArity(); // arity = 3
  ////
  //// END-EXAMPLE FunctionInterfaceArity
  ////

  VISKORES_TEST_ASSERT(arity == 3, "Unexpected arity.");

  free(buffer);
}

////
//// BEGIN-EXAMPLE FunctionInterfaceGetParameter
////
template<typename FunctionSignature>
void GetFirstParameter(
  const viskores::internal::FunctionInterface<FunctionSignature>& interface)
{
  // The following two uses of GetParameter are equivalent
  std::cout << viskores::internal::ParameterGet<1>(interface) << std::endl;
}
////
//// END-EXAMPLE FunctionInterfaceGetParameter
////

void TryGetParameter()
{
  std::cout << "Getting parameters." << std::endl;

  GetFirstParameter(
    viskores::internal::make_FunctionInterface<void>(std::string("foo")));
}

// SetParameter no longer supported
#if 0
////
//// BEGIN-EXAMPLE FunctionInterfaceSetParameter
////
void SetFirstParameterResolved(
  viskores::internal::FunctionInterface<void(std::string)>& interface,
  const std::string& newFirstParameter)
{
  // The following two uses of SetParameter are equivalent
  interface.SetParameter<1>(newFirstParameter);
  interface.SetParameter(newFirstParameter, viskores::internal::IndexTag<1>());
}

template<typename FunctionSignature, typename T>
void SetFirstParameterTemplated(
  viskores::internal::FunctionInterface<FunctionSignature>& interface,
  T newFirstParameter)
{
  // The following two uses of SetParameter are equivalent
  interface.template SetParameter<1>(newFirstParameter);
  interface.SetParameter(newFirstParameter, viskores::internal::IndexTag<1>());
}
////
//// END-EXAMPLE FunctionInterfaceSetParameter
////

void TrySetParameter()
{
  std::cout << "Setting parameters." << std::endl;

  viskores::internal::FunctionInterface<void(std::string)> functionInterface;

  SetFirstParameterResolved(functionInterface, std::string("foo"));
  VISKORES_TEST_ASSERT(functionInterface.GetParameter<1>() == "foo",
                   "Did not set string.");

  SetFirstParameterTemplated(functionInterface, std::string("bar"));
  VISKORES_TEST_ASSERT(functionInterface.GetParameter<1>() == "bar",
                   "Did not set string.");
}
#endif

// Invoke no longer supported
#if 0
void BasicInvoke()
{
  ////
  //// BEGIN-EXAMPLE FunctionInterfaceBasicInvoke
  ////
  viskores::internal::FunctionInterface<size_t(const char*)> strlenInterface;
  strlenInterface.SetParameter<1>("Hello world");

  strlenInterface.InvokeCont(strlen);

  size_t length = strlenInterface.GetReturnValue(); // length = 11
  ////
  //// END-EXAMPLE FunctionInterfaceBasicInvoke
  ////

  VISKORES_TEST_ASSERT(length == 11, "Bad length.");
}

namespace TransformedInvokeNamespace
{

////
//// BEGIN-EXAMPLE FunctionInterfaceTransformInvoke
////
// Our transform converts C strings to integers, leaves everything else alone.
struct TransformFunctor
{
  template<typename T>
  VISKORES_CONT const T& operator()(const T& x) const
  {
    return x;
  }

  VISKORES_CONT
  viskores::Int32 operator()(const char* x) const { return atoi(x); }
};

// The function we are invoking simply compares two numbers.
struct IsSameFunctor
{
  template<typename T1, typename T2>
  VISKORES_CONT bool operator()(const T1& x, const T2& y) const
  {
    return x == y;
  }
};

void TryTransformedInvoke()
{
  viskores::internal::FunctionInterface<bool(const char*, viskores::Int32)>
    functionInterface = viskores::internal::make_FunctionInterface<bool>(
      (const char*)"42", (viskores::Int32)42);

  functionInterface.InvokeCont(IsSameFunctor(), TransformFunctor());

  bool isSame = functionInterface.GetReturnValue(); // isSame = true
  ////
  //// PAUSE-EXAMPLE
  ////
  VISKORES_TEST_ASSERT(isSame, "Did not get right return value.");
  ////
  //// RESUME-EXAMPLE
  ////
}
////
//// END-EXAMPLE FunctionInterfaceTransformInvoke
////

} // namespace TransformedInvokeNamespace

using namespace TransformedInvokeNamespace;

namespace ReturnContainerNamespace
{

////
//// BEGIN-EXAMPLE FunctionInterfaceReturnContainer
////
template<typename ResultType, bool Valid>
struct PrintReturnFunctor;

template<typename ResultType>
struct PrintReturnFunctor<ResultType, true>
{
  VISKORES_CONT
  void operator()(
    const viskores::internal::FunctionInterfaceReturnContainer<ResultType>& x) const
  {
    std::cout << x.Value << std::endl;
  }
};

template<typename ResultType>
struct PrintReturnFunctor<ResultType, false>
{
  VISKORES_CONT
  void operator()(
    const viskores::internal::FunctionInterfaceReturnContainer<ResultType>&) const
  {
    std::cout << "No return type." << std::endl;
  }
};

template<typename FunctionInterfaceType>
void PrintReturn(const FunctionInterfaceType& functionInterface)
{
  using ResultType = typename FunctionInterfaceType::ResultType;
  using ReturnContainerType =
    viskores::internal::FunctionInterfaceReturnContainer<ResultType>;

  PrintReturnFunctor<ResultType, ReturnContainerType::VALID> printReturn;
  printReturn(functionInterface.GetReturnValueSafe());
}
////
//// END-EXAMPLE FunctionInterfaceReturnContainer
////

} // namespace ReturnContainerNamespace

void TryPrintReturn()
{
  viskores::internal::FunctionInterface<size_t(const char*)> strlenInterface;
  strlenInterface.SetParameter<1>("Hello world");
  strlenInterface.InvokeCont(strlen);
  ReturnContainerNamespace::PrintReturn(strlenInterface);

  ReturnContainerNamespace::PrintReturn(
    viskores::internal::make_FunctionInterface<void>((const char*)"Hello world"));
}
#endif

// Append and Replace no longer supported
#if 0
void Append()
{
  ////
  //// BEGIN-EXAMPLE FunctionInterfaceAppend
  ////
  using viskores::internal::FunctionInterface;
  using viskores::internal::make_FunctionInterface;

  using InitialFunctionInterfaceType =
    FunctionInterface<void(std::string, viskores::Id)>;
  InitialFunctionInterfaceType initialFunctionInterface =
    make_FunctionInterface<void>(std::string("Hello World"), viskores::Id(42));

  using AppendedFunctionInterfaceType1 =
    FunctionInterface<void(std::string, viskores::Id, std::string)>;
  AppendedFunctionInterfaceType1 appendedFunctionInterface1 =
    initialFunctionInterface.Append(std::string("foobar"));
  // appendedFunctionInterface1 has parameters ("Hello World", 42, "foobar")

  using AppendedFunctionInterfaceType2 =
    InitialFunctionInterfaceType::AppendType<viskores::Float32>::type;
  AppendedFunctionInterfaceType2 appendedFunctionInterface2 =
    initialFunctionInterface.Append(viskores::Float32(3.141));
  // appendedFunctionInterface2 has parameters ("Hello World", 42, 3.141)
  ////
  //// END-EXAMPLE FunctionInterfaceAppend
  ////

  std::cout << "Checking appended interface 1." << std::endl;
  VISKORES_TEST_ASSERT(appendedFunctionInterface1.GetParameter<1>() ==
                     std::string("Hello World"),
                   "Bad value in interface.");
  VISKORES_TEST_ASSERT(appendedFunctionInterface1.GetParameter<2>() == 42,
                   "Bad value in interface.");
  VISKORES_TEST_ASSERT(appendedFunctionInterface1.GetParameter<3>() ==
                     std::string("foobar"),
                   "Bad value in interface.");

  std::cout << "Checking appended interface 2." << std::endl;
  VISKORES_TEST_ASSERT(appendedFunctionInterface2.GetParameter<1>() ==
                     std::string("Hello World"),
                   "Bad value in interface.");
  VISKORES_TEST_ASSERT(appendedFunctionInterface2.GetParameter<2>() == 42,
                   "Bad value in interface.");
  VISKORES_TEST_ASSERT(appendedFunctionInterface2.GetParameter<3>() ==
                     viskores::Float32(3.141),
                   "Bad value in interface.");
}

void Replace()
{
  ////
  //// BEGIN-EXAMPLE FunctionInterfaceReplace
  ////
  using viskores::internal::FunctionInterface;
  using viskores::internal::make_FunctionInterface;

  using InitialFunctionInterfaceType =
    FunctionInterface<void(std::string, viskores::Id)>;
  InitialFunctionInterfaceType initialFunctionInterface =
    make_FunctionInterface<void>(std::string("Hello World"), viskores::Id(42));

  using ReplacedFunctionInterfaceType1 =
    FunctionInterface<void(viskores::Float32, viskores::Id)>;
  ReplacedFunctionInterfaceType1 replacedFunctionInterface1 =
    initialFunctionInterface.Replace<1>(viskores::Float32(3.141));
  // replacedFunctionInterface1 has parameters (3.141, 42)

  using ReplacedFunctionInterfaceType2 =
    InitialFunctionInterfaceType::ReplaceType<2, std::string>::type;
  ReplacedFunctionInterfaceType2 replacedFunctionInterface2 =
    initialFunctionInterface.Replace<2>(std::string("foobar"));
  // replacedFunctionInterface2 has parameters ("Hello World", "foobar")
  ////
  //// END-EXAMPLE FunctionInterfaceReplace
  ////

  std::cout << "Checking replaced interface 1." << std::endl;
  VISKORES_TEST_ASSERT(replacedFunctionInterface1.GetParameter<1>() ==
                     viskores::Float32(3.141),
                   "Bad value in interface.");
  VISKORES_TEST_ASSERT(replacedFunctionInterface1.GetParameter<2>() == 42,
                   "Bad value in interface.");

  std::cout << "Checking replaced interface 2." << std::endl;
  VISKORES_TEST_ASSERT(replacedFunctionInterface2.GetParameter<1>() == "Hello World",
                   "Bad value in interface.");
  VISKORES_TEST_ASSERT(replacedFunctionInterface2.GetParameter<2>() == "foobar",
                   "Bad value in interface.");
}

void NextFunctionChainCall(
  const viskores::internal::FunctionInterface<void(viskores::Id*, viskores::Id)>& parameters)
{
  viskores::Id expectedValue = TestValue(0, viskores::Id());

  viskores::Id* array = parameters.GetParameter<1>();
  viskores::Id numValues = parameters.GetParameter<2>();

  std::cout << "Checking values." << std::endl;
  for (viskores::Id index = 0; index < numValues; index++)
  {
    VISKORES_TEST_ASSERT(array[index] == expectedValue, "Bad value.");
  }
}

////
//// BEGIN-EXAMPLE FunctionInterfaceAppendAndReplace
////
template<typename FunctionInterfaceType>
void FunctionCallChain(const FunctionInterfaceType& parameters, viskores::Id arraySize)
{
  // In this hypothetical function call chain, this function replaces the
  // first parameter with an array of that type and appends the array size
  // to the end of the parameters.

  using ArrayValueType =
    typename FunctionInterfaceType::template ParameterType<1>::type;

  // Allocate and initialize array.
  ArrayValueType value = parameters.template GetParameter<1>();
  ArrayValueType* array = new ArrayValueType[arraySize];
  for (viskores::Id index = 0; index < arraySize; index++)
  {
    array[index] = value;
  }

  // Call next function with modified parameters.
  NextFunctionChainCall(parameters.template Replace<1>(array).Append(arraySize));

  // Clean up.
  delete[] array;
}
////
//// END-EXAMPLE FunctionInterfaceAppendAndReplace
////

void TryAppendReplace()
{
  std::cout << "Using replace and append in function call chain." << std::endl;
  FunctionCallChain(
    viskores::internal::make_FunctionInterface<void>(TestValue(0, viskores::Id())), 10);
}
#endif

namespace StaticTransformNamespace
{

////
//// BEGIN-EXAMPLE FunctionInterfaceStaticTransform
////
struct ParametersToPointersFunctor
{
  template<typename T, viskores::IdComponent Index>
  struct ReturnType
  {
    using type = const T*;
  };

  template<typename T, viskores::IdComponent Index>
  VISKORES_CONT const T* operator()(const T& x,
                                    viskores::internal::IndexTag<Index>) const
  {
    return &x;
  }
};

template<typename FunctionInterfaceType>
VISKORES_CONT typename FunctionInterfaceType::template StaticTransformType<
  ParametersToPointersFunctor>::type
ParametersToPointers(FunctionInterfaceType& functionInterface)
{
  return functionInterface.StaticTransformCont(ParametersToPointersFunctor());
}
////
//// END-EXAMPLE FunctionInterfaceStaticTransform
////

} // namespace StaticTransformNamespace

using namespace StaticTransformNamespace;

void TryStaticTransform()
{
  viskores::internal::FunctionInterface<void(viskores::Float32, viskores::Int32)>
    originalFunctionInterface = viskores::internal::make_FunctionInterface<void>(
      TestValue(1, viskores::Float32()), TestValue(2, viskores::Int32()));

  viskores::internal::FunctionInterface<void(const viskores::Float32*,
                                             const viskores::Int32*)>
    transformedFunctionInterface = ParametersToPointers(originalFunctionInterface);

  VISKORES_TEST_ASSERT(
    test_equal(*viskores::internal::ParameterGet<1>(transformedFunctionInterface),
               TestValue(1, viskores::Float32())),
    "Bad value in pointer.");
  VISKORES_TEST_ASSERT(
    test_equal(*viskores::internal::ParameterGet<2>(transformedFunctionInterface),
               TestValue(2, viskores::Int32())),
    "Bad value in pointer.");
}

// DynamicTransform no longer supported
#if 0
namespace DynamicTransformNamespace
{

////
//// BEGIN-EXAMPLE FunctionInterfaceDynamicTransform
////
struct UnpackNumbersTransformFunctor
{
  template<typename InputType, typename ContinueFunctor, viskores::IdComponent Index>
  VISKORES_CONT void operator()(const InputType& input,
                            const ContinueFunctor& continueFunction,
                            viskores::internal::IndexTag<Index>) const
  {
    continueFunction(input);
  }

  template<typename ContinueFunctor, viskores::IdComponent Index>
  VISKORES_CONT void operator()(const std::string& input,
                            const ContinueFunctor& continueFunction,
                            viskores::internal::IndexTag<Index>) const
  {
    if ((input[0] >= '0') && (input[0] <= '9'))
    {
      std::stringstream stream(input);
      viskores::FloatDefault value;
      stream >> value;
      continueFunction(value);
    }
    else
    {
      continueFunction(input);
    }
  }
};

////
//// PAUSE-EXAMPLE
////
struct CheckFunctor
{
  VISKORES_CONT
  void operator()(viskores::FloatDefault value1, std::string value2) const
  {
    VISKORES_TEST_ASSERT(test_equal(value1, 42), "Wrong converted value.");
    VISKORES_TEST_ASSERT(value2 == "Hello World", "Wrong passed value");
  }

  template<typename T1, typename T2>
  VISKORES_CONT void operator()(T1, T2) const
  {
    VISKORES_TEST_FAIL("Called wrong form of CheckFunctor");
  }
};
////
//// RESUME-EXAMPLE
////
struct UnpackNumbersFinishFunctor
{
  template<typename FunctionInterfaceType>
  VISKORES_CONT void operator()(FunctionInterfaceType& functionInterface) const
  {
    // Do something
    ////
    //// PAUSE-EXAMPLE
    ////
    functionInterface.InvokeCont(CheckFunctor());
    ////
    //// RESUME-EXAMPLE
    ////
  }
};

template<typename FunctionInterfaceType>
void DoUnpackNumbers(const FunctionInterfaceType& functionInterface)
{
  functionInterface.DynamicTransformCont(UnpackNumbersTransformFunctor(),
                                         UnpackNumbersFinishFunctor());
}
////
//// END-EXAMPLE FunctionInterfaceDynamicTransform
////

} // namespace DynamicTransformNamespace

using namespace DynamicTransformNamespace;

void TryDynamicTransform()
{
  viskores::internal::FunctionInterface<void(std::string, std::string)>
    functionInterface = viskores::internal::make_FunctionInterface<void>(
      std::string("42"), std::string("Hello World"));
  DoUnpackNumbers(functionInterface);
}
#endif

// DynamicTransform function no longer supported
#if 0
namespace DynamicTransformFunctorNamespace
{

////
//// BEGIN-EXAMPLE DynamicTransform
////
template<typename Device>
struct ArrayCopyFunctor
{
  template<typename Signature>
  VISKORES_CONT void operator()(
    viskores::internal::FunctionInterface<Signature> functionInterface) const
  {
    functionInterface.InvokeCont(*this);
  }

  template<typename T, typename SIn, typename SOut>
  VISKORES_CONT void operator()(const viskores::cont::ArrayHandle<T, SIn>& input,
                            viskores::cont::ArrayHandle<T, SOut>& output) const
  {
    viskores::cont::Algorithm::Copy(input, output);
  }

  template<typename TIn, typename SIn, typename TOut, typename SOut>
  VISKORES_CONT void operator()(const viskores::cont::ArrayHandle<TIn, SIn>&,
                            viskores::cont::ArrayHandle<TOut, SOut>&) const
  {
    throw viskores::cont::ErrorBadType("Arrays to copy must be the same type.");
  }
};

template<typename Device>
void CopyVariantArrays(viskores::cont::VariantArrayHandle input,
                       viskores::cont::VariantArrayHandle output,
                       Device)
{
  viskores::internal::FunctionInterface<void(viskores::cont::VariantArrayHandle,
                                         viskores::cont::VariantArrayHandle)>
    functionInterface = viskores::internal::make_FunctionInterface<void>(input, output);

  functionInterface.DynamicTransformCont(viskores::cont::internal::DynamicTransform(),
                                         ArrayCopyFunctor<Device>());
}
////
//// END-EXAMPLE DynamicTransform
////

} // namespace DynamicTransformFunctorNamespace

using namespace DynamicTransformFunctorNamespace;

void TryDynamicTransformFunctor()
{
  static const viskores::Id ARRAY_SIZE = 10;
  viskores::Float32 buffer[ARRAY_SIZE];
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    buffer[index] = TestValue(index, viskores::Float32());
  }

  viskores::cont::ArrayHandle<viskores::Float32> inputArray =
    viskores::cont::make_ArrayHandle(buffer, ARRAY_SIZE);
  viskores::cont::ArrayHandle<viskores::Float32> outputArray;

  CopyVariantArrays(inputArray, outputArray, viskores::cont::DeviceAdapterTagSerial());

  CheckPortal(outputArray.ReadPortal());
}
#endif

// FunctionInterface::ForEach no longer supported
#if 0
namespace ForEachNamespace
{

////
//// BEGIN-EXAMPLE FunctionInterfaceForEach
////
struct PrintArgumentFunctor
{
  template<typename T, viskores::IdComponent Index>
  VISKORES_CONT void operator()(const T& argument, viskores::internal::IndexTag<Index>) const
  {
    std::cout << Index << ":" << argument << " ";
  }
};

template<typename FunctionInterfaceType>
VISKORES_CONT void PrintArguments(const FunctionInterfaceType& functionInterface)
{
  std::cout << "( ";
  functionInterface.ForEachCont(PrintArgumentFunctor());
  std::cout << ")" << std::endl;
}
////
//// END-EXAMPLE FunctionInterfaceForEach
////

} // namespace ForEachNamespace

using namespace ForEachNamespace;

void TryPrintArguments()
{
  PrintArguments(viskores::internal::make_FunctionInterface<void>(
    std::string("Hello"), 42, std::string("World"), 3.14));
}
#endif

void Test()
{
  BasicFunctionInterface();
  TryGetParameter();
  //  TrySetParameter();
  //  BasicInvoke();
  //  TryTransformedInvoke();
  //  TryPrintReturn();
  //  Append();
  //  Replace();
  //  TryAppendReplace();
  TryStaticTransform();
  //  TryDynamicTransform();
  //  TryDynamicTransformFunctor();
  //  TryPrintArguments();
}

} // anonymous namespace

int GuideExampleFunctionInterface(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
