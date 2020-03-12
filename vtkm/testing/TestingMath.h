//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_testing_TestingMath_h
#define vtk_m_testing_TestingMath_h

#include <vtkm/Math.h>

#include <vtkm/TypeList.h>
#include <vtkm/VecTraits.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <limits>

#define VTKM_MATH_ASSERT(condition, message)                                                       \
  if (!(condition))                                                                                \
  {                                                                                                \
    this->RaiseError(message);                                                                     \
  }

//-----------------------------------------------------------------------------
namespace UnitTestMathNamespace
{

class Lists
{
public:
  static constexpr vtkm::IdComponent NUM_NUMBERS = 5;

  VTKM_EXEC_CONT vtkm::Float64 NumberList(vtkm::Int32 i) const
  {
    vtkm::Float64 numberList[NUM_NUMBERS] = { 0.25, 0.5, 1.0, 2.0, 3.75 };
    return numberList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 AngleList(vtkm::Int32 i) const
  {
    vtkm::Float64 angleList[NUM_NUMBERS] = { 0.643501108793284, // angle for 3, 4, 5 triangle.
                                             0.78539816339745,  // pi/4
                                             0.5235987755983,   // pi/6
                                             1.0471975511966,   // pi/3
                                             0.0 };
    return angleList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 OppositeList(vtkm::Int32 i) const
  {
    vtkm::Float64 oppositeList[NUM_NUMBERS] = { 3.0, 1.0, 1.0, 1.732050807568877 /*sqrt(3)*/, 0.0 };
    return oppositeList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 AdjacentList(vtkm::Int32 i) const
  {
    vtkm::Float64 adjacentList[NUM_NUMBERS] = { 4.0, 1.0, 1.732050807568877 /*sqrt(3)*/, 1.0, 1.0 };
    return adjacentList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 HypotenuseList(vtkm::Int32 i) const
  {
    vtkm::Float64 hypotenuseList[NUM_NUMBERS] = {
      5.0, 1.414213562373095 /*sqrt(2)*/, 2.0, 2.0, 1.0
    };
    return hypotenuseList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 NumeratorList(vtkm::Int32 i) const
  {
    vtkm::Float64 numeratorList[NUM_NUMBERS] = { 6.5, 5.8, 9.3, 77.0, 0.1 };
    return numeratorList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 DenominatorList(vtkm::Int32 i) const
  {
    vtkm::Float64 denominatorList[NUM_NUMBERS] = { 2.3, 1.6, 3.1, 19.0, 0.4 };
    return denominatorList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 FModRemainderList(vtkm::Int32 i) const
  {
    vtkm::Float64 fModRemainderList[NUM_NUMBERS] = { 1.9, 1.0, 0.0, 1.0, 0.1 };
    return fModRemainderList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 RemainderList(vtkm::Int32 i) const
  {
    vtkm::Float64 remainderList[NUM_NUMBERS] = { -0.4, -0.6, 0.0, 1.0, 0.1 };
    return remainderList[i];
  }
  VTKM_EXEC_CONT vtkm::Int64 QuotientList(vtkm::Int32 i) const
  {
    vtkm::Int64 quotientList[NUM_NUMBERS] = { 3, 4, 3, 4, 0 };
    return quotientList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 XList(vtkm::Int32 i) const
  {
    vtkm::Float64 xList[NUM_NUMBERS] = { 4.6, 0.1, 73.4, 55.0, 3.75 };
    return xList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 FractionalList(vtkm::Int32 i) const
  {
    vtkm::Float64 fractionalList[NUM_NUMBERS] = { 0.6, 0.1, 0.4, 0.0, 0.75 };
    return fractionalList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 FloorList(vtkm::Int32 i) const
  {
    vtkm::Float64 floorList[NUM_NUMBERS] = { 4.0, 0.0, 73.0, 55.0, 3.0 };
    return floorList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 CeilList(vtkm::Int32 i) const
  {
    vtkm::Float64 ceilList[NUM_NUMBERS] = { 5.0, 1.0, 74.0, 55.0, 4.0 };
    return ceilList[i];
  }
  VTKM_EXEC_CONT vtkm::Float64 RoundList(vtkm::Int32 i) const
  {
    vtkm::Float64 roundList[NUM_NUMBERS] = { 5.0, 0.0, 73.0, 55.0, 4.0 };
    return roundList[i];
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct ScalarFieldTests : public vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void TestPi() const
  {
    //    std::cout << "Testing Pi" << std::endl;
    VTKM_MATH_ASSERT(test_equal(vtkm::Pi(), 3.14159265), "Pi not correct.");
    VTKM_MATH_ASSERT(test_equal(vtkm::Pif(), 3.14159265f), "Pif not correct.");
    VTKM_MATH_ASSERT(test_equal(vtkm::Pi<vtkm::Float64>(), 3.14159265),
                     "Pi template function not correct.");
  }

  VTKM_EXEC
  void TestArcTan2() const
  {
    //    std::cout << "Testing arc tan 2" << std::endl;

    VTKM_MATH_ASSERT(test_equal(vtkm::ATan2(T(0.0), T(1.0)), T(0.0)), "ATan2 x+ axis.");
    VTKM_MATH_ASSERT(test_equal(vtkm::ATan2(T(1.0), T(0.0)), T(0.5 * vtkm::Pi())),
                     "ATan2 y+ axis.");
    VTKM_MATH_ASSERT(test_equal(vtkm::ATan2(T(-1.0), T(0.0)), T(-0.5 * vtkm::Pi())),
                     "ATan2 y- axis.");

    VTKM_MATH_ASSERT(test_equal(vtkm::ATan2(T(1.0), T(1.0)), T(0.25 * vtkm::Pi())),
                     "ATan2 Quadrant 1");
    VTKM_MATH_ASSERT(test_equal(vtkm::ATan2(T(1.0), T(-1.0)), T(0.75 * vtkm::Pi())),
                     "ATan2 Quadrant 2");
    VTKM_MATH_ASSERT(test_equal(vtkm::ATan2(T(-1.0), T(-1.0)), T(-0.75 * vtkm::Pi())),
                     "ATan2 Quadrant 3");
    VTKM_MATH_ASSERT(test_equal(vtkm::ATan2(T(-1.0), T(1.0)), T(-0.25 * vtkm::Pi())),
                     "ATan2 Quadrant 4");
  }

  VTKM_EXEC
  void TestPow() const
  {
    //    std::cout << "Running power tests." << std::endl;
    for (vtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS; index++)
    {
      T x = static_cast<T>(Lists{}.NumberList(index));
      T powx = vtkm::Pow(x, static_cast<T>(2.0));
      T sqrx = x * x;
      VTKM_MATH_ASSERT(test_equal(powx, sqrx), "Power gave wrong result.");
    }
  }

  VTKM_EXEC
  void TestLog2() const
  {
    //    std::cout << "Testing Log2" << std::endl;
    VTKM_MATH_ASSERT(test_equal(vtkm::Log2(T(0.25)), T(-2.0)), "Bad value from Log2");
    VTKM_MATH_ASSERT(test_equal(vtkm::Log2(vtkm::Vec<T, 4>(0.5, 1.0, 2.0, 4.0)),
                                vtkm::Vec<T, 4>(-1.0, 0.0, 1.0, 2.0)),
                     "Bad value from Log2");
  }

  VTKM_EXEC
  void TestNonFinites() const
  {
    //    std::cout << "Testing non-finites." << std::endl;

    T zero = 0.0;
    T finite = 1.0;
    T nan = vtkm::Nan<T>();
    T inf = vtkm::Infinity<T>();
    T neginf = vtkm::NegativeInfinity<T>();
    T epsilon = vtkm::Epsilon<T>();

    // General behavior.
    VTKM_MATH_ASSERT(nan != vtkm::Nan<T>(), "Nan not equal itself.");
    VTKM_MATH_ASSERT(!(nan >= zero), "Nan not greater or less.");
    VTKM_MATH_ASSERT(!(nan <= zero), "Nan not greater or less.");
    VTKM_MATH_ASSERT(!(nan >= finite), "Nan not greater or less.");
    VTKM_MATH_ASSERT(!(nan <= finite), "Nan not greater or less.");

    VTKM_MATH_ASSERT(neginf < inf, "Infinity big");
    VTKM_MATH_ASSERT(zero < inf, "Infinity big");
    VTKM_MATH_ASSERT(finite < inf, "Infinity big");
    VTKM_MATH_ASSERT(zero > -inf, "-Infinity small");
    VTKM_MATH_ASSERT(finite > -inf, "-Infinity small");
    VTKM_MATH_ASSERT(zero > neginf, "-Infinity small");
    VTKM_MATH_ASSERT(finite > neginf, "-Infinity small");

    VTKM_MATH_ASSERT(zero < epsilon, "Negative epsilon");
    VTKM_MATH_ASSERT(finite > epsilon, "Large epsilon");

    // Math check functions.
    VTKM_MATH_ASSERT(!vtkm::IsNan(zero), "Bad IsNan check.");
    VTKM_MATH_ASSERT(!vtkm::IsNan(finite), "Bad IsNan check.");
    VTKM_MATH_ASSERT(vtkm::IsNan(nan), "Bad IsNan check.");
    VTKM_MATH_ASSERT(!vtkm::IsNan(inf), "Bad IsNan check.");
    VTKM_MATH_ASSERT(!vtkm::IsNan(neginf), "Bad IsNan check.");
    VTKM_MATH_ASSERT(!vtkm::IsNan(epsilon), "Bad IsNan check.");

    VTKM_MATH_ASSERT(!vtkm::IsInf(zero), "Bad infinity check.");
    VTKM_MATH_ASSERT(!vtkm::IsInf(finite), "Bad infinity check.");
    VTKM_MATH_ASSERT(!vtkm::IsInf(nan), "Bad infinity check.");
    VTKM_MATH_ASSERT(vtkm::IsInf(inf), "Bad infinity check.");
    VTKM_MATH_ASSERT(vtkm::IsInf(neginf), "Bad infinity check.");
    VTKM_MATH_ASSERT(!vtkm::IsInf(epsilon), "Bad infinity check.");

    VTKM_MATH_ASSERT(vtkm::IsFinite(zero), "Bad finite check.");
    VTKM_MATH_ASSERT(vtkm::IsFinite(finite), "Bad finite check.");
    VTKM_MATH_ASSERT(!vtkm::IsFinite(nan), "Bad finite check.");
    VTKM_MATH_ASSERT(!vtkm::IsFinite(inf), "Bad finite check.");
    VTKM_MATH_ASSERT(!vtkm::IsFinite(neginf), "Bad finite check.");
    VTKM_MATH_ASSERT(vtkm::IsFinite(epsilon), "Bad finite check.");
  }

  VTKM_EXEC
  void TestRemainders() const
  {
    //    std::cout << "Testing remainders." << std::endl;
    Lists table;
    for (vtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS; index++)
    {
      T numerator = static_cast<T>(table.NumeratorList(index));
      T denominator = static_cast<T>(table.DenominatorList(index));
      T fmodremainder = static_cast<T>(table.FModRemainderList(index));
      T remainder = static_cast<T>(table.RemainderList(index));
      vtkm::Int64 quotient = table.QuotientList(index);

      VTKM_MATH_ASSERT(test_equal(vtkm::FMod(numerator, denominator), fmodremainder),
                       "Bad FMod remainder.");
      VTKM_MATH_ASSERT(test_equal(vtkm::Remainder(numerator, denominator), remainder),
                       "Bad remainder.");
      vtkm::Int64 q;
      VTKM_MATH_ASSERT(test_equal(vtkm::RemainderQuotient(numerator, denominator, q), remainder),
                       "Bad remainder-quotient remainder.");
      VTKM_MATH_ASSERT(test_equal(q, quotient), "Bad reminder-quotient quotient.");
    }
  }

  VTKM_EXEC
  void TestRound() const
  {
    //    std::cout << "Testing round." << std::endl;
    Lists table;
    for (vtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS; index++)
    {
      T x = static_cast<T>(table.XList(index));
      T fractional = static_cast<T>(table.FractionalList(index));
      T floor = static_cast<T>(table.FloorList(index));
      T ceil = static_cast<T>(table.CeilList(index));
      T round = static_cast<T>(table.RoundList(index));

      T intPart;
      VTKM_MATH_ASSERT(test_equal(vtkm::ModF(x, intPart), fractional),
                       "ModF returned wrong fractional part.");
      VTKM_MATH_ASSERT(test_equal(intPart, floor), "ModF returned wrong integral part.");
      VTKM_MATH_ASSERT(test_equal(vtkm::Floor(x), floor), "Bad floor.");
      VTKM_MATH_ASSERT(test_equal(vtkm::Ceil(x), ceil), "Bad ceil.");
      VTKM_MATH_ASSERT(test_equal(vtkm::Round(x), round), "Bad round.");
    }
  }

  VTKM_EXEC
  void TestIsNegative() const
  {
    //    std::cout << "Testing SignBit and IsNegative." << std::endl;
    T x = 0;
    VTKM_MATH_ASSERT(vtkm::SignBit(x) == 0, "SignBit wrong for 0.");
    VTKM_MATH_ASSERT(!vtkm::IsNegative(x), "IsNegative wrong for 0.");

    x = 20;
    VTKM_MATH_ASSERT(vtkm::SignBit(x) == 0, "SignBit wrong for 20.");
    VTKM_MATH_ASSERT(!vtkm::IsNegative(x), "IsNegative wrong for 20.");

    x = -20;
    VTKM_MATH_ASSERT(vtkm::SignBit(x) != 0, "SignBit wrong for -20.");
    VTKM_MATH_ASSERT(vtkm::IsNegative(x), "IsNegative wrong for -20.");

    x = 0.02f;
    VTKM_MATH_ASSERT(vtkm::SignBit(x) == 0, "SignBit wrong for 0.02.");
    VTKM_MATH_ASSERT(!vtkm::IsNegative(x), "IsNegative wrong for 0.02.");

    x = -0.02f;
    VTKM_MATH_ASSERT(vtkm::SignBit(x) != 0, "SignBit wrong for -0.02.");
    VTKM_MATH_ASSERT(vtkm::IsNegative(x), "IsNegative wrong for -0.02.");
  }

  VTKM_EXEC
  void operator()(vtkm::Id) const
  {
    this->TestPi();
    this->TestArcTan2();
    this->TestPow();
    this->TestLog2();
    this->TestNonFinites();
    this->TestRemainders();
    this->TestRound();
    this->TestIsNegative();
  }
};

template <typename Device>
struct TryScalarFieldTests
{
  template <typename T>
  void operator()(const T&) const
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(ScalarFieldTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------
template <typename VectorType>
struct ScalarVectorFieldTests : public vtkm::exec::FunctorBase
{
  using Traits = vtkm::VecTraits<VectorType>;
  using ComponentType = typename Traits::ComponentType;
  enum
  {
    NUM_COMPONENTS = Traits::NUM_COMPONENTS
  };

  VTKM_EXEC
  void TestTriangleTrig() const
  {
    //    std::cout << "Testing normal trig functions." << std::endl;
    Lists table;
    for (vtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType angle;
      VectorType opposite;
      VectorType adjacent;
      VectorType hypotenuse;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        Traits::SetComponent(angle,
                             componentIndex,
                             static_cast<ComponentType>(table.AngleList(componentIndex + index)));
        Traits::SetComponent(
          opposite,
          componentIndex,
          static_cast<ComponentType>(table.OppositeList(componentIndex + index)));
        Traits::SetComponent(
          adjacent,
          componentIndex,
          static_cast<ComponentType>(table.AdjacentList(componentIndex + index)));
        Traits::SetComponent(
          hypotenuse,
          componentIndex,
          static_cast<ComponentType>(table.HypotenuseList(componentIndex + index)));
      }

      VTKM_MATH_ASSERT(test_equal(vtkm::Sin(angle), opposite / hypotenuse), "Sin failed test.");
      VTKM_MATH_ASSERT(test_equal(vtkm::Cos(angle), adjacent / hypotenuse), "Cos failed test.");
      VTKM_MATH_ASSERT(test_equal(vtkm::Tan(angle), opposite / adjacent), "Tan failed test.");

      VTKM_MATH_ASSERT(test_equal(vtkm::ASin(opposite / hypotenuse), angle),
                       "Arc Sin failed test.");

#if defined(VTKM_ICC)
      // When the intel compiler has vectorization enabled ( -O2/-O3 ) it converts the
      // `adjacent/hypotenuse` divide operation into reciprocal (rcpps) and
      // multiply (mulps) operations. This causes a change in the expected result that
      // is larger than the default tolerance of test_equal.
      //
      VTKM_MATH_ASSERT(test_equal(vtkm::ACos(adjacent / hypotenuse), angle, 0.0004),
                       "Arc Cos failed test.");
#else
      VTKM_MATH_ASSERT(test_equal(vtkm::ACos(adjacent / hypotenuse), angle),
                       "Arc Cos failed test.");
#endif
      VTKM_MATH_ASSERT(test_equal(vtkm::ATan(opposite / adjacent), angle), "Arc Tan failed test.");
    }
  }

  VTKM_EXEC
  void TestHyperbolicTrig() const
  {
    //    std::cout << "Testing hyperbolic trig functions." << std::endl;

    const VectorType zero(0);
    const VectorType half(0.5);
    Lists table;
    for (vtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType x;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        Traits::SetComponent(
          x, componentIndex, static_cast<ComponentType>(table.AngleList(componentIndex + index)));
      }

      const VectorType minusX = zero - x;

      VTKM_MATH_ASSERT(test_equal(vtkm::SinH(x), half * (vtkm::Exp(x) - vtkm::Exp(minusX))),
                       "SinH does not match definition.");
      VTKM_MATH_ASSERT(test_equal(vtkm::CosH(x), half * (vtkm::Exp(x) + vtkm::Exp(minusX))),
                       "SinH does not match definition.");
      VTKM_MATH_ASSERT(test_equal(vtkm::TanH(x), vtkm::SinH(x) / vtkm::CosH(x)),
                       "TanH does not match definition");

      VTKM_MATH_ASSERT(test_equal(vtkm::ASinH(vtkm::SinH(x)), x), "SinH not inverting.");
      VTKM_MATH_ASSERT(test_equal(vtkm::ACosH(vtkm::CosH(x)), x), "CosH not inverting.");
      VTKM_MATH_ASSERT(test_equal(vtkm::ATanH(vtkm::TanH(x)), x), "TanH not inverting.");
    }
  }

  template <typename FunctionType>
  VTKM_EXEC void RaiseToTest(FunctionType function, ComponentType exponent) const
  {
    Lists table;
    for (vtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType original;
      VectorType raiseresult;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        ComponentType x = static_cast<ComponentType>(table.NumberList(componentIndex + index));
        Traits::SetComponent(original, componentIndex, x);
        Traits::SetComponent(raiseresult, componentIndex, vtkm::Pow(x, exponent));
      }

      VectorType mathresult = function(original);

      VTKM_MATH_ASSERT(test_equal(mathresult, raiseresult), "Exponent functions do not agree.");
    }
  }

  struct SqrtFunctor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::Sqrt(x); }
  };
  VTKM_EXEC
  void TestSqrt() const
  {
    //    std::cout << "  Testing Sqrt" << std::endl;
    RaiseToTest(SqrtFunctor(), 0.5);
  }

  struct RSqrtFunctor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::RSqrt(x); }
  };
  VTKM_EXEC
  void TestRSqrt() const
  {
    //    std::cout << "  Testing RSqrt"<< std::endl;
    RaiseToTest(RSqrtFunctor(), -0.5);
  }

  struct CbrtFunctor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::Cbrt(x); }
  };
  VTKM_EXEC
  void TestCbrt() const
  {
    //    std::cout << "  Testing Cbrt" << std::endl;
    RaiseToTest(CbrtFunctor(), vtkm::Float32(1.0 / 3.0));
  }

  struct RCbrtFunctor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::RCbrt(x); }
  };
  VTKM_EXEC
  void TestRCbrt() const
  {
    //    std::cout << "  Testing RCbrt" << std::endl;
    RaiseToTest(RCbrtFunctor(), vtkm::Float32(-1.0 / 3.0));
  }

  template <typename FunctionType>
  VTKM_EXEC void RaiseByTest(FunctionType function,
                             ComponentType base,
                             ComponentType exponentbias = 0.0,
                             ComponentType resultbias = 0.0) const
  {
    Lists table;
    for (vtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType original;
      VectorType raiseresult;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        ComponentType x = static_cast<ComponentType>(table.NumberList(componentIndex + index));
        Traits::SetComponent(original, componentIndex, x);
        Traits::SetComponent(
          raiseresult, componentIndex, vtkm::Pow(base, x + exponentbias) + resultbias);
      }

      VectorType mathresult = function(original);

      VTKM_MATH_ASSERT(test_equal(mathresult, raiseresult), "Exponent functions do not agree.");
    }
  }

  struct ExpFunctor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::Exp(x); }
  };
  VTKM_EXEC
  void TestExp() const
  {
    //    std::cout << "  Testing Exp" << std::endl;
    RaiseByTest(ExpFunctor(), vtkm::Float32(2.71828183));
  }

  struct Exp2Functor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::Exp2(x); }
  };
  VTKM_EXEC
  void TestExp2() const
  {
    //    std::cout << "  Testing Exp2" << std::endl;
    RaiseByTest(Exp2Functor(), 2.0);
  }

