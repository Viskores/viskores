//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/ImplicitFunction.h>
#include <viskores/Math.h>
#include <viskores/VectorAnalysis.h>

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleMultiplexer.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/Initialize.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/Timer.h>

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include "Benchmarker.h"

#include <cctype>
#include <random>
#include <string>
#include <utility>

namespace
{

//==============================================================================
// Benchmark Parameters

#define ARRAY_SIZE (1 << 22)
#define CUBE_SIZE 256

using ValueTypes = viskores::List<viskores::Float32, viskores::Float64>;
using InterpValueTypes = viskores::List<viskores::Float32, viskores::Vec3f_32>;

//==============================================================================
// Worklets and helpers

// Hold configuration state (e.g. active device)
viskores::cont::InitializeResult Config;

template <typename T>
class BlackScholes : public viskores::worklet::WorkletMapField
{
  T Riskfree;
  T Volatility;

public:
  using ControlSignature = void(FieldIn, FieldIn, FieldIn, FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  BlackScholes(T risk, T volatility)
    : Riskfree(risk)
    , Volatility(volatility)
  {
  }

  VISKORES_EXEC
  T CumulativeNormalDistribution(T d) const
  {
    const viskores::Float32 A1 = 0.31938153f;
    const viskores::Float32 A2 = -0.356563782f;
    const viskores::Float32 A3 = 1.781477937f;
    const viskores::Float32 A4 = -1.821255978f;
    const viskores::Float32 A5 = 1.330274429f;
    const viskores::Float32 RSQRT2PI = 0.39894228040143267793994605993438f;

    const viskores::Float32 df = static_cast<viskores::Float32>(d);
    const viskores::Float32 K = 1.0f / (1.0f + 0.2316419f * viskores::Abs(df));

    viskores::Float32 cnd = RSQRT2PI * viskores::Exp(-0.5f * df * df) *
      (K * (A1 + K * (A2 + K * (A3 + K * (A4 + K * A5)))));

    if (df > 0.0f)
    {
      cnd = 1.0f - cnd;
    }

    return static_cast<T>(cnd);
  }

  template <typename U, typename V, typename W>
  VISKORES_EXEC void operator()(const U& sp, const V& os, const W& oy, T& callResult, T& putResult)
    const
  {
    const T stockPrice = static_cast<T>(sp);
    const T optionStrike = static_cast<T>(os);
    const T optionYears = static_cast<T>(oy);

    // Black-Scholes formula for both call and put
    const T sqrtYears = viskores::Sqrt(optionYears);
    const T volMultSqY = this->Volatility * sqrtYears;

    const T d1 = (viskores::Log(stockPrice / optionStrike) +
                  (this->Riskfree + 0.5f * Volatility * Volatility) * optionYears) /
      (volMultSqY);
    const T d2 = d1 - volMultSqY;
    const T CNDD1 = CumulativeNormalDistribution(d1);
    const T CNDD2 = CumulativeNormalDistribution(d2);

    //Calculate Call and Put simultaneously
    T expRT = viskores::Exp(-this->Riskfree * optionYears);
    callResult = stockPrice * CNDD1 - optionStrike * expRT * CNDD2;
    putResult = optionStrike * expRT * (1.0f - CNDD2) - stockPrice * (1.0f - CNDD1);
  }
};

class Mag : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  VISKORES_EXEC void operator()(const viskores::Vec<T, 3>& vec, U& result) const
  {
    result = static_cast<U>(viskores::Magnitude(vec));
  }
};

class Square : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  VISKORES_EXEC void operator()(T input, U& output) const
  {
    output = static_cast<U>(input * input);
  }
};

class Sin : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  VISKORES_EXEC void operator()(T input, U& output) const
  {
    output = static_cast<U>(viskores::Sin(input));
  }
};

class Cos : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  VISKORES_EXEC void operator()(T input, U& output) const
  {
    output = static_cast<U>(viskores::Cos(input));
  }
};

class FusedMath : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  VISKORES_EXEC void operator()(const viskores::Vec<T, 3>& vec, T& result) const
  {
    const T m = viskores::Magnitude(vec);
    result = viskores::Cos(viskores::Sin(m) * viskores::Sin(m));
  }

  template <typename T, typename U>
  VISKORES_EXEC void operator()(const viskores::Vec<T, 3>&, U&) const
  {
    this->RaiseError("Mixed types unsupported.");
  }
};

