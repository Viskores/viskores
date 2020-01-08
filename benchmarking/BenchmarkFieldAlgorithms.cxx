//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Math.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleMultiplexer.h>
#include <vtkm/cont/ArrayHandleVirtual.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ImplicitFunctionHandle.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include "Benchmarker.h"
#include <vtkm/cont/testing/Testing.h>

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

using ValueTypes = vtkm::List<vtkm::Float32, vtkm::Float64>;
using InterpValueTypes = vtkm::List<vtkm::Float32, vtkm::Vec3f_32>;

//==============================================================================
// Worklets and helpers

// Hold configuration state (e.g. active device)
vtkm::cont::InitializeResult Config;

template <typename T>
class BlackScholes : public vtkm::worklet::WorkletMapField
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

  VTKM_EXEC
  T CumulativeNormalDistribution(T d) const
  {
    const vtkm::Float32 A1 = 0.31938153f;
    const vtkm::Float32 A2 = -0.356563782f;
    const vtkm::Float32 A3 = 1.781477937f;
    const vtkm::Float32 A4 = -1.821255978f;
    const vtkm::Float32 A5 = 1.330274429f;
    const vtkm::Float32 RSQRT2PI = 0.39894228040143267793994605993438f;

    const vtkm::Float32 df = static_cast<vtkm::Float32>(d);
    const vtkm::Float32 K = 1.0f / (1.0f + 0.2316419f * vtkm::Abs(df));

    vtkm::Float32 cnd =
      RSQRT2PI * vtkm::Exp(-0.5f * df * df) * (K * (A1 + K * (A2 + K * (A3 + K * (A4 + K * A5)))));

    if (df > 0.0f)
    {
      cnd = 1.0f - cnd;
    }

    return static_cast<T>(cnd);
  }

  template <typename U, typename V, typename W>
  VTKM_EXEC void operator()(const U& sp, const V& os, const W& oy, T& callResult, T& putResult)
    const
  {
    const T stockPrice = static_cast<T>(sp);
    const T optionStrike = static_cast<T>(os);
    const T optionYears = static_cast<T>(oy);

    // Black-Scholes formula for both call and put
    const T sqrtYears = vtkm::Sqrt(optionYears);
    const T volMultSqY = this->Volatility * sqrtYears;

    const T d1 = (vtkm::Log(stockPrice / optionStrike) +
                  (this->Riskfree + 0.5f * Volatility * Volatility) * optionYears) /
      (volMultSqY);
    const T d2 = d1 - volMultSqY;
    const T CNDD1 = CumulativeNormalDistribution(d1);
    const T CNDD2 = CumulativeNormalDistribution(d2);

    //Calculate Call and Put simultaneously
    T expRT = vtkm::Exp(-this->Riskfree * optionYears);
    callResult = stockPrice * CNDD1 - optionStrike * expRT * CNDD2;
    putResult = optionStrike * expRT * (1.0f - CNDD2) - stockPrice * (1.0f - CNDD1);
  }
};

class Mag : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  VTKM_EXEC void operator()(const vtkm::Vec<T, 3>& vec, U& result) const
  {
    result = static_cast<U>(vtkm::Magnitude(vec));
  }
};

class Square : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  VTKM_EXEC void operator()(T input, U& output) const
  {
    output = static_cast<U>(input * input);
  }
};

class Sin : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  VTKM_EXEC void operator()(T input, U& output) const
  {
    output = static_cast<U>(vtkm::Sin(input));
  }
};

class Cos : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T, typename U>
  VTKM_EXEC void operator()(T input, U& output) const
  {
    output = static_cast<U>(vtkm::Cos(input));
  }
};

class FusedMath : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Vec<T, 3>& vec, T& result) const
  {
    const T m = vtkm::Magnitude(vec);
    result = vtkm::Cos(vtkm::Sin(m) * vtkm::Sin(m));
  }

  template <typename T, typename U>
  VTKM_EXEC void operator()(const vtkm::Vec<T, 3>&, U&) const
  {
    this->RaiseError("Mixed types unsupported.");
  }
};

