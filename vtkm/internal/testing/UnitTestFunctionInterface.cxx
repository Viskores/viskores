//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/internal/FunctionInterface.h>
#include <vtkm/testing/Testing.h>

#include <sstream>
#include <string>

// Disable the test on the overhead to invoke a function. It is unreliable and
// the most critical invoke (for the instance of a worklet) does not directly
// use the Invoke method.
//#define TEST_INVOKE_TIME
#ifdef TEST_INVOKE_TIME
#include <vtkm/cont/Timer.h>
#endif

namespace
{

using Type1 = vtkm::Id;
const Type1 Arg1 = 1234;

using Type2 = vtkm::Float64;
const Type2 Arg2 = 5678.125;

using Type3 = std::string;
const Type3 Arg3("Third argument");

using Type4 = vtkm::Vec<vtkm::Float32, 3>;
const Type4 Arg4(1.2f, 3.4f, 5.6f);

using Type5 = vtkm::Id3;
const Type5 Arg5(4, 5, 6);

struct ThreeArgFunctor
{
  void operator()(const Type1& a1, const Type2& a2, const Type3& a3) const
  {
    std::cout << "In 3 arg functor." << std::endl;

    VTKM_TEST_ASSERT(a1 == Arg1, "Arg 1 incorrect.");
    VTKM_TEST_ASSERT(a2 == Arg2, "Arg 2 incorrect.");
    VTKM_TEST_ASSERT(a3 == Arg3, "Arg 3 incorrect.");
  }
};

struct ThreeArgModifyFunctor
{
  void operator()(Type1& a1, Type2& a2, Type3& a3) const
  {
    std::cout << "In 3 arg modify functor." << std::endl;

    a1 = Arg1;
    a2 = Arg2;
    a3 = Arg3;
  }
};

struct GetReferenceFunctor
{
  template <typename T>
  struct ReturnType
  {
    using type = const typename std::remove_reference<T>::type*;
  };

  template <typename T>
  const T* operator()(const T& x) const
  {
    return &x;
  }
};

struct PointerTransform
{
  template <typename T, vtkm::IdComponent Index>
  struct ReturnType
  {
    using type = const T*;
  };

  template <typename T, typename IndexTag>
  const T* operator()(const T& x, IndexTag) const
  {
    return &x;
  }
};

struct ThreePointerArgFunctor
{
  void operator()(const Type1* a1, const Type2* a2, const Type3* a3) const
  {
    std::cout << "In 3 point arg functor." << std::endl;

    VTKM_TEST_ASSERT(*a1 == Arg1, "Arg 1 incorrect.");
    VTKM_TEST_ASSERT(*a2 == Arg2, "Arg 2 incorrect.");
    VTKM_TEST_ASSERT(*a3 == Arg3, "Arg 3 incorrect.");
  }
};

struct ThreeArgFunctorWithReturn
{
  std::string operator()(const Type1& a1, const Type2& a2, const Type3& a3) const
  {
    std::cout << "In 3 arg functor with return." << std::endl;

    std::stringstream buffer;
    buffer.precision(10);
    buffer << a1 << " " << a2 << " " << a3;
    return buffer.str();
  }
};

struct FiveArgFunctor
{
  void operator()(const Type1& a1,
                  const Type2& a2,
                  const Type3& a3,
                  const Type4& a4,
                  const Type5& a5) const
  {
    std::cout << "In 5 arg functor." << std::endl;

    VTKM_TEST_ASSERT(a1 == Arg1, "Arg 1 incorrect.");
    VTKM_TEST_ASSERT(a2 == Arg2, "Arg 2 incorrect.");
    VTKM_TEST_ASSERT(a3 == Arg3, "Arg 3 incorrect.");
    VTKM_TEST_ASSERT(a4 == Arg4, "Arg 4 incorrect.");
    VTKM_TEST_ASSERT(a5 == Arg5, "Arg 5 incorrect.");
  }
};

struct FiveArgSwizzledFunctor
{
  void operator()(const Type5& a5,
                  const Type1& a1,
                  const Type3& a3,
                  const Type4& a4,
                  const Type2& a2) const
  {
    std::cout << "In 5 arg functor." << std::endl;

    VTKM_TEST_ASSERT(a1 == Arg1, "Arg 1 incorrect.");
    VTKM_TEST_ASSERT(a2 == Arg2, "Arg 2 incorrect.");
    VTKM_TEST_ASSERT(a3 == Arg3, "Arg 3 incorrect.");
    VTKM_TEST_ASSERT(a4 == Arg4, "Arg 4 incorrect.");
    VTKM_TEST_ASSERT(a5 == Arg5, "Arg 5 incorrect.");
  }
};

#ifdef TEST_INVOKE_TIME
struct LotsOfArgsFunctor
{
  LotsOfArgsFunctor()
    : Field(0)
  {
  }

