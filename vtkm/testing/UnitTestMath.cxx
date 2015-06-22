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

void RunMathTests()
{
  PowTest<vtkm::Float32>();
  PowTest<vtkm::Float64>();
  Log2Test();
  Log2Test();
  vtkm::testing::Testing::TryTypes(TestExpFunctor(), vtkm::TypeListTagField());
  vtkm::testing::Testing::TryTypes(TestMinMaxFunctor(), vtkm::TypeListTagScalarAll());
}

} // Anonymous namespace

//-----------------------------------------------------------------------------
int UnitTestMath(int, char *[])
{
  return vtkm::testing::Testing::Run(RunMathTests);
}