class GenerateEdges : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(CellSetIn cellset, WholeArrayOut edgeIds);
  using ExecutionSignature = void(PointIndices, ThreadIndices, _2);
  using InputDomain = _1;

  template <typename ConnectivityInVec, typename ThreadIndicesType, typename IdPairTableType>
  VTKM_EXEC void operator()(const ConnectivityInVec& connectivity,
                            const ThreadIndicesType threadIndices,
                            const IdPairTableType& edgeIds) const
  {
    const vtkm::Id writeOffset = (threadIndices.GetInputIndex() * 12);

    const vtkm::IdComponent edgeTable[24] = { 0, 1, 1, 2, 3, 2, 0, 3, 4, 5, 5, 6,
                                              7, 6, 4, 7, 0, 4, 1, 5, 2, 6, 3, 7 };

    for (vtkm::Id i = 0; i < 12; ++i)
    {
      const vtkm::Id offset = (i * 2);
      const vtkm::Id2 edge(connectivity[edgeTable[offset]], connectivity[edgeTable[offset + 1]]);
      edgeIds.Set(writeOffset + i, edge);
    }
  }
};

class InterpolateField : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn interpolation_ids,
                                FieldIn interpolation_weights,
                                WholeArrayIn inputField,
                                FieldOut output);
  using ExecutionSignature = void(_1, _2, _3, _4);
  using InputDomain = _1;

  template <typename WeightType, typename T, typename S, typename D>
  VTKM_EXEC void operator()(const vtkm::Id2& low_high,
                            const WeightType& weight,
                            const vtkm::exec::ExecutionWholeArrayConst<T, S, D>& inPortal,
                            T& result) const
  {
    //fetch the low / high values from inPortal
    result = vtkm::Lerp(inPortal.Get(low_high[0]), inPortal.Get(low_high[1]), weight);
  }

  template <typename WeightType, typename T, typename S, typename D, typename U>
  VTKM_EXEC void operator()(const vtkm::Id2&,
                            const WeightType&,
                            const vtkm::exec::ExecutionWholeArrayConst<T, S, D>&,
                            U&) const
  {
    //the inPortal and result need to be the same type so this version only
    //exists to generate code when using dynamic arrays
    this->RaiseError("Mixed types unsupported.");
  }
};

template <typename ImplicitFunction>
class EvaluateImplicitFunction : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  EvaluateImplicitFunction(const ImplicitFunction* function)
    : Function(function)
  {
  }

  template <typename VecType, typename ScalarType>
  VTKM_EXEC void operator()(const VecType& point, ScalarType& val) const
  {
    val = this->Function->Value(point);
  }

private:
  const ImplicitFunction* Function;
};

template <typename T1, typename T2>
class Evaluate2ImplicitFunctions : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  Evaluate2ImplicitFunctions(const T1* f1, const T2* f2)
    : Function1(f1)
    , Function2(f2)
  {
  }

  template <typename VecType, typename ScalarType>
  VTKM_EXEC void operator()(const VecType& point, ScalarType& val) const
  {
    val = this->Function1->Value(point) + this->Function2->Value(point);
  }

private:
  const T1* Function1;
  const T2* Function2;
};

struct PassThroughFunctor
{
  template <typename T>
  VTKM_EXEC_CONT T operator()(const T& x) const
  {
    return x;
  }
};

template <typename ArrayHandleType>
using ArrayHandlePassThrough =
  vtkm::cont::ArrayHandleTransform<ArrayHandleType, PassThroughFunctor, PassThroughFunctor>;

template <typename ValueType, vtkm::IdComponent>
struct JunkArrayHandle : vtkm::cont::ArrayHandle<ValueType>
{
};

template <typename ArrayHandleType>
using BMArrayHandleMultiplexer =
  vtkm::cont::ArrayHandleMultiplexer<ArrayHandleType,
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
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  return BMArrayHandleMultiplexer<ArrayHandleType>(array);
}

template <typename ArrayHandleType>
BMArrayHandleMultiplexer<ArrayHandleType> make_ArrayHandleMultiplexerN(const ArrayHandleType& array)
{
  VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
  return BMArrayHandleMultiplexer<ArrayHandleType>(ArrayHandlePassThrough<ArrayHandleType>(array));
}


//==============================================================================
// Benchmark implementations:

template <typename Value>
struct BenchBlackScholesImpl
{
  using ValueArrayHandle = vtkm::cont::ArrayHandle<Value>;

  ValueArrayHandle StockPrice;
  ValueArrayHandle OptionStrike;
  ValueArrayHandle OptionYears;

