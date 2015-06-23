//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#include <vtkm/Math.h>

#include <vtkm/TypeListTag.h>
#include <vtkm/VecTraits.h>

#include <vtkm/testing/Testing.h>

//-----------------------------------------------------------------------------
namespace {

const vtkm::IdComponent NUM_NUMBERS = 5;
const vtkm::Float64 NumberList[NUM_NUMBERS] = { 0.25, 0.5, 1.0, 2.0, 3.75 };

const vtkm::Float64 NumeratorList[NUM_NUMBERS] =   { 6.5, 5.8, 9.3, 77.0, 0.1 };
const vtkm::Float64 DenominatorList[NUM_NUMBERS] = { 2.3, 1.6, 3.1, 19.0, 0.4 };
const vtkm::Float64 FModRemainderList[NUM_NUMBERS]={ 1.9, 1.0, 0.0,  1.0, 0.1 };
const vtkm::Float64 RemainderList[NUM_NUMBERS] =   {-0.4,-0.6, 0.0,  1.0, 0.1 };
const vtkm::Int64   QuotientList[NUM_NUMBERS] =    { 3  , 4  , 3  ,  4  , 0   };

const vtkm::Float64 XList[NUM_NUMBERS] =           {4.6, 0.1, 73.4, 55.0, 3.75 };
const vtkm::Float64 FractionalList[NUM_NUMBERS] =  {0.6, 0.1,  0.4,  0.0, 0.75 };
const vtkm::Float64 FloorList[NUM_NUMBERS] =       {4.0, 0.0, 73.0, 55.0, 3.0  };
const vtkm::Float64 CeilList[NUM_NUMBERS] =        {5.0, 1.0, 74.0, 55.0, 4.0  };
const vtkm::Float64 RoundList[NUM_NUMBERS] =       {5.0, 0.0, 73.0, 55.0, 4.0  };

//-----------------------------------------------------------------------------
template<typename T>
void PowTest()
{
  std::cout << "Runing power tests." << std::endl;
  for (vtkm::IdComponent index = 0; index < NUM_NUMBERS; index++)
    {
    T x = static_cast<T>(NumberList[index]);
    T powx = vtkm::Pow(x, static_cast<T>(2.0));
    T sqrx = x*x;
    VTKM_TEST_ASSERT(test_equal(powx, sqrx), "Power gave wrong result.");
    }
}

//-----------------------------------------------------------------------------
template<typename VectorType, typename FunctionType>
void RaiseToTest(FunctionType function,
                 typename vtkm::VecTraits<VectorType>::ComponentType exponent)
{
  typedef vtkm::VecTraits<VectorType> Traits;
  typedef typename Traits::ComponentType ComponentType;
  const vtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;

  for (vtkm::IdComponent index = 0;
       index < NUM_NUMBERS - NUM_COMPONENTS + 1;
       index++)
  {
    VectorType original;
    VectorType raiseresult;
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < NUM_COMPONENTS;
         componentIndex++)
    {
      ComponentType x =
          static_cast<ComponentType>(NumberList[componentIndex + index]);
      Traits::SetComponent(original, componentIndex, x);
      Traits::SetComponent(raiseresult, componentIndex, vtkm::Pow(x, exponent));
    }

    VectorType mathresult = function(original);

    VTKM_TEST_ASSERT(test_equal(mathresult, raiseresult),
                    "Exponent functions do not agree.");
  }
}

template<typename VectorType> struct SqrtFunctor {
  VectorType operator()(VectorType x) const { return vtkm::Sqrt(x); }
};
template<typename VectorType>
void SqrtTest()
{
  std::cout << "  Testing Sqrt" << std::endl;
  RaiseToTest<VectorType>(SqrtFunctor<VectorType>(), 0.5);
}

template<typename VectorType> struct RSqrtFunctor {
  VectorType operator()(VectorType x) const {return vtkm::RSqrt(x);}
};
template<typename VectorType>
void RSqrtTest()
{
  std::cout << "  Testing RSqrt"<< std::endl;
  RaiseToTest<VectorType>(RSqrtFunctor<VectorType>(), -0.5);
}

template<typename VectorType> struct CbrtFunctor {
  VectorType operator()(VectorType x) const { return vtkm::Cbrt(x); }
};
template<typename VectorType>
void CbrtTest()
{
  std::cout << "  Testing Cbrt" << std::endl;
  RaiseToTest<VectorType>(CbrtFunctor<VectorType>(), vtkm::Float32(1.0/3.0));
}