class GenerateEdges : public viskores::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, WholeArrayOut edgeIds);
  using ExecutionSignature = void(PointIndices, ThreadIndices, _2);
  using InputDomain = _1;

  template <typename ConnectivityInVec, typename ThreadIndicesType, typename IdPairTableType>
  VISKORES_EXEC void operator()(const ConnectivityInVec& connectivity,
                                const ThreadIndicesType threadIndices,
                                const IdPairTableType& edgeIds) const
  {
    const viskores::Id writeOffset = (threadIndices.GetInputIndex() * 12);

    const viskores::IdComponent edgeTable[24] = { 0, 1, 1, 2, 3, 2, 0, 3, 4, 5, 5, 6,
                                                  7, 6, 4, 7, 0, 4, 1, 5, 2, 6, 3, 7 };

    for (viskores::Id i = 0; i < 12; ++i)
    {
      const viskores::Id offset = (i * 2);
      const viskores::Id2 edge(connectivity[edgeTable[offset]],
                               connectivity[edgeTable[offset + 1]]);
      edgeIds.Set(writeOffset + i, edge);
    }
  }
};

class InterpolateField : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn interpolation_ids,
                                FieldIn interpolation_weights,
                                WholeArrayIn inputField,
                                FieldOut output);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  template <typename WeightType, typename PortalType, typename U>
  VISKORES_EXEC void operator()(const viskores::Id2& low_high,
                                const WeightType& weight,
                                const PortalType& inPortal,
                                U& result) const
  {
    using T = typename PortalType::ValueType;
    this->DoIt(low_high, weight, inPortal, result, typename std::is_same<T, U>::type{});
  }

  template <typename WeightType, typename PortalType>
  VISKORES_EXEC void DoIt(const viskores::Id2& low_high,
                          const WeightType& weight,
                          const PortalType& inPortal,
                          typename PortalType::ValueType& result,
                          std::true_type) const
  {
    //fetch the low / high values from inPortal
    result = viskores::Lerp(inPortal.Get(low_high[0]), inPortal.Get(low_high[1]), weight);
  }

  template <typename WeightType, typename PortalType, typename U>
  VISKORES_EXEC void DoIt(const viskores::Id2&,
                          const WeightType&,
                          const PortalType&,
                          U&,
                          std::false_type) const
  {
    //the inPortal and result need to be the same type so this version only
    //exists to generate code when using dynamic arrays
    this->RaiseError("Mixed types unsupported.");
  }
};

class EvaluateImplicitFunction : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, ExecObject);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename VecType, typename ScalarType, typename FunctionType>
  VISKORES_EXEC void operator()(const VecType& point,
                                ScalarType& val,
                                const FunctionType& function) const
  {
    val = function.Value(point);
  }
};

class Evaluate2ImplicitFunctions : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, ExecObject, ExecObject);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename VecType, typename ScalarType, typename FType1, typename FType2>
  VISKORES_EXEC void operator()(const VecType& point,
                                ScalarType& val,
                                const FType1& function1,
                                const FType2& function2) const
  {
    val = function1.Value(point) + function2.Value(point);
  }
};

struct PassThroughFunctor
{
  template <typename T>
  VISKORES_EXEC_CONT T operator()(const T& x) const
  {
    return x;
  }
};

template <typename ArrayHandleType>
using ArrayHandlePassThrough =
  viskores::cont::ArrayHandleTransform<ArrayHandleType, PassThroughFunctor, PassThroughFunctor>;

template <typename ValueType, viskores::IdComponent>
struct JunkArrayHandle : viskores::cont::ArrayHandle<ValueType>
{
};

template <typename ArrayHandleType>
using BMArrayHandleMultiplexer =
  viskores::cont::ArrayHandleMultiplexer<ArrayHandleType,
                                         JunkArrayHandle<typename ArrayHandleType::ValueType, 0>,
                                         JunkArrayHandle<typename ArrayHandleType::ValueType, 1>,
                                         JunkArrayHandle<typename ArrayHandleType::ValueType, 2>,
                                         JunkArrayHandle<typename ArrayHandleType::ValueType, 3>,
                                         JunkArrayHandle<typename ArrayHandleType::ValueType, 4>,
                                         JunkArrayHandle<typename ArrayHandleType::ValueType, 5>,
                                         JunkArrayHandle<typename ArrayHandleType::ValueType, 6>,
                                         JunkArrayHandle<typename ArrayHandleType::ValueType, 7>,
                                         JunkArrayHandle<typename ArrayHandleType::ValueType, 8>,
                                         JunkArrayHandle<typename ArrayHandleType::ValueType, 9>,
                                         ArrayHandlePassThrough<ArrayHandleType>>;