  struct ExpM1Functor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::ExpM1(x); }
  };
  VTKM_EXEC
  void TestExpM1() const
  {
    //    std::cout << "  Testing ExpM1" << std::endl;
    RaiseByTest(ExpM1Functor(), ComponentType(2.71828183), 0.0, -1.0);
  }

  struct Exp10Functor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::Exp10(x); }
  };
  VTKM_EXEC
  void TestExp10() const
  {
    //    std::cout << "  Testing Exp10" << std::endl;
    RaiseByTest(Exp10Functor(), 10.0);
  }

  template <typename FunctionType>
  VTKM_EXEC void LogBaseTest(FunctionType function,
                             ComponentType base,
                             ComponentType bias = 0.0) const
  {
    Lists table;
    for (vtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType basevector(base);
      VectorType original;
      VectorType biased;
      for (vtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        ComponentType x = static_cast<ComponentType>(table.NumberList(componentIndex + index));
        Traits::SetComponent(original, componentIndex, x);
        Traits::SetComponent(biased, componentIndex, x + bias);
      }

      VectorType logresult = vtkm::Log2(biased) / vtkm::Log2(basevector);

      VectorType mathresult = function(original);

      VTKM_MATH_ASSERT(test_equal(mathresult, logresult), "Exponent functions do not agree.");
    }
  }

  struct LogFunctor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::Log(x); }
  };
  VTKM_EXEC
  void TestLog() const
  {
    //    std::cout << "  Testing Log" << std::endl;
    LogBaseTest(LogFunctor(), vtkm::Float32(2.71828183));
  }

  struct Log10Functor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::Log10(x); }
  };
  VTKM_EXEC
  void TestLog10() const
  {
    //    std::cout << "  Testing Log10" << std::endl;
    LogBaseTest(Log10Functor(), 10.0);
  }

  struct Log1PFunctor
  {
    VTKM_EXEC
    VectorType operator()(VectorType x) const { return vtkm::Log1P(x); }
  };
  VTKM_EXEC
  void TestLog1P() const
  {
    //    std::cout << "  Testing Log1P" << std::endl;
    LogBaseTest(Log1PFunctor(), ComponentType(2.71828183), 1.0);
  }

  VTKM_EXEC
  void TestCopySign() const
  {
    //    std::cout << "Testing CopySign." << std::endl;
    // Assuming all TestValues positive.
    VectorType positive1 = TestValue(1, VectorType());
    VectorType positive2 = TestValue(2, VectorType());
    VectorType negative1 = -positive1;
    VectorType negative2 = -positive2;

    VTKM_MATH_ASSERT(test_equal(vtkm::CopySign(positive1, positive2), positive1),
                     "CopySign failed.");
    VTKM_MATH_ASSERT(test_equal(vtkm::CopySign(negative1, positive2), positive1),
                     "CopySign failed.");
    VTKM_MATH_ASSERT(test_equal(vtkm::CopySign(positive1, negative2), negative1),
                     "CopySign failed.");
    VTKM_MATH_ASSERT(test_equal(vtkm::CopySign(negative1, negative2), negative1),
                     "CopySign failed.");
  }

  VTKM_EXEC
  void operator()(vtkm::Id) const
  {
    this->TestTriangleTrig();
    this->TestHyperbolicTrig();
    this->TestSqrt();
    this->TestRSqrt();
    this->TestCbrt();
    this->TestRCbrt();
    this->TestExp();
    this->TestExp2();
    this->TestExpM1();
    this->TestExp10();
    this->TestLog();
    this->TestLog10();
    this->TestLog1P();
    this->TestCopySign();
  }
};