template<typename VectorType> struct RCbrtFunctor {
  VectorType operator()(VectorType x) const {return vtkm::RCbrt(x);}
};
template<typename VectorType>
void RCbrtTest()
{
  std::cout << "  Testing RCbrt" << std::endl;
  RaiseToTest<VectorType>(RCbrtFunctor<VectorType>(), vtkm::Float32(-1.0/3.0));
}

//-----------------------------------------------------------------------------
template<typename VectorType, typename FunctionType>
void RaiseByTest(FunctionType function,
                 typename vtkm::VecTraits<VectorType>::ComponentType base,
                 typename vtkm::VecTraits<VectorType>::ComponentType exponentbias = 0.0,
                 typename vtkm::VecTraits<VectorType>::ComponentType resultbias = 0.0)
{
  typedef vtkm::VecTraits<VectorType> Traits;
  typedef typename Traits::ComponentType ComponentType;
  const vtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;

  for (vtkm::IdComponent index = 0;
       index < NUM_NUMBERS - NUM_COMPONENTS + 1;
       index++)
  {
    VectorType original;
    VectorType raiseresult;
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < NUM_COMPONENTS;
         componentIndex++)
    {
      ComponentType x =
          static_cast<ComponentType>(NumberList[componentIndex + index]);
      Traits::SetComponent(original, componentIndex, x);
      Traits::SetComponent(raiseresult,
                           componentIndex,
                           vtkm::Pow(base, x + exponentbias) + resultbias);
    }

    VectorType mathresult = function(original);

    VTKM_TEST_ASSERT(test_equal(mathresult, raiseresult),
                    "Exponent functions do not agree.");
  }
}

template<typename VectorType> struct ExpFunctor {
  VectorType operator()(VectorType x) const {return vtkm::Exp(x);}
};
template<typename VectorType>
void ExpTest()
{
  std::cout << "  Testing Exp" << std::endl;
  RaiseByTest<VectorType>(ExpFunctor<VectorType>(), vtkm::Float32(2.71828183));
}

template<typename VectorType> struct Exp2Functor {
  VectorType operator()(VectorType x) const {return vtkm::Exp2(x);}
};
template<typename VectorType>
void Exp2Test()
{
  std::cout << "  Testing Exp2" << std::endl;
  RaiseByTest<VectorType>(Exp2Functor<VectorType>(), 2.0);
}

template<typename VectorType> struct ExpM1Functor {
  VectorType operator()(VectorType x) const {return vtkm::ExpM1(x);}
};
template<typename VectorType>
void ExpM1Test()
{
  std::cout << "  Testing ExpM1" << std::endl;
  RaiseByTest<VectorType>(ExpM1Functor<VectorType>(),
                          vtkm::Float32(2.71828183),
                          0.0,
                          -1.0);
}

template<typename VectorType> struct Exp10Functor {
  VectorType operator()(VectorType x) const {return vtkm::Exp10(x);}
};
template<typename VectorType>
void Exp10Test()
{
  std::cout << "  Testing Exp10" << std::endl;
  RaiseByTest<VectorType>(Exp10Functor<VectorType>(), 10.0);
}

//-----------------------------------------------------------------------------
void Log2Test()
{
  std::cout << "Testing Log2" << std::endl;
  VTKM_TEST_ASSERT(test_equal(vtkm::Log2(vtkm::Float32(0.25)),
                              vtkm::Float32(-2.0)),
                   "Bad value from Log2");
  VTKM_TEST_ASSERT(
        test_equal(vtkm::Log2(vtkm::Vec<vtkm::Float64,4>(0.5, 1.0, 2.0, 4.0)),
                   vtkm::Vec<vtkm::Float64,4>(-1.0, 0.0, 1.0, 2.0)),
        "Bad value from Log2");
}

template<typename VectorType, typename FunctionType>
void LogBaseTest(FunctionType function,
                 typename vtkm::VecTraits<VectorType>::ComponentType base,
                 typename vtkm::VecTraits<VectorType>::ComponentType bias=0.0)
{
  typedef vtkm::VecTraits<VectorType> Traits;
  typedef typename Traits::ComponentType ComponentType;
  const vtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;

  for (vtkm::IdComponent index = 0;
       index < NUM_NUMBERS - NUM_COMPONENTS + 1;
       index++)
  {
    VectorType basevector(base);
    VectorType original;
    VectorType biased;
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < NUM_COMPONENTS;
         componentIndex++)
    {
      ComponentType x =
          static_cast<ComponentType>(NumberList[componentIndex + index]);
      Traits::SetComponent(original, componentIndex, x);
      Traits::SetComponent(biased, componentIndex, x + bias);
    }

    VectorType logresult = vtkm::Log2(biased)/vtkm::Log2(basevector);

    VectorType mathresult = function(original);

    VTKM_TEST_ASSERT(test_equal(mathresult, logresult),
                    "Exponent functions do not agree.");
  }
}