template <typename ArrayHandleType>
BMArrayHandleMultiplexer<ArrayHandleType> make_ArrayHandleMultiplexer0(const ArrayHandleType& array)
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);
  return BMArrayHandleMultiplexer<ArrayHandleType>(array);
}

template <typename ArrayHandleType>
BMArrayHandleMultiplexer<ArrayHandleType> make_ArrayHandleMultiplexerN(const ArrayHandleType& array)
{
  VISKORES_IS_ARRAY_HANDLE(ArrayHandleType);
  return BMArrayHandleMultiplexer<ArrayHandleType>(ArrayHandlePassThrough<ArrayHandleType>(array));
}


//==============================================================================
// Benchmark implementations:

template <typename Value>
struct BenchBlackScholesImpl
{
  using ValueArrayHandle = viskores::cont::ArrayHandle<Value>;

  ValueArrayHandle StockPrice;
  ValueArrayHandle OptionStrike;
  ValueArrayHandle OptionYears;

  ::benchmark::State& State;
  viskores::Id ArraySize;

  viskores::cont::Timer Timer;
  viskores::cont::Invoker Invoker;

  VISKORES_CONT
  BenchBlackScholesImpl(::benchmark::State& state)
    : State{ state }
    , ArraySize{ ARRAY_SIZE }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {

    { // Initialize arrays
      std::mt19937 rng;
      std::uniform_real_distribution<Value> price_range(Value(5.0f), Value(30.0f));
      std::uniform_real_distribution<Value> strike_range(Value(1.0f), Value(100.0f));
      std::uniform_real_distribution<Value> year_range(Value(0.25f), Value(10.0f));

      this->StockPrice.Allocate(this->ArraySize);
      this->OptionStrike.Allocate(this->ArraySize);
      this->OptionYears.Allocate(this->ArraySize);

      auto stockPricePortal = this->StockPrice.WritePortal();
      auto optionStrikePortal = this->OptionStrike.WritePortal();
      auto optionYearsPortal = this->OptionYears.WritePortal();

      for (viskores::Id i = 0; i < this->ArraySize; ++i)
      {
        stockPricePortal.Set(i, price_range(rng));
        optionStrikePortal.Set(i, strike_range(rng));
        optionYearsPortal.Set(i, year_range(rng));
      }
    }

    { // Configure label:
      const viskores::Id numBytes = this->ArraySize * static_cast<viskores::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "NumValues:" << this->ArraySize << " ("
           << viskores::cont::GetHumanReadableSize(numBytes) << ")";
      this->State.SetLabel(desc.str());
    }
  }

  template <typename BenchArrayType>
  VISKORES_CONT void Run(const BenchArrayType& stockPrice,
                         const BenchArrayType& optionStrike,
                         const BenchArrayType& optionYears)
  {
    static constexpr Value RISKFREE = 0.02f;
    static constexpr Value VOLATILITY = 0.30f;

    BlackScholes<Value> worklet(RISKFREE, VOLATILITY);
    viskores::cont::ArrayHandle<Value> callResultHandle;
    viskores::cont::ArrayHandle<Value> putResultHandle;

    for (auto _ : this->State)
    {
      (void)_;
      this->Timer.Start();
      this->Invoker(
        worklet, stockPrice, optionStrike, optionYears, callResultHandle, putResultHandle);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }

    const int64_t iterations = static_cast<int64_t>(this->State.iterations());
    const int64_t numValues = static_cast<int64_t>(this->ArraySize);
    this->State.SetItemsProcessed(numValues * iterations);
  }
};

template <typename ValueType>
void BenchBlackScholesStatic(::benchmark::State& state)
{
  BenchBlackScholesImpl<ValueType> impl{ state };
  impl.Run(impl.StockPrice, impl.OptionStrike, impl.OptionYears);
};
VISKORES_BENCHMARK_TEMPLATES(BenchBlackScholesStatic, ValueTypes);

template <typename ValueType>
void BenchBlackScholesMultiplexer0(::benchmark::State& state)
{
  BenchBlackScholesImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexer0(impl.StockPrice),
           make_ArrayHandleMultiplexer0(impl.OptionStrike),
           make_ArrayHandleMultiplexer0(impl.OptionYears));
};
VISKORES_BENCHMARK_TEMPLATES(BenchBlackScholesMultiplexer0, ValueTypes);