  ::benchmark::State& State;
  vtkm::Id ArraySize;

  vtkm::cont::Timer Timer;
  vtkm::cont::Invoker Invoker;

  VTKM_CONT
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

      auto stockPricePortal = this->StockPrice.GetPortalControl();
      auto optionStrikePortal = this->OptionStrike.GetPortalControl();
      auto optionYearsPortal = this->OptionYears.GetPortalControl();

      for (vtkm::Id i = 0; i < this->ArraySize; ++i)
      {
        stockPricePortal.Set(i, price_range(rng));
        optionStrikePortal.Set(i, strike_range(rng));
        optionYearsPortal.Set(i, year_range(rng));
      }
    }

    { // Configure label:
      const vtkm::Id numBytes = this->ArraySize * static_cast<vtkm::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "NumValues:" << this->ArraySize << " (" << vtkm::cont::GetHumanReadableSize(numBytes)
           << ")";
      this->State.SetLabel(desc.str());
    }
  }

  template <typename BenchArrayType>
  VTKM_CONT void Run(const BenchArrayType& stockPrice,
                     const BenchArrayType& optionStrike,
                     const BenchArrayType& optionYears)
  {
    static constexpr Value RISKFREE = 0.02f;
    static constexpr Value VOLATILITY = 0.30f;

    BlackScholes<Value> worklet(RISKFREE, VOLATILITY);
    vtkm::cont::ArrayHandle<Value> callResultHandle;
    vtkm::cont::ArrayHandle<Value> putResultHandle;

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
VTKM_BENCHMARK_TEMPLATES(BenchBlackScholesStatic, ValueTypes);

template <typename ValueType>
void BenchBlackScholesDynamic(::benchmark::State& state)
{
  BenchBlackScholesImpl<ValueType> impl{ state };
  impl.Run(vtkm::cont::make_ArrayHandleVirtual(impl.StockPrice),
           vtkm::cont::make_ArrayHandleVirtual(impl.OptionStrike),
           vtkm::cont::make_ArrayHandleVirtual(impl.OptionYears));
};
VTKM_BENCHMARK_TEMPLATES(BenchBlackScholesDynamic, ValueTypes);

template <typename ValueType>
void BenchBlackScholesMultiplexer0(::benchmark::State& state)
{
  BenchBlackScholesImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexer0(impl.StockPrice),
           make_ArrayHandleMultiplexer0(impl.OptionStrike),
           make_ArrayHandleMultiplexer0(impl.OptionYears));
};
VTKM_BENCHMARK_TEMPLATES(BenchBlackScholesMultiplexer0, ValueTypes);

template <typename ValueType>
void BenchBlackScholesMultiplexerN(::benchmark::State& state)
{
  BenchBlackScholesImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexerN(impl.StockPrice),
           make_ArrayHandleMultiplexerN(impl.OptionStrike),
           make_ArrayHandleMultiplexerN(impl.OptionYears));
};
VTKM_BENCHMARK_TEMPLATES(BenchBlackScholesMultiplexerN, ValueTypes);

template <typename Value>
struct BenchMathImpl
{
  vtkm::cont::ArrayHandle<vtkm::Vec<Value, 3>> InputHandle;
  vtkm::cont::ArrayHandle<Value> TempHandle1;
  vtkm::cont::ArrayHandle<Value> TempHandle2;

  ::benchmark::State& State;
  vtkm::Id ArraySize;

  vtkm::cont::Timer Timer;
  vtkm::cont::Invoker Invoker;