template <typename Device>
struct TryScalarVectorFieldTests
{
  template <typename VectorType>
  void operator()(const VectorType&) const
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(ScalarVectorFieldTests<VectorType>(), 1);
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct AllTypesTests : public vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void TestMinMax() const
  {
    T low = TestValue(2, T());
    T high = TestValue(10, T());
    //    std::cout << "Testing min/max " << low << " " << high << std::endl;
    VTKM_MATH_ASSERT(test_equal(vtkm::Min(low, high), low), "Wrong min.");
    VTKM_MATH_ASSERT(test_equal(vtkm::Min(high, low), low), "Wrong min.");
    VTKM_MATH_ASSERT(test_equal(vtkm::Max(low, high), high), "Wrong max.");
    VTKM_MATH_ASSERT(test_equal(vtkm::Max(high, low), high), "Wrong max.");

    using Traits = vtkm::VecTraits<T>;
    T mixed1 = low;
    T mixed2 = high;
    Traits::SetComponent(mixed1, 0, Traits::GetComponent(high, 0));
    Traits::SetComponent(mixed2, 0, Traits::GetComponent(low, 0));
    VTKM_MATH_ASSERT(test_equal(vtkm::Min(mixed1, mixed2), low), "Wrong min.");
    VTKM_MATH_ASSERT(test_equal(vtkm::Min(mixed2, mixed1), low), "Wrong min.");
    VTKM_MATH_ASSERT(test_equal(vtkm::Max(mixed1, mixed2), high), "Wrong max.");
    VTKM_MATH_ASSERT(test_equal(vtkm::Max(mixed2, mixed1), high), "Wrong max.");
  }