  void operator()(vtkm::FloatDefault arg1,
                  vtkm::FloatDefault arg2,
                  vtkm::FloatDefault arg3,
                  vtkm::FloatDefault arg4,
                  vtkm::FloatDefault arg5,
                  vtkm::FloatDefault arg6,
                  vtkm::FloatDefault arg7,
                  vtkm::FloatDefault arg8,
                  vtkm::FloatDefault arg9,
                  vtkm::FloatDefault arg10)
  {
    VTKM_TEST_ASSERT(arg1 == 1.0, "Got bad argument");
    VTKM_TEST_ASSERT(arg2 == 2.0, "Got bad argument");
    VTKM_TEST_ASSERT(arg3 == 3.0, "Got bad argument");
    VTKM_TEST_ASSERT(arg4 == 4.0, "Got bad argument");
    VTKM_TEST_ASSERT(arg5 == 5.0, "Got bad argument");
    VTKM_TEST_ASSERT(arg6 == 6.0, "Got bad argument");
    VTKM_TEST_ASSERT(arg7 == 7.0, "Got bad argument");
    VTKM_TEST_ASSERT(arg8 == 8.0, "Got bad argument");
    VTKM_TEST_ASSERT(arg9 == 9.0, "Got bad argument");
    VTKM_TEST_ASSERT(arg10 == 10.0, "Got bad argument");

    this->Field += arg1 + arg2 + arg3 + arg4 + arg5 + arg6 + arg7 + arg8 + arg9 + arg10;
  }
  vtkm::FloatDefault Field;
};
#endif //TEST_INVOKE_TIME

template <typename T>
std::string ToString(const T& value)
{
  std::stringstream stream;
  stream.precision(10);
  stream << value;
  return stream.str();
}
std::string ToString(const std::string& value)
{
  return value;
}

struct StringTransform
{
  template <typename T>
  std::string operator()(const T& input) const
  {
    return ToString(input);
  }
};

struct ThreeArgStringFunctorWithReturn
{
  std::string operator()(std::string arg1, std::string arg2, std::string arg3) const
  {
    return arg1 + " " + arg2 + " " + arg3;
  }
};

struct DynamicTransformFunctor
{
  template <typename T, typename ContinueFunctor, vtkm::IdComponent Index>
  void operator()(const T& input,
                  const ContinueFunctor continueFunc,
                  vtkm::internal::IndexTag<Index>) const
  {
    continueFunc(input + T(Index));
    continueFunc(ToString(input + T(Index)));
  }

  template <typename ContinueFunctor, vtkm::IdComponent Index>
  void operator()(const std::string& input,
                  const ContinueFunctor continueFunc,
                  vtkm::internal::IndexTag<Index>) const
  {
    continueFunc(input);
  }
};

vtkm::Id g_DynamicTransformFinishCalls;

struct DynamicTransformFinish
{
  template <typename Signature>
  void operator()(vtkm::internal::FunctionInterface<Signature>& funcInterface) const
  {
    g_DynamicTransformFinishCalls++;
    VTKM_TEST_ASSERT(ToString(funcInterface.template GetParameter<1>()) == ToString(Arg1 + 1),
                     "Arg 1 incorrect");
    VTKM_TEST_ASSERT(ToString(funcInterface.template GetParameter<2>()) == ToString(Arg2 + 2),
                     "Arg 2 incorrect");
    VTKM_TEST_ASSERT(ToString(funcInterface.template GetParameter<3>()) == ToString(Arg3),
                     "Arg 3 incorrect");
  }
};

struct ForEachFunctor
{
  template <typename T, vtkm::IdComponent Index>
  void operator()(T& x, vtkm::internal::IndexTag<Index>) const
  {
    x = T(Index) + x;
  }