template <typename ValueType>
void BenchBlackScholesMultiplexerN(::benchmark::State& state)
{
  BenchBlackScholesImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexerN(impl.StockPrice),
           make_ArrayHandleMultiplexerN(impl.OptionStrike),
           make_ArrayHandleMultiplexerN(impl.OptionYears));
};
VISKORES_BENCHMARK_TEMPLATES(BenchBlackScholesMultiplexerN, ValueTypes);

template <typename Value>
struct BenchMathImpl
{
  viskores::cont::ArrayHandle<viskores::Vec<Value, 3>> InputHandle;
  viskores::cont::ArrayHandle<Value> TempHandle1;
  viskores::cont::ArrayHandle<Value> TempHandle2;

  ::benchmark::State& State;
  viskores::Id ArraySize;

  viskores::cont::Timer Timer;
  viskores::cont::Invoker Invoker;

  VISKORES_CONT
  BenchMathImpl(::benchmark::State& state)
    : State{ state }
    , ArraySize{ ARRAY_SIZE }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {
    { // Initialize input
      std::mt19937 rng;
      std::uniform_real_distribution<Value> range;

      this->InputHandle.Allocate(this->ArraySize);
      auto portal = this->InputHandle.WritePortal();
      for (viskores::Id i = 0; i < this->ArraySize; ++i)
      {
        portal.Set(i, viskores::Vec<Value, 3>{ range(rng), range(rng), range(rng) });
      }
    }
  }

  template <typename InputArrayType, typename BenchArrayType>
  VISKORES_CONT void Run(const InputArrayType& inputHandle,
                         const BenchArrayType& tempHandle1,
                         const BenchArrayType& tempHandle2)
  {
    { // Configure label:
      const viskores::Id numBytes = this->ArraySize * static_cast<viskores::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "NumValues:" << this->ArraySize << " ("
           << viskores::cont::GetHumanReadableSize(numBytes) << ")";
      this->State.SetLabel(desc.str());
    }

    for (auto _ : this->State)
    {
      (void)_;

      this->Timer.Start();
      this->Invoker(Mag{}, inputHandle, tempHandle1);
      this->Invoker(Sin{}, tempHandle1, tempHandle2);
      this->Invoker(Square{}, tempHandle2, tempHandle1);
      this->Invoker(Cos{}, tempHandle1, tempHandle2);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }

    const int64_t iterations = static_cast<int64_t>(this->State.iterations());
    const int64_t numValues = static_cast<int64_t>(this->ArraySize);
    this->State.SetItemsProcessed(numValues * iterations);
  }
};

template <typename ValueType>
void BenchMathStatic(::benchmark::State& state)
{
  BenchMathImpl<ValueType> impl{ state };
  impl.Run(impl.InputHandle, impl.TempHandle1, impl.TempHandle2);
};
VISKORES_BENCHMARK_TEMPLATES(BenchMathStatic, ValueTypes);

template <typename ValueType>
void BenchMathMultiplexer0(::benchmark::State& state)
{
  BenchMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexer0(impl.InputHandle),
           make_ArrayHandleMultiplexer0(impl.TempHandle1),
           make_ArrayHandleMultiplexer0(impl.TempHandle2));
};
VISKORES_BENCHMARK_TEMPLATES(BenchMathMultiplexer0, ValueTypes);

template <typename ValueType>
void BenchMathMultiplexerN(::benchmark::State& state)
{
  BenchMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexerN(impl.InputHandle),
           make_ArrayHandleMultiplexerN(impl.TempHandle1),
           make_ArrayHandleMultiplexerN(impl.TempHandle2));
};
VISKORES_BENCHMARK_TEMPLATES(BenchMathMultiplexerN, ValueTypes);

template <typename Value>
struct BenchFusedMathImpl
{
  viskores::cont::ArrayHandle<viskores::Vec<Value, 3>> InputHandle;

  ::benchmark::State& State;
  viskores::Id ArraySize;

  viskores::cont::Timer Timer;
  viskores::cont::Invoker Invoker;