  VTKM_EXEC
  void operator()(vtkm::Id) const { this->TestMinMax(); }
};

template <typename Device>
struct TryAllTypesTests
{
  template <typename T>
  void operator()(const T&) const
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(AllTypesTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct AbsTests : public vtkm::exec::FunctorBase
{
  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    //    std::cout << "Testing Abs." << std::endl;
    T positive = TestValue(index, T()); // Assuming all TestValues positive.
    T negative = -positive;

    VTKM_MATH_ASSERT(test_equal(vtkm::Abs(positive), positive), "Abs returned wrong value.");
    VTKM_MATH_ASSERT(test_equal(vtkm::Abs(negative), positive), "Abs returned wrong value.");
  }
};

template <typename Device>
struct TryAbsTests
{
  template <typename T>
  void operator()(const T&) const
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(AbsTests<T>(), 10);
  }
};

using TypeListAbs =
  vtkm::ListAppend<vtkm::List<vtkm::Int32, vtkm::Int64>, vtkm::TypeListIndex, vtkm::TypeListField>;

//-----------------------------------------------------------------------------
static constexpr vtkm::Id BitOpSamples = 1024 * 1024;

template <typename T>
struct BitOpTests : public vtkm::exec::FunctorBase
{
  static constexpr T MaxT = std::numeric_limits<T>::max();
  static constexpr T Offset = MaxT / BitOpSamples;