  template <vtkm::IdComponent Index>
  void operator()(std::string& x, vtkm::internal::IndexTag<Index>) const
  {
    std::stringstream message;
    message << x << "+" << Index;
    x = message.str();
  }
};

void TryFunctionInterface5(
  vtkm::internal::FunctionInterface<void(Type1, Type2, Type3, Type4, Type5)> funcInterface)
{
  std::cout << "Checking 5 parameter function interface." << std::endl;
  VTKM_TEST_ASSERT(funcInterface.GetArity() == 5, "Got wrong number of parameters.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() == Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<2>() == Arg2, "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() == Arg3, "Arg 3 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<4>() == Arg4, "Arg 4 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<5>() == Arg5, "Arg 5 incorrect.");

  std::cout << "Checking invocation." << std::endl;
  funcInterface.InvokeCont(FiveArgFunctor());
  funcInterface.InvokeExec(FiveArgFunctor());

  std::cout << "Swizzling parameters with replace." << std::endl;
  funcInterface.Replace<1>(Arg5)
    .Replace(Arg1, vtkm::internal::IndexTag<2>())
    .Replace<5>(Arg2)
    .InvokeCont(FiveArgSwizzledFunctor());
}

void TestBasicFunctionInterface()
{
  std::cout << "Creating basic function interface." << std::endl;
  vtkm::internal::FunctionInterface<void(Type1, Type2, Type3)> funcInterface =
    vtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3);

  std::cout << "Checking parameters." << std::endl;
  VTKM_TEST_ASSERT(funcInterface.GetArity() == 3, "Got wrong number of parameters.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() == Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter(vtkm::internal::IndexTag<2>()) == Arg2,
                   "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() == Arg3, "Arg 3 incorrect.");

  std::cout << "Checking invocation." << std::endl;
  funcInterface.InvokeCont(ThreeArgFunctor());
  funcInterface.InvokeExec(ThreeArgFunctor());

  std::cout << "Checking invocation with argument modification." << std::endl;
  funcInterface.SetParameter<1>(Type1());
  funcInterface.SetParameter(Type2(), vtkm::internal::IndexTag<2>());
  funcInterface.SetParameter<3>(Type3());
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() != Arg1, "Arg 1 not cleared.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<2>() != Arg2, "Arg 2 not cleared.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() != Arg3, "Arg 3 not cleared.");

  funcInterface.InvokeCont(ThreeArgModifyFunctor());
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() == Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<2>() == Arg2, "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() == Arg3, "Arg 3 incorrect.");

  TryFunctionInterface5(vtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3, Arg4, Arg5));
}

void TestInvokeResult()
{
  std::cout << "Checking invocation with return." << std::endl;
  vtkm::internal::FunctionInterface<std::string(Type1, Type2, Type3)> funcInterface =
    vtkm::internal::make_FunctionInterface<std::string>(Arg1, Arg2, Arg3);

  funcInterface.InvokeCont(ThreeArgFunctorWithReturn());
  std::string result = funcInterface.GetReturnValue();

  std::cout << "Got result: " << result << std::endl;
  VTKM_TEST_ASSERT(result == "1234 5678.125 Third argument", "Got bad result from invoke.");
}