  VISKORES_CONT
  BenchFusedMathImpl(::benchmark::State& state)
    : State{ state }
    , ArraySize{ ARRAY_SIZE }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {
    { // Initialize input
      std::mt19937 rng;
      std::uniform_real_distribution<Value> range;

      this->InputHandle.Allocate(this->ArraySize);
      auto portal = this->InputHandle.WritePortal();
      for (viskores::Id i = 0; i < this->ArraySize; ++i)
      {
        portal.Set(i, viskores::Vec<Value, 3>{ range(rng), range(rng), range(rng) });
      }
    }

    { // Configure label:
      const viskores::Id numBytes = this->ArraySize * static_cast<viskores::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "NumValues:" << this->ArraySize << " ("
           << viskores::cont::GetHumanReadableSize(numBytes) << ")";
      this->State.SetLabel(desc.str());
    }
  }

  template <typename BenchArrayType>
  VISKORES_CONT void Run(const BenchArrayType& inputHandle)
  {
    viskores::cont::ArrayHandle<Value> result;

    for (auto _ : this->State)
    {
      (void)_;

      this->Timer.Start();
      this->Invoker(FusedMath{}, inputHandle, result);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }

    const int64_t iterations = static_cast<int64_t>(this->State.iterations());
    const int64_t numValues = static_cast<int64_t>(this->ArraySize);
    this->State.SetItemsProcessed(numValues * iterations);
  }
};

template <typename ValueType>
void BenchFusedMathStatic(::benchmark::State& state)
{
  BenchFusedMathImpl<ValueType> impl{ state };
  impl.Run(impl.InputHandle);
};
VISKORES_BENCHMARK_TEMPLATES(BenchFusedMathStatic, ValueTypes);

template <typename ValueType>
void BenchFusedMathMultiplexer0(::benchmark::State& state)
{
  BenchFusedMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexer0(impl.InputHandle));
};
VISKORES_BENCHMARK_TEMPLATES(BenchFusedMathMultiplexer0, ValueTypes);

template <typename ValueType>
void BenchFusedMathMultiplexerN(::benchmark::State& state)
{
  BenchFusedMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexerN(impl.InputHandle));
};
VISKORES_BENCHMARK_TEMPLATES(BenchFusedMathMultiplexerN, ValueTypes);

template <typename Value>
struct BenchEdgeInterpImpl
{
  viskores::cont::ArrayHandle<viskores::Float32> WeightHandle;
  viskores::cont::ArrayHandle<Value> FieldHandle;
  viskores::cont::ArrayHandle<viskores::Id2> EdgePairHandle;

  ::benchmark::State& State;
  viskores::Id CubeSize;

  viskores::cont::Timer Timer;
  viskores::cont::Invoker Invoker;

  VISKORES_CONT
  BenchEdgeInterpImpl(::benchmark::State& state)
    : State{ state }
    , CubeSize{ CUBE_SIZE }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {
    { // Initialize arrays
      using CT = typename viskores::VecTraits<Value>::ComponentType;

      std::mt19937 rng;
      std::uniform_real_distribution<viskores::Float32> weight_range(0.0f, 1.0f);
      std::uniform_real_distribution<CT> field_range;

      //basically the core challenge is to generate an array whose
      //indexing pattern matches that of a edge based algorithm.
      //
      //So for this kind of problem we generate the 12 edges of each
      //cell and place them into array.
      viskores::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(viskores::Id3{ this->CubeSize, this->CubeSize, this->CubeSize });

      const viskores::Id numberOfEdges = cellSet.GetNumberOfCells() * 12;

      this->EdgePairHandle.Allocate(numberOfEdges);
      this->Invoker(GenerateEdges{}, cellSet, this->EdgePairHandle);

      { // Per-edge weights
        this->WeightHandle.Allocate(numberOfEdges);
        auto portal = this->WeightHandle.WritePortal();
        for (viskores::Id i = 0; i < numberOfEdges; ++i)
        {
          portal.Set(i, weight_range(rng));
        }
      }

      { // Point field
        this->FieldHandle.Allocate(cellSet.GetNumberOfPoints());
        auto portal = this->FieldHandle.WritePortal();
        for (viskores::Id i = 0; i < portal.GetNumberOfValues(); ++i)
        {
          portal.Set(i, field_range(rng));
        }
      }
    }

    { // Configure label:
      const viskores::Id numValues = this->FieldHandle.GetNumberOfValues();
      const viskores::Id numBytes = numValues * static_cast<viskores::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "FieldValues:" << numValues << " (" << viskores::cont::GetHumanReadableSize(numBytes)
           << ") | CubeSize: " << this->CubeSize;
      this->State.SetLabel(desc.str());
    }
  }