  VTKM_CONT
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
      auto portal = this->InputHandle.GetPortalControl();
      for (vtkm::Id i = 0; i < this->ArraySize; ++i)
      {
        portal.Set(i, vtkm::Vec<Value, 3>{ range(rng), range(rng), range(rng) });
      }
    }
  }

  template <typename InputArrayType, typename BenchArrayType>
  VTKM_CONT void Run(const InputArrayType& inputHandle,
                     const BenchArrayType& tempHandle1,
                     const BenchArrayType& tempHandle2)
  {
    { // Configure label:
      const vtkm::Id numBytes = this->ArraySize * static_cast<vtkm::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "NumValues:" << this->ArraySize << " (" << vtkm::cont::GetHumanReadableSize(numBytes)
           << ")";
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
VTKM_BENCHMARK_TEMPLATES(BenchMathStatic, ValueTypes);

template <typename ValueType>
void BenchMathDynamic(::benchmark::State& state)
{
  BenchMathImpl<ValueType> impl{ state };
  impl.Run(vtkm::cont::make_ArrayHandleVirtual(impl.InputHandle),
           vtkm::cont::make_ArrayHandleVirtual(impl.TempHandle1),
           vtkm::cont::make_ArrayHandleVirtual(impl.TempHandle2));
};
VTKM_BENCHMARK_TEMPLATES(BenchMathDynamic, ValueTypes);

template <typename ValueType>
void BenchMathMultiplexer0(::benchmark::State& state)
{
  BenchMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexer0(impl.InputHandle),
           make_ArrayHandleMultiplexer0(impl.TempHandle1),
           make_ArrayHandleMultiplexer0(impl.TempHandle2));
};
VTKM_BENCHMARK_TEMPLATES(BenchMathMultiplexer0, ValueTypes);

template <typename ValueType>
void BenchMathMultiplexerN(::benchmark::State& state)
{
  BenchMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexerN(impl.InputHandle),
           make_ArrayHandleMultiplexerN(impl.TempHandle1),
           make_ArrayHandleMultiplexerN(impl.TempHandle2));
};
VTKM_BENCHMARK_TEMPLATES(BenchMathMultiplexerN, ValueTypes);

template <typename Value>
struct BenchFusedMathImpl
{
  vtkm::cont::ArrayHandle<vtkm::Vec<Value, 3>> InputHandle;

  ::benchmark::State& State;
  vtkm::Id ArraySize;

  vtkm::cont::Timer Timer;
  vtkm::cont::Invoker Invoker;

  VTKM_CONT
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
      auto portal = this->InputHandle.GetPortalControl();
      for (vtkm::Id i = 0; i < this->ArraySize; ++i)
      {
        portal.Set(i, vtkm::Vec<Value, 3>{ range(rng), range(rng), range(rng) });
      }
    }

    { // Configure label:
      const vtkm::Id numBytes = this->ArraySize * static_cast<vtkm::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "NumValues:" << this->ArraySize << " (" << vtkm::cont::GetHumanReadableSize(numBytes)
           << ")";
      this->State.SetLabel(desc.str());
    }
  }

  template <typename BenchArrayType>
  VTKM_CONT void Run(const BenchArrayType& inputHandle)
  {
    vtkm::cont::ArrayHandle<Value> result;

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
VTKM_BENCHMARK_TEMPLATES(BenchFusedMathStatic, ValueTypes);

template <typename ValueType>
void BenchFusedMathDynamic(::benchmark::State& state)
{
  BenchFusedMathImpl<ValueType> impl{ state };
  impl.Run(vtkm::cont::make_ArrayHandleVirtual(impl.InputHandle));
};
VTKM_BENCHMARK_TEMPLATES(BenchFusedMathDynamic, ValueTypes);

template <typename ValueType>
void BenchFusedMathMultiplexer0(::benchmark::State& state)
{
  BenchFusedMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexer0(impl.InputHandle));
};
VTKM_BENCHMARK_TEMPLATES(BenchFusedMathMultiplexer0, ValueTypes);

template <typename ValueType>
void BenchFusedMathMultiplexerN(::benchmark::State& state)
{
  BenchFusedMathImpl<ValueType> impl{ state };
  impl.Run(make_ArrayHandleMultiplexerN(impl.InputHandle));
};
VTKM_BENCHMARK_TEMPLATES(BenchFusedMathMultiplexerN, ValueTypes);

template <typename Value>
struct BenchEdgeInterpImpl
{
  vtkm::cont::ArrayHandle<vtkm::Float32> WeightHandle;
  vtkm::cont::ArrayHandle<Value> FieldHandle;
  vtkm::cont::ArrayHandle<vtkm::Id2> EdgePairHandle;

  ::benchmark::State& State;
  vtkm::Id CubeSize;

  vtkm::cont::Timer Timer;
  vtkm::cont::Invoker Invoker;