void TestAppend()
{
  std::cout << "Appending interface with return value." << std::endl;
  vtkm::internal::FunctionInterface<std::string(Type1, Type2)> funcInterface2ArgWRet =
    vtkm::internal::make_FunctionInterface<std::string>(Arg1, Arg2);

  vtkm::internal::FunctionInterface<std::string(Type1, Type2, Type3)> funcInterface3ArgWRet =
    funcInterface2ArgWRet.Append(Arg3);
  VTKM_TEST_ASSERT(funcInterface3ArgWRet.GetParameter<1>() == Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface3ArgWRet.GetParameter<2>() == Arg2, "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface3ArgWRet.GetParameter<3>() == Arg3, "Arg 3 incorrect.");

  std::cout << "Invoking appended function interface." << std::endl;
  funcInterface3ArgWRet.InvokeExec(ThreeArgFunctorWithReturn());
  std::string result = funcInterface3ArgWRet.GetReturnValue();

  std::cout << "Got result: " << result << std::endl;
  VTKM_TEST_ASSERT(result == "1234 5678.125 Third argument", "Got bad result from invoke.");

  std::cout << "Appending another value." << std::endl;
  vtkm::internal::FunctionInterface<std::string(Type1, Type2, Type3, Type4)> funcInterface4ArgWRet =
    funcInterface3ArgWRet.Append(Arg4);
  VTKM_TEST_ASSERT(funcInterface4ArgWRet.GetParameter<4>() == Arg4, "Arg 4 incorrect.");
  VTKM_TEST_ASSERT(funcInterface4ArgWRet.GetReturnValue() == "1234 5678.125 Third argument",
                   "Got bad result from copy.");

  std::cout << "Checking double append." << std::endl;
  vtkm::internal::FunctionInterface<void(Type1, Type2, Type3)> funcInterface3 =
    vtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3);
  TryFunctionInterface5(funcInterface3.Append(Arg4).Append(Arg5));
}

void TestTransformInvoke()
{
  std::cout << "Trying transform invoke." << std::endl;
  vtkm::internal::FunctionInterface<std::string(Type1, Type2, Type3)> funcInterface =
    vtkm::internal::make_FunctionInterface<std::string>(Arg1, Arg2, Arg3);

  funcInterface.InvokeCont(ThreeArgStringFunctorWithReturn(), StringTransform());
  std::string result = funcInterface.GetReturnValue();

  std::cout << "Got result: " << result << std::endl;
  VTKM_TEST_ASSERT(result == "1234 5678.125 Third argument", "Got bad result from invoke.");
}

void TestStaticTransform()
{
  std::cout << "Trying static transform." << std::endl;
  using OriginalType = vtkm::internal::FunctionInterface<void(Type1, Type2, Type3)>;
  OriginalType funcInterface = vtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3);

  std::cout << "Transform with reported type." << std::endl;
  using ReportedType = OriginalType::StaticTransformType<PointerTransform>::type;
  ReportedType funcInterfaceTransform1 = funcInterface.StaticTransformCont(PointerTransform());
  funcInterfaceTransform1.InvokeCont(ThreePointerArgFunctor());
  funcInterfaceTransform1 = funcInterface.StaticTransformExec(PointerTransform());
  funcInterfaceTransform1.InvokeExec(ThreePointerArgFunctor());
}

void TestDynamicTransform()
{
  std::cout << "Trying dynamic transform." << std::endl;
  vtkm::internal::FunctionInterface<void(Type1, Type2, Type3)> funcInterface =
    vtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3);

  g_DynamicTransformFinishCalls = 0;

  funcInterface.DynamicTransformCont(DynamicTransformFunctor(), DynamicTransformFinish());

  // We use an idiosyncrasy of DynamicTransform to call finish with two
  // permutations for every non string argument and one permutation for every
  // string argument. Thus, we expect it to be called 4 times.
  std::cout << "Number of finish calls: " << g_DynamicTransformFinishCalls << std::endl;
  VTKM_TEST_ASSERT(g_DynamicTransformFinishCalls == 4,
                   "DynamicTransform did not call finish the right number of times.");
}