  template <typename EdgePairArrayType, typename WeightArrayType, typename FieldArrayType>
  VISKORES_CONT void Run(const EdgePairArrayType& edgePairs,
                         const WeightArrayType& weights,
                         const FieldArrayType& field)
  {
    viskores::cont::ArrayHandle<Value> result;

    for (auto _ : this->State)
    {
      (void)_;
      this->Timer.Start();
      this->Invoker(InterpolateField{}, edgePairs, weights, field, result);
      this->Timer.Stop();

      this->State.SetIterationTime(this->Timer.GetElapsedTime());
    }
  }
};

template <typename ValueType>
void BenchEdgeInterpStatic(::benchmark::State& state)
{
  BenchEdgeInterpImpl<ValueType> impl{ state };
  impl.Run(impl.EdgePairHandle, impl.WeightHandle, impl.FieldHandle);
};
VISKORES_BENCHMARK_TEMPLATES(BenchEdgeInterpStatic, InterpValueTypes);

struct ImplicitFunctionBenchData
{
  viskores::cont::ArrayHandle<viskores::Vec3f> Points;
  viskores::cont::ArrayHandle<viskores::FloatDefault> Result;
  viskores::Sphere Sphere1;
  viskores::Sphere Sphere2;
};

static ImplicitFunctionBenchData MakeImplicitFunctionBenchData()
{
  viskores::Id count = ARRAY_SIZE;
  viskores::FloatDefault bounds[6] = { -2.0f, 2.0f, -2.0f, 2.0f, -2.0f, 2.0f };

  ImplicitFunctionBenchData data;
  data.Points.Allocate(count);
  data.Result.Allocate(count);

  std::default_random_engine rangen;
  std::uniform_real_distribution<viskores::FloatDefault> distx(bounds[0], bounds[1]);
  std::uniform_real_distribution<viskores::FloatDefault> disty(bounds[2], bounds[3]);
  std::uniform_real_distribution<viskores::FloatDefault> distz(bounds[4], bounds[5]);

  auto portal = data.Points.WritePortal();
  for (viskores::Id i = 0; i < count; ++i)
  {
    portal.Set(i, viskores::make_Vec(distx(rangen), disty(rangen), distz(rangen)));
  }

  data.Sphere1 = viskores::Sphere({ 0.22f, 0.33f, 0.44f }, 0.55f);
  data.Sphere2 = viskores::Sphere({ 0.22f, 0.33f, 0.11f }, 0.77f);

  return data;
}

void BenchImplicitFunction(::benchmark::State& state)
{
  using EvalWorklet = EvaluateImplicitFunction;

  const viskores::cont::DeviceAdapterId device = Config.Device;

  auto data = MakeImplicitFunctionBenchData();

  {
    std::ostringstream desc;
    desc << data.Points.GetNumberOfValues() << " points";
    state.SetLabel(desc.str());
  }

  EvalWorklet eval;

  viskores::cont::Timer timer{ device };
  viskores::cont::Invoker invoker{ device };

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(eval, data.Points, data.Result, data.Sphere1);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VISKORES_BENCHMARK(BenchImplicitFunction);

void Bench2ImplicitFunctions(::benchmark::State& state)
{
  using EvalWorklet = Evaluate2ImplicitFunctions;

  const viskores::cont::DeviceAdapterId device = Config.Device;

  auto data = MakeImplicitFunctionBenchData();

  {
    std::ostringstream desc;
    desc << data.Points.GetNumberOfValues() << " points";
    state.SetLabel(desc.str());
  }

  EvalWorklet eval;

  viskores::cont::Timer timer{ device };
  viskores::cont::Invoker invoker{ device };

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(eval, data.Points, data.Result, data.Sphere1, data.Sphere2);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VISKORES_BENCHMARK(Bench2ImplicitFunctions);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse Viskores options:
  auto opts = viskores::cont::InitializeOptions::RequireDevice;

  std::vector<char*> args(argv, argv + argc);
  viskores::bench::detail::InitializeArgs(&argc, args, opts);

  // Parse Viskores options:
  Config = viskores::cont::Initialize(argc, args.data(), opts);

  // This occurs when it is help
  if (opts == viskores::cont::InitializeOptions::None)
  {
    std::cout << Config.Usage << std::endl;
  }
  else
  {
    viskores::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);
  }

  // handle benchmarking related args and run benchmarks:
  VISKORES_EXECUTE_BENCHMARKS(argc, args.data());
}