  VTKM_EXEC void operator()(vtkm::Id i) const
  {
    const T idx = static_cast<T>(i);
    const T word = idx * this->Offset;

    TestWord(word - idx);
    TestWord(word);
    TestWord(word + idx);
  }

  VTKM_EXEC void TestWord(T word) const
  {
    VTKM_MATH_ASSERT(test_equal(vtkm::CountSetBits(word), this->DumbCountBits(word)),
                     "CountBits returned wrong value.");
    VTKM_MATH_ASSERT(test_equal(vtkm::FindFirstSetBit(word), this->DumbFindFirstSetBit(word)),
                     "FindFirstSetBit returned wrong value.")
  }

  VTKM_EXEC vtkm::Int32 DumbCountBits(T word) const
  {
    vtkm::Int32 bits = 0;
    while (word)
    {
      if (word & 0x1)
      {
        ++bits;
      }
      word >>= 1;
    }
    return bits;
  }

  VTKM_EXEC vtkm::Int32 DumbFindFirstSetBit(T word) const
  {
    if (word == 0)
    {
      return 0;
    }

    vtkm::Int32 bit = 1;
    while ((word & 0x1) == 0)
    {
      word >>= 1;
      ++bit;
    }
    return bit;
  }
};

template <typename Device>
struct TryBitOpTests
{
  template <typename T>
  void operator()(const T&) const
  {
    vtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(BitOpTests<T>(), BitOpSamples);
  }
};

using TypeListBitOp = vtkm::List<vtkm::UInt32, vtkm::UInt64>;

//-----------------------------------------------------------------------------
template <typename Device>
void RunMathTests()
{
  std::cout << "Tests for scalar types." << std::endl;
  vtkm::testing::Testing::TryTypes(TryScalarFieldTests<Device>(), vtkm::TypeListFieldScalar());
  std::cout << "Test for scalar and vector types." << std::endl;
  vtkm::testing::Testing::TryTypes(TryScalarVectorFieldTests<Device>(), vtkm::TypeListField());
  std::cout << "Test for exemplar types." << std::endl;
  vtkm::testing::Testing::TryTypes(TryAllTypesTests<Device>());
  std::cout << "Test all Abs types" << std::endl;
  vtkm::testing::Testing::TryTypes(TryAbsTests<Device>(), TypeListAbs());
  std::cout << "Test all bit operations" << std::endl;
  vtkm::testing::Testing::TryTypes(TryBitOpTests<Device>(), TypeListBitOp());
}

} // namespace UnitTestMathNamespace

#endif //vtk_m_testing_TestingMath_h