void TestForEach()
{
  std::cout << "Checking running a function on each parameter." << std::endl;
  vtkm::internal::FunctionInterface<void(Type1, Type2, Type3, Type4, Type5)> funcInterface =
    vtkm::internal::make_FunctionInterface<void>(Arg1, Arg2, Arg3, Arg4, Arg5);
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() == Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<2>() == Arg2, "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() == Arg3, "Arg 3 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<4>() == Arg4, "Arg 4 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<5>() == Arg5, "Arg 5 incorrect.");

  funcInterface.ForEachCont(ForEachFunctor());
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() == Type1(1) + Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<2>() == Type2(2) + Arg2, "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() == Arg3 + "+3", "Arg 3 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<4>() == Type4(4) + Arg4, "Arg 4 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<5>() == Type5(5) + Arg5, "Arg 5 incorrect.");

  funcInterface.ForEachExec(ForEachFunctor());
  VTKM_TEST_ASSERT(funcInterface.GetParameter<1>() == Type1(2) + Arg1, "Arg 1 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<2>() == Type2(4) + Arg2, "Arg 2 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<3>() == Arg3 + "+3+3", "Arg 3 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<4>() == Type4(8) + Arg4, "Arg 4 incorrect.");
  VTKM_TEST_ASSERT(funcInterface.GetParameter<5>() == Type5(10) + Arg5, "Arg 5 incorrect.");
}

#ifdef TEST_INVOKE_TIME
void TestInvokeTime()
{
  std::cout << "Checking time to call lots of args lots of times." << std::endl;
  static vtkm::Id NUM_TRIALS = 50000;
  LotsOfArgsFunctor f;

  vtkm::cont::Timer<> timer;
  for (vtkm::Id trial = 0; trial < NUM_TRIALS; trial++)
  {
    f(1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f);
  }
  vtkm::Float64 directCallTime = timer.GetElapsedTime();
  std::cout << "Time for direct call: " << directCallTime << " seconds" << std::endl;

  vtkm::internal::FunctionInterface<void(vtkm::FloatDefault,
                                         vtkm::FloatDefault,
                                         vtkm::FloatDefault,
                                         vtkm::FloatDefault,
                                         vtkm::FloatDefault,
                                         vtkm::FloatDefault,
                                         vtkm::FloatDefault,
                                         vtkm::FloatDefault,
                                         vtkm::FloatDefault,
                                         vtkm::FloatDefault)>
    funcInterface = vtkm::internal::make_FunctionInterface<void>(vtkm::FloatDefault(1),
                                                                 vtkm::FloatDefault(2),
                                                                 vtkm::FloatDefault(3),
                                                                 vtkm::FloatDefault(4),
                                                                 vtkm::FloatDefault(5),
                                                                 vtkm::FloatDefault(6),
                                                                 vtkm::FloatDefault(7),
                                                                 vtkm::FloatDefault(8),
                                                                 vtkm::FloatDefault(9),
                                                                 vtkm::FloatDefault(10));

  timer.Reset();
  for (vtkm::Id trial = 0; trial < NUM_TRIALS; trial++)
  {
    funcInterface.InvokeCont(f);
  }
  vtkm::Float64 invokeCallTime = timer.GetElapsedTime();
  std::cout << "Time for invoking function interface: " << invokeCallTime << " seconds"
            << std::endl;
  std::cout << "Pointless result (making sure compiler computes it) " << f.Field << std::endl;

#if !defined(NDEBUG) && defined(VTKM_MSVC)
  // We expect function calls through the FunctionInterface class to take
  // about the same amount of time as calling the function directly. If
  // optimization is on and functions are properly inlined, this should
  // certainly be the case. Most compilers are close even in debug mode.
  // However, some compilers might add extra debug instructions for each
  // function call, so this might slow things down. Handle that contingency
  // here with just a warning. (Future builds might need to add more cases
  // here.)
  if (invokeCallTime >= 1.05 * directCallTime)
  {
    std::cout << "WARNING: Function interface took longer than expected." << std::endl
              << "That should be corrected in release builds." << std::endl;
  }
#else
  // Might need to disable this for non-release builds.
  VTKM_TEST_ASSERT(invokeCallTime < 1.05 * directCallTime,
                   "Function interface invoke took longer than expected.");
#endif
}
#endif // if TEST_INVOKE_TIME

void TestFunctionInterface()
{
  TestBasicFunctionInterface();
  TestInvokeResult();
  TestAppend();
  TestTransformInvoke();
  TestStaticTransform();
  TestDynamicTransform();
  TestForEach();
#ifdef TEST_INVOKE_TIME
  TestInvokeTime();
#endif //TEST_INVOKE_TIME
}

} // anonymous namespace

int UnitTestFunctionInterface(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestFunctionInterface, argc, argv);
}