  VTKM_CONT
  BenchEdgeInterpImpl(::benchmark::State& state)
    : State{ state }
    , CubeSize{ CUBE_SIZE }
    , Timer{ Config.Device }
    , Invoker{ Config.Device }
  {
    { // Initialize arrays
      using CT = typename vtkm::VecTraits<Value>::ComponentType;

      std::mt19937 rng;
      std::uniform_real_distribution<vtkm::Float32> weight_range(0.0f, 1.0f);
      std::uniform_real_distribution<CT> field_range;

      //basically the core challenge is to generate an array whose
      //indexing pattern matches that of a edge based algorithm.
      //
      //So for this kind of problem we generate the 12 edges of each
      //cell and place them into array.
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(vtkm::Id3{ this->CubeSize, this->CubeSize, this->CubeSize });

      const vtkm::Id numberOfEdges = cellSet.GetNumberOfCells() * 12;

      this->EdgePairHandle.Allocate(numberOfEdges);
      this->Invoker(GenerateEdges{}, cellSet, this->EdgePairHandle);

      { // Per-edge weights
        this->WeightHandle.Allocate(numberOfEdges);
        auto portal = this->WeightHandle.GetPortalControl();
        for (vtkm::Id i = 0; i < numberOfEdges; ++i)
        {
          portal.Set(i, weight_range(rng));
        }
      }

      { // Point field
        this->FieldHandle.Allocate(cellSet.GetNumberOfPoints());
        auto portal = this->FieldHandle.GetPortalControl();
        for (vtkm::Id i = 0; i < portal.GetNumberOfValues(); ++i)
        {
          portal.Set(i, field_range(rng));
        }
      }
    }

    { // Configure label:
      const vtkm::Id numValues = this->FieldHandle.GetNumberOfValues();
      const vtkm::Id numBytes = numValues * static_cast<vtkm::Id>(sizeof(Value));
      std::ostringstream desc;
      desc << "FieldValues:" << numValues << " (" << vtkm::cont::GetHumanReadableSize(numBytes)
           << ") | CubeSize: " << this->CubeSize;
      this->State.SetLabel(desc.str());
    }
  }