template<typename VectorType> struct LogFunctor {
  VectorType operator()(VectorType x) const {return vtkm::Log(x);}
};
template<typename VectorType>
void LogTest()
{
  std::cout << "  Testing Log" << std::endl;
  LogBaseTest<VectorType>(LogFunctor<VectorType>(), vtkm::Float32(2.71828183));
}

template<typename VectorType> struct Log10Functor {
  VectorType operator()(VectorType x) const {return vtkm::Log10(x);}
};
template<typename VectorType>
void Log10Test()
{
  std::cout << "  Testing Log10" << std::endl;
  LogBaseTest<VectorType>(Log10Functor<VectorType>(), 10.0);
}

template<typename VectorType> struct Log1PFunctor {
  VectorType operator()(VectorType x) const {return vtkm::Log1P(x);}
};
template<typename VectorType>
void Log1PTest()
{
  std::cout << "  Testing Log1P" << std::endl;
  LogBaseTest<VectorType>(Log1PFunctor<VectorType>(),
                          vtkm::Float32(2.71828183),
                          1.0);
}

//-----------------------------------------------------------------------------
struct TestExpFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    SqrtTest<T>();
    RSqrtTest<T>();
    CbrtTest<T>();
    RCbrtTest<T>();
    ExpTest<T>();
    Exp2Test<T>();
    ExpM1Test<T>();
    Exp10Test<T>();
    LogTest<T>();
    Log10Test<T>();
    Log1PTest<T>();
  }
};

//-----------------------------------------------------------------------------
struct TestMinMaxFunctor
{
  template<typename T>
  void operator()(const T&) const {
    T low = TestValue(2, T());
    T high = TestValue(10, T());
    std::cout << "Testing min/max " << low << " " << high << std::endl;
    VTKM_TEST_ASSERT(test_equal(vtkm::Min(low, high), low), "Wrong min.");
    VTKM_TEST_ASSERT(test_equal(vtkm::Min(high, low), low), "Wrong min.");
    VTKM_TEST_ASSERT(test_equal(vtkm::Max(low, high), high), "Wrong max.");
    VTKM_TEST_ASSERT(test_equal(vtkm::Max(high, low), high), "Wrong max.");
  }
};

//-----------------------------------------------------------------------------
template<typename T>
void TestNonFinites()
{
  std::cout << "Testing non-finites." << std::endl;

  T zero = 0.0;
  T finite = 1.0;
  T nan = vtkm::Nan<T>();
  T inf = vtkm::Infinity<T>();
  T neginf = vtkm::NegativeInfinity<T>();
  T epsilon = vtkm::Epsilon<T>();

  // General behavior.
  VTKM_TEST_ASSERT(nan != nan, "Nan not equal itself.");
  VTKM_TEST_ASSERT(!(nan >= zero), "Nan not greater or less.");
  VTKM_TEST_ASSERT(!(nan <= zero), "Nan not greater or less.");
  VTKM_TEST_ASSERT(!(nan >= finite), "Nan not greater or less.");
  VTKM_TEST_ASSERT(!(nan <= finite), "Nan not greater or less.");

  VTKM_TEST_ASSERT(neginf < inf, "Infinity big");
  VTKM_TEST_ASSERT(zero < inf, "Infinity big");
  VTKM_TEST_ASSERT(finite < inf, "Infinity big");
  VTKM_TEST_ASSERT(zero > -inf, "-Infinity small");
  VTKM_TEST_ASSERT(finite > -inf, "-Infinity small");
  VTKM_TEST_ASSERT(zero > neginf, "-Infinity small");
  VTKM_TEST_ASSERT(finite > neginf, "-Infinity small");

  VTKM_TEST_ASSERT(zero < epsilon, "Negative epsilon");
  VTKM_TEST_ASSERT(finite > epsilon, "Large epsilon");

  // Math check functions.
  VTKM_TEST_ASSERT(!vtkm::IsNan(zero), "Bad IsNan check.");
  VTKM_TEST_ASSERT(!vtkm::IsNan(finite), "Bad IsNan check.");
  VTKM_TEST_ASSERT(vtkm::IsNan(nan), "Bad IsNan check.");
  VTKM_TEST_ASSERT(!vtkm::IsNan(inf), "Bad IsNan check.");
  VTKM_TEST_ASSERT(!vtkm::IsNan(neginf), "Bad IsNan check.");
  VTKM_TEST_ASSERT(!vtkm::IsNan(epsilon), "Bad IsNan check.");

  VTKM_TEST_ASSERT(!vtkm::IsInf(zero), "Bad infinity check.");
  VTKM_TEST_ASSERT(!vtkm::IsInf(finite), "Bad infinity check.");
  VTKM_TEST_ASSERT(!vtkm::IsInf(nan), "Bad infinity check.");
  VTKM_TEST_ASSERT(vtkm::IsInf(inf), "Bad infinity check.");
  VTKM_TEST_ASSERT(vtkm::IsInf(neginf), "Bad infinity check.");
  VTKM_TEST_ASSERT(!vtkm::IsInf(epsilon), "Bad infinity check.");

  VTKM_TEST_ASSERT(vtkm::IsFinite(zero), "Bad finite check.");
  VTKM_TEST_ASSERT(vtkm::IsFinite(finite), "Bad finite check.");
  VTKM_TEST_ASSERT(!vtkm::IsFinite(nan), "Bad finite check.");
  VTKM_TEST_ASSERT(!vtkm::IsFinite(inf), "Bad finite check.");
  VTKM_TEST_ASSERT(!vtkm::IsFinite(neginf), "Bad finite check.");
  VTKM_TEST_ASSERT(vtkm::IsFinite(epsilon), "Bad finite check.");
}