  template <typename EdgePairArrayType, typename WeightArrayType, typename FieldArrayType>
  VTKM_CONT void Run(const EdgePairArrayType& edgePairs,
                     const WeightArrayType& weights,
                     const FieldArrayType& field)
  {
    vtkm::cont::ArrayHandle<Value> result;

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
VTKM_BENCHMARK_TEMPLATES(BenchEdgeInterpStatic, InterpValueTypes);

template <typename ValueType>
void BenchEdgeInterpDynamic(::benchmark::State& state)
{
  BenchEdgeInterpImpl<ValueType> impl{ state };
  impl.Run(vtkm::cont::make_ArrayHandleVirtual(impl.EdgePairHandle),
           vtkm::cont::make_ArrayHandleVirtual(impl.WeightHandle),
           vtkm::cont::make_ArrayHandleVirtual(impl.FieldHandle));
};
VTKM_BENCHMARK_TEMPLATES(BenchEdgeInterpDynamic, InterpValueTypes);

struct ImplicitFunctionBenchData
{
  vtkm::cont::ArrayHandle<vtkm::Vec3f> Points;
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> Result;
  vtkm::Sphere Sphere1;
  vtkm::Sphere Sphere2;
};

static ImplicitFunctionBenchData MakeImplicitFunctionBenchData()
{
  vtkm::Id count = ARRAY_SIZE;
  vtkm::FloatDefault bounds[6] = { -2.0f, 2.0f, -2.0f, 2.0f, -2.0f, 2.0f };

  ImplicitFunctionBenchData data;
  data.Points.Allocate(count);
  data.Result.Allocate(count);

  std::default_random_engine rangen;
  std::uniform_real_distribution<vtkm::FloatDefault> distx(bounds[0], bounds[1]);
  std::uniform_real_distribution<vtkm::FloatDefault> disty(bounds[2], bounds[3]);
  std::uniform_real_distribution<vtkm::FloatDefault> distz(bounds[4], bounds[5]);

  auto portal = data.Points.GetPortalControl();
  for (vtkm::Id i = 0; i < count; ++i)
  {
    portal.Set(i, vtkm::make_Vec(distx(rangen), disty(rangen), distz(rangen)));
  }

  data.Sphere1 = vtkm::Sphere({ 0.22f, 0.33f, 0.44f }, 0.55f);
  data.Sphere2 = vtkm::Sphere({ 0.22f, 0.33f, 0.11f }, 0.77f);

  return data;
}

void BenchImplicitFunction(::benchmark::State& state)
{
  using EvalWorklet = EvaluateImplicitFunction<vtkm::Sphere>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;

  auto data = MakeImplicitFunctionBenchData();

  {
    std::ostringstream desc;
    desc << data.Points.GetNumberOfValues() << " points";
    state.SetLabel(desc.str());
  }

  auto handle = vtkm::cont::make_ImplicitFunctionHandle(data.Sphere1);
  auto function = static_cast<const vtkm::Sphere*>(handle.PrepareForExecution(device));
  EvalWorklet eval(function);

  vtkm::cont::Timer timer{ device };
  vtkm::cont::Invoker invoker{ device };

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(eval, data.Points, data.Result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VTKM_BENCHMARK(BenchImplicitFunction);

void BenchVirtualImplicitFunction(::benchmark::State& state)
{
  using EvalWorklet = EvaluateImplicitFunction<vtkm::ImplicitFunction>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;

  auto data = MakeImplicitFunctionBenchData();

  {
    std::ostringstream desc;
    desc << data.Points.GetNumberOfValues() << " points";
    state.SetLabel(desc.str());
  }

  auto sphere = vtkm::cont::make_ImplicitFunctionHandle(data.Sphere1);
  EvalWorklet eval(sphere.PrepareForExecution(device));

  vtkm::cont::Timer timer{ device };
  vtkm::cont::Invoker invoker{ device };

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(eval, data.Points, data.Result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VTKM_BENCHMARK(BenchVirtualImplicitFunction);

void Bench2ImplicitFunctions(::benchmark::State& state)
{
  using EvalWorklet = Evaluate2ImplicitFunctions<vtkm::Sphere, vtkm::Sphere>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;

  auto data = MakeImplicitFunctionBenchData();

  {
    std::ostringstream desc;
    desc << data.Points.GetNumberOfValues() << " points";
    state.SetLabel(desc.str());
  }

  auto h1 = vtkm::cont::make_ImplicitFunctionHandle(data.Sphere1);
  auto h2 = vtkm::cont::make_ImplicitFunctionHandle(data.Sphere2);
  auto f1 = static_cast<const vtkm::Sphere*>(h1.PrepareForExecution(device));
  auto f2 = static_cast<const vtkm::Sphere*>(h2.PrepareForExecution(device));
  EvalWorklet eval(f1, f2);

  vtkm::cont::Timer timer{ device };
  vtkm::cont::Invoker invoker{ device };

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(eval, data.Points, data.Result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VTKM_BENCHMARK(Bench2ImplicitFunctions);

void Bench2VirtualImplicitFunctions(::benchmark::State& state)
{
  using EvalWorklet = Evaluate2ImplicitFunctions<vtkm::ImplicitFunction, vtkm::ImplicitFunction>;

  const vtkm::cont::DeviceAdapterId device = Config.Device;

  auto data = MakeImplicitFunctionBenchData();

  {
    std::ostringstream desc;
    desc << data.Points.GetNumberOfValues() << " points";
    state.SetLabel(desc.str());
  }

  auto s1 = vtkm::cont::make_ImplicitFunctionHandle(data.Sphere1);
  auto s2 = vtkm::cont::make_ImplicitFunctionHandle(data.Sphere2);
  EvalWorklet eval(s1.PrepareForExecution(device), s2.PrepareForExecution(device));

  vtkm::cont::Timer timer{ device };
  vtkm::cont::Invoker invoker{ device };

  for (auto _ : state)
  {
    (void)_;
    timer.Start();
    invoker(eval, data.Points, data.Result);
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VTKM_BENCHMARK(Bench2VirtualImplicitFunctions);

} // end anon namespace

int main(int argc, char* argv[])
{
  // Parse VTK-m options:
  auto opts = vtkm::cont::InitializeOptions::RequireDevice | vtkm::cont::InitializeOptions::AddHelp;
  Config = vtkm::cont::Initialize(argc, argv, opts);

  // Setup device:
  vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(Config.Device);

  // handle benchmarking related args and run benchmarks:
  VTKM_EXECUTE_BENCHMARKS(argc, argv);
}