//-----------------------------------------------------------------------------
template<typename T>
void TestRemainders()
{
  std::cout << "Testing remainders." << std::endl;
  for (vtkm::IdComponent index = 0; index < NUM_NUMBERS; index++)
  {
    T numerator = static_cast<T>(NumeratorList[index]);
    T denominator = static_cast<T>(DenominatorList[index]);
    T fmodremainder = static_cast<T>(FModRemainderList[index]);
    T remainder = static_cast<T>(RemainderList[index]);
    vtkm::Int64 quotient = QuotientList[index];

    VTKM_TEST_ASSERT(test_equal(vtkm::FMod(numerator, denominator), fmodremainder),
                     "Bad FMod remainder.");
    VTKM_TEST_ASSERT(test_equal(vtkm::Remainder(numerator, denominator), remainder),
                     "Bad remainder.");
    vtkm::Int64 q;
    VTKM_TEST_ASSERT(test_equal(vtkm::RemainderQuotient(numerator, denominator, q), remainder),
                     "Bad remainder-quotient remainder.");
    VTKM_TEST_ASSERT(test_equal(q, quotient),
                     "Bad reminder-quotient quotient.");
  }
}

//-----------------------------------------------------------------------------
template<typename T>
void TestRound()
{
  std::cout << "Testing round." << std::endl;
  for (vtkm::IdComponent index = 0; index < NUM_NUMBERS; index++)
  {
    T x = static_cast<T>(XList[index]);
    T fractional = static_cast<T>(FractionalList[index]);
    T floor = static_cast<T>(FloorList[index]);
    T ceil = static_cast<T>(CeilList[index]);
    T round = static_cast<T>(RoundList[index]);

    T intPart;
    VTKM_TEST_ASSERT(test_equal(vtkm::ModF(x,intPart), fractional),
                     "ModF returned wrong fractional part.");
    VTKM_TEST_ASSERT(test_equal(intPart, floor),
                     "ModF returned wrong integral part.");
    VTKM_TEST_ASSERT(test_equal(vtkm::Floor(x), floor),
                     "Bad floor.");
    VTKM_TEST_ASSERT(test_equal(vtkm::Ceil(x), ceil),
                     "Bad ceil.");
    VTKM_TEST_ASSERT(test_equal(vtkm::Round(x), round),
                     "Bad round.");
  }
}

void RunMathTests()
{
  PowTest<vtkm::Float32>();
  PowTest<vtkm::Float64>();
  Log2Test();
  Log2Test();
  vtkm::testing::Testing::TryTypes(TestExpFunctor(), vtkm::TypeListTagField());
  vtkm::testing::Testing::TryTypes(TestMinMaxFunctor(), vtkm::TypeListTagScalarAll());
  TestNonFinites<vtkm::Float32>();
  TestNonFinites<vtkm::Float64>();
  TestRemainders<vtkm::Float32>();
  TestRemainders<vtkm::Float64>();
  TestRound<vtkm::Float32>();
  TestRound<vtkm::Float64>();
}

} // Anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestMath(int, char *[])
{
  return vtkm::testing::Testing::Run(RunMathTests);
}
