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
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/ImplicitFunctionHandle.h>
#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/cont/VariantArrayHandle.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/Invoker.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include "Benchmarker.h"
#include <vtkm/cont/testing/Testing.h>

#include <cctype>
#include <random>
#include <string>

namespace vtkm
{
namespace benchmarking
{

#define ARRAY_SIZE (1 << 22)
#define CUBE_SIZE 256
static const std::string DIVIDER(40, '-');

enum BenchmarkName
{
  BLACK_SCHOLES = 1,
  MATH = 1 << 1,
  FUSED_MATH = 1 << 2,
  INTERPOLATE_FIELD = 1 << 3,
  IMPLICIT_FUNCTION = 1 << 4,
  ALL = BLACK_SCHOLES | MATH | FUSED_MATH | INTERPOLATE_FIELD | IMPLICIT_FUNCTION
};

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

class GenerateEdges : public vtkm::worklet::WorkletMapPointToCell
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

struct ValueTypes : vtkm::ListTagBase<vtkm::Float32, vtkm::Float64>
{
};

struct InterpValueTypes : vtkm::ListTagBase<vtkm::Float32, vtkm::Vec<vtkm::Float32, 3>>
{
};

/// This class runs a series of micro-benchmarks to measure
/// performance of different field operations
class BenchmarkFieldAlgorithms
{
  using StorageTag = vtkm::cont::StorageTagBasic;

  using Timer = vtkm::cont::Timer;

  using ValueVariantHandle = vtkm::cont::VariantArrayHandleBase<ValueTypes>;
  using InterpVariantHandle = vtkm::cont::VariantArrayHandleBase<InterpValueTypes>;
  using EdgeIdVariantHandle = vtkm::cont::VariantArrayHandleBase<vtkm::TypeListTagId2>;

private:
  template <typename Value, typename DeviceAdapter>
  struct BenchBlackScholes
  {
    using ValueArrayHandle = vtkm::cont::ArrayHandle<Value, StorageTag>;

    ValueArrayHandle StockPrice;
    ValueArrayHandle OptionStrike;
    ValueArrayHandle OptionYears;

    std::vector<Value> price;
    std::vector<Value> strike;
    std::vector<Value> years;

    VTKM_CONT
    BenchBlackScholes()
    {
      std::mt19937 rng;
      std::uniform_real_distribution<Value> price_range(Value(5.0f), Value(30.0f));
      std::uniform_real_distribution<Value> strike_range(Value(1.0f), Value(100.0f));
      std::uniform_real_distribution<Value> year_range(Value(0.25f), Value(10.0f));

      this->price.resize(ARRAY_SIZE);
      this->strike.resize(ARRAY_SIZE);
      this->years.resize(ARRAY_SIZE);
      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        this->price[i] = price_range(rng);
        this->strike[i] = strike_range(rng);
        this->years[i] = year_range(rng);
      }

      this->StockPrice = vtkm::cont::make_ArrayHandle(this->price);
      this->OptionStrike = vtkm::cont::make_ArrayHandle(this->strike);
      this->OptionYears = vtkm::cont::make_ArrayHandle(this->years);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::ArrayHandle<Value> callResultHandle, putResultHandle;
      const Value RISKFREE = 0.02f;
      const Value VOLATILITY = 0.30f;

      Timer timer{ DeviceAdapter() };
      timer.Start();
      BlackScholes<Value> worklet(RISKFREE, VOLATILITY);
      vtkm::worklet::DispatcherMapField<BlackScholes<Value>> dispatcher(worklet);

      dispatcher.Invoke(
        this->StockPrice, this->OptionStrike, this->OptionYears, callResultHandle, putResultHandle);

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Static"); }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "BlackScholes "
                  << "[" << this->Type() << "] "
                  << " with a domain size of: " << ARRAY_SIZE;
      return description.str();
    }
  };

  template <typename Value, typename DeviceAdapter>
  struct BenchBlackScholesDynamic : public BenchBlackScholes<Value, DeviceAdapter>
  {

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      ValueVariantHandle dstocks(this->StockPrice);
      ValueVariantHandle dstrikes(this->OptionStrike);
      ValueVariantHandle doptions(this->OptionYears);

      vtkm::cont::ArrayHandle<Value> callResultHandle, putResultHandle;
      const Value RISKFREE = 0.02f;
      const Value VOLATILITY = 0.30f;

      Timer timer{ DeviceAdapter() };
      timer.Start();
      BlackScholes<Value> worklet(RISKFREE, VOLATILITY);
      vtkm::worklet::DispatcherMapField<BlackScholes<Value>> dispatcher(worklet);

      dispatcher.Invoke(dstocks, dstrikes, doptions, callResultHandle, putResultHandle);

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Dynamic"); }
  };

  VTKM_MAKE_BENCHMARK(BlackScholes, BenchBlackScholes);
  VTKM_MAKE_BENCHMARK(BlackScholesDynamic, BenchBlackScholesDynamic);

  template <typename Value, typename DeviceAdapter>
  struct BenchMath
  {
    std::vector<vtkm::Vec<Value, 3>> input;
    vtkm::cont::ArrayHandle<vtkm::Vec<Value, 3>, StorageTag> InputHandle;

    VTKM_CONT
    BenchMath()
    {
      std::mt19937 rng;
      std::uniform_real_distribution<Value> range;

      this->input.resize(ARRAY_SIZE);
      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        this->input[i] = vtkm::Vec<Value, 3>(range(rng), range(rng), range(rng));
      }

      this->InputHandle = vtkm::cont::make_ArrayHandle(this->input);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::ArrayHandle<Value> tempHandle1;
      vtkm::cont::ArrayHandle<Value> tempHandle2;

      Timer timer{ DeviceAdapter() };
      timer.Start();

      vtkm::worklet::Invoker invoke(DeviceAdapter{});
      invoke(Mag{}, this->InputHandle, tempHandle1);
      invoke(Sin{}, tempHandle1, tempHandle2);
      invoke(Square{}, tempHandle2, tempHandle1);
      invoke(Cos{}, tempHandle1, tempHandle2);

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Static"); }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "Magnitude -> Sine -> Square -> Cosine "
                  << "[" << this->Type() << "] "
                  << "with a domain size of: " << ARRAY_SIZE;
      return description.str();
    }
  };

  template <typename Value, typename DeviceAdapter>
  struct BenchMathDynamic : public BenchMath<Value, DeviceAdapter>
  {

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      using MathTypes = vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 3>, vtkm::Vec<vtkm::Float64, 3>>;

      vtkm::cont::ArrayHandle<Value> temp1;
      vtkm::cont::ArrayHandle<Value> temp2;
      vtkm::cont::VariantArrayHandleBase<MathTypes> dinput(this->InputHandle);
      ValueVariantHandle dtemp1(temp1);
      ValueVariantHandle dtemp2(temp2);

      Timer timer{ DeviceAdapter() };
      timer.Start();

      vtkm::worklet::Invoker invoke(DeviceAdapter{});
      invoke(Mag{}, dinput, dtemp1);
      invoke(Sin{}, dtemp1, dtemp2);
      invoke(Square{}, dtemp2, dtemp1);
      invoke(Cos{}, dtemp1, dtemp2);

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Dynamic"); }
  };

  VTKM_MAKE_BENCHMARK(Math, BenchMath);
  VTKM_MAKE_BENCHMARK(MathDynamic, BenchMathDynamic);

  template <typename Value, typename DeviceAdapter>
  struct BenchFusedMath
  {
    std::vector<vtkm::Vec<Value, 3>> input;
    vtkm::cont::ArrayHandle<vtkm::Vec<Value, 3>, StorageTag> InputHandle;

    VTKM_CONT
    BenchFusedMath()
    {
      std::mt19937 rng;
      std::uniform_real_distribution<Value> range;

      this->input.resize(ARRAY_SIZE);
      for (std::size_t i = 0; i < ARRAY_SIZE; ++i)
      {
        this->input[i] = vtkm::Vec<Value, 3>(range(rng), range(rng), range(rng));
      }

      this->InputHandle = vtkm::cont::make_ArrayHandle(this->input);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::ArrayHandle<Value> result;

      Timer timer{ DeviceAdapter() };
      timer.Start();
      vtkm::worklet::DispatcherMapField<FusedMath> dispatcher;
      dispatcher.Invoke(this->InputHandle, result);

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Static"); }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "Fused Magnitude -> Sine -> Square -> Cosine "
                  << "[" << this->Type() << "] "
                  << "with a domain size of: " << ARRAY_SIZE;
      return description.str();
    }
  };

  template <typename Value, typename DeviceAdapter>
  struct BenchFusedMathDynamic : public BenchFusedMath<Value, DeviceAdapter>
  {

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      using MathTypes = vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 3>, vtkm::Vec<vtkm::Float64, 3>>;

      vtkm::cont::VariantArrayHandleBase<MathTypes> dinput(this->InputHandle);

      vtkm::cont::ArrayHandle<Value, StorageTag> result;

      Timer timer{ DeviceAdapter() };
      timer.Start();
      vtkm::worklet::DispatcherMapField<FusedMath> dispatcher;
      dispatcher.Invoke(dinput, result);

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Dynamic"); }
  };

  VTKM_MAKE_BENCHMARK(FusedMath, BenchFusedMath);
  VTKM_MAKE_BENCHMARK(FusedMathDynamic, BenchFusedMathDynamic);

  template <typename Value, typename DeviceAdapter>
  struct BenchEdgeInterp
  {
    std::vector<vtkm::Float32> weight;
    std::vector<Value> field;

    vtkm::cont::ArrayHandle<vtkm::Float32, StorageTag> WeightHandle;
    vtkm::cont::ArrayHandle<Value, StorageTag> FieldHandle;
    vtkm::cont::ArrayHandle<vtkm::Id2, StorageTag> EdgePairHandle;

    VTKM_CONT
    BenchEdgeInterp()
    {
      using CT = typename vtkm::VecTraits<Value>::ComponentType;
      std::mt19937 rng;
      std::uniform_real_distribution<vtkm::Float32> weight_range(0.0f, 1.0f);
      std::uniform_real_distribution<CT> field_range;

      //basically the core challenge is to generate an array whose
      //indexing pattern matches that of a edge based algorithm.
      //
      //So for this kind of problem we generate the 12 edges of each
      //cell and place them into array.
      //
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions(vtkm::Id3(CUBE_SIZE, CUBE_SIZE, CUBE_SIZE));

      const vtkm::Id numberOfEdges = cellSet.GetNumberOfCells() * 12;
      const std::size_t esize = static_cast<std::size_t>(numberOfEdges);
      const std::size_t psize = static_cast<std::size_t>(cellSet.GetNumberOfPoints());

      this->EdgePairHandle.Allocate(numberOfEdges);
      vtkm::worklet::DispatcherMapTopology<GenerateEdges> dispatcher;
      dispatcher.Invoke(cellSet, this->EdgePairHandle);

      this->weight.resize(esize);
      for (std::size_t i = 0; i < esize; ++i)
      {
        this->weight[i] = weight_range(rng);
      }

      this->field.resize(psize);
      for (std::size_t i = 0; i < psize; ++i)
      {
        this->field[i] = Value(field_range(rng));
      }

      this->FieldHandle = vtkm::cont::make_ArrayHandle(this->field);
      this->WeightHandle = vtkm::cont::make_ArrayHandle(this->weight);
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      vtkm::cont::ArrayHandle<Value> result;

      Timer timer{ DeviceAdapter() };
      timer.Start();
      vtkm::worklet::DispatcherMapField<InterpolateField> dispatcher;
      dispatcher.Invoke(this->EdgePairHandle, this->WeightHandle, this->FieldHandle, result);

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Static"); }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      const std::size_t size = (CUBE_SIZE - 1) * (CUBE_SIZE - 1) * (CUBE_SIZE - 1) * 12;
      description << "Edge Interpolation "
                  << "[" << this->Type() << "] "
                  << "with a domain size of: " << size;
      return description.str();
    }
  };

  template <typename Value, typename DeviceAdapter>
  struct BenchEdgeInterpDynamic : public BenchEdgeInterp<Value, DeviceAdapter>
  {

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      InterpVariantHandle dfield(this->FieldHandle);
      ValueVariantHandle dweight(this->WeightHandle);
      EdgeIdVariantHandle dedges(this->EdgePairHandle);
      vtkm::cont::ArrayHandle<Value> result;

      Timer timer{ DeviceAdapter() };
      timer.Start();
      vtkm::worklet::DispatcherMapField<InterpolateField> dispatcher;
      dispatcher.Invoke(dedges, dweight, dfield, result);

      return timer.GetElapsedTime();
    }

    virtual std::string Type() const { return std::string("Dynamic"); }
  };

  VTKM_MAKE_BENCHMARK(EdgeInterp, BenchEdgeInterp);
  VTKM_MAKE_BENCHMARK(EdgeInterpDynamic, BenchEdgeInterpDynamic);

  struct ImplicitFunctionBenchData
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> Points;
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> Result;
    vtkm::Sphere Sphere1, Sphere2;
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

  template <typename Value, typename DeviceAdapter>
  struct BenchImplicitFunction
  {
    BenchImplicitFunction()
      : Internal(MakeImplicitFunctionBenchData())
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      using EvalWorklet = EvaluateImplicitFunction<vtkm::Sphere>;
      using EvalDispatcher = vtkm::worklet::DispatcherMapField<EvalWorklet>;

      auto handle = vtkm::cont::make_ImplicitFunctionHandle(Internal.Sphere1);
      auto function = static_cast<const vtkm::Sphere*>(handle.PrepareForExecution(DeviceAdapter()));
      EvalWorklet eval(function);

      Timer timer{ DeviceAdapter() };
      timer.Start();
      EvalDispatcher dispatcher(eval);
      dispatcher.Invoke(this->Internal.Points, this->Internal.Result);

      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "Implicit Function (vtkm::Sphere) on " << Internal.Points.GetNumberOfValues()
                  << " points";
      return description.str();
    }

    ImplicitFunctionBenchData Internal;
  };

  template <typename Value, typename DeviceAdapter>
  struct BenchVirtualImplicitFunction
  {
    BenchVirtualImplicitFunction()
      : Internal(MakeImplicitFunctionBenchData())
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      using EvalWorklet = EvaluateImplicitFunction<vtkm::ImplicitFunction>;
      using EvalDispatcher = vtkm::worklet::DispatcherMapField<EvalWorklet>;

      auto sphere = vtkm::cont::make_ImplicitFunctionHandle(Internal.Sphere1);
      EvalWorklet eval(sphere.PrepareForExecution(DeviceAdapter()));

      Timer timer{ DeviceAdapter() };
      timer.Start();
      EvalDispatcher dispatcher(eval);
      dispatcher.Invoke(this->Internal.Points, this->Internal.Result);

      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "Implicit Function (VirtualImplicitFunction) on "
                  << Internal.Points.GetNumberOfValues() << " points";
      return description.str();
    }

    ImplicitFunctionBenchData Internal;
  };

  template <typename Value, typename DeviceAdapter>
  struct Bench2ImplicitFunctions
  {
    Bench2ImplicitFunctions()
      : Internal(MakeImplicitFunctionBenchData())
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      using EvalWorklet = Evaluate2ImplicitFunctions<vtkm::Sphere, vtkm::Sphere>;
      using EvalDispatcher = vtkm::worklet::DispatcherMapField<EvalWorklet>;

      auto h1 = vtkm::cont::make_ImplicitFunctionHandle(Internal.Sphere1);
      auto h2 = vtkm::cont::make_ImplicitFunctionHandle(Internal.Sphere2);
      auto f1 = static_cast<const vtkm::Sphere*>(h1.PrepareForExecution(DeviceAdapter()));
      auto f2 = static_cast<const vtkm::Sphere*>(h2.PrepareForExecution(DeviceAdapter()));
      EvalWorklet eval(f1, f2);

      Timer timer{ DeviceAdapter() };
      timer.Start();
      EvalDispatcher dispatcher(eval);
      dispatcher.Invoke(this->Internal.Points, this->Internal.Result);

      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "Implicit Function 2x(vtkm::Sphere) on " << Internal.Points.GetNumberOfValues()
                  << " points";
      return description.str();
    }

    ImplicitFunctionBenchData Internal;
  };

  template <typename Value, typename DeviceAdapter>
  struct Bench2VirtualImplicitFunctions
  {
    Bench2VirtualImplicitFunctions()
      : Internal(MakeImplicitFunctionBenchData())
    {
    }

    VTKM_CONT
    vtkm::Float64 operator()()
    {
      using EvalWorklet =
        Evaluate2ImplicitFunctions<vtkm::ImplicitFunction, vtkm::ImplicitFunction>;
      using EvalDispatcher = vtkm::worklet::DispatcherMapField<EvalWorklet>;

      auto s1 = vtkm::cont::make_ImplicitFunctionHandle(Internal.Sphere1);
      auto s2 = vtkm::cont::make_ImplicitFunctionHandle(Internal.Sphere2);
      EvalWorklet eval(s1.PrepareForExecution(DeviceAdapter()),
                       s2.PrepareForExecution(DeviceAdapter()));

      Timer timer{ DeviceAdapter() };
      timer.Start();
      EvalDispatcher dispatcher(eval);
      dispatcher.Invoke(this->Internal.Points, this->Internal.Result);

      return timer.GetElapsedTime();
    }

    VTKM_CONT
    std::string Description() const
    {
      std::stringstream description;
      description << "Implicit Function 2x(VirtualImplicitFunction) on "
                  << Internal.Points.GetNumberOfValues() << " points";
      return description.str();
    }

    ImplicitFunctionBenchData Internal;
  };

  VTKM_MAKE_BENCHMARK(ImplicitFunction, BenchImplicitFunction);
  VTKM_MAKE_BENCHMARK(ImplicitFunctionVirtual, BenchVirtualImplicitFunction);
  VTKM_MAKE_BENCHMARK(ImplicitFunction2, Bench2ImplicitFunctions);
  VTKM_MAKE_BENCHMARK(ImplicitFunctionVirtual2, Bench2VirtualImplicitFunctions);

public:
  static VTKM_CONT int Run(int benchmarks, vtkm::cont::DeviceAdapterId id)
  {
    std::cout << DIVIDER << "\nRunning Field Algorithm benchmarks\n";

    if (benchmarks & BLACK_SCHOLES)
    {
      std::cout << DIVIDER << "\nBenchmarking BlackScholes\n";
      VTKM_RUN_BENCHMARK(BlackScholes, ValueTypes(), id);
      VTKM_RUN_BENCHMARK(BlackScholesDynamic, ValueTypes(), id);
    }

    if (benchmarks & MATH)
    {
      std::cout << DIVIDER << "\nBenchmarking Multiple Math Worklets\n";
      VTKM_RUN_BENCHMARK(Math, ValueTypes(), id);
      VTKM_RUN_BENCHMARK(MathDynamic, ValueTypes(), id);
    }

    if (benchmarks & FUSED_MATH)
    {
      std::cout << DIVIDER << "\nBenchmarking Single Fused Math Worklet\n";
      VTKM_RUN_BENCHMARK(FusedMath, ValueTypes(), id);
      VTKM_RUN_BENCHMARK(FusedMathDynamic, ValueTypes(), id);
    }

    if (benchmarks & INTERPOLATE_FIELD)
    {
      std::cout << DIVIDER << "\nBenchmarking Edge Based Field InterpolationWorklet\n";
      VTKM_RUN_BENCHMARK(EdgeInterp, InterpValueTypes(), id);
      VTKM_RUN_BENCHMARK(EdgeInterpDynamic, InterpValueTypes(), id);
    }

    if (benchmarks & IMPLICIT_FUNCTION)
    {
      using FloatDefaultType = vtkm::ListTagBase<vtkm::FloatDefault>;

      std::cout << "\nBenchmarking Implicit Function\n";
      VTKM_RUN_BENCHMARK(ImplicitFunction, FloatDefaultType(), id);
      VTKM_RUN_BENCHMARK(ImplicitFunctionVirtual, FloatDefaultType(), id);
      VTKM_RUN_BENCHMARK(ImplicitFunction2, FloatDefaultType(), id);
      VTKM_RUN_BENCHMARK(ImplicitFunctionVirtual2, FloatDefaultType(), id);
    }

    return 0;
  }
};

#undef ARRAY_SIZE
}
} // namespace vtkm::benchmarking

int main(int argc, char* argv[])
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = vtkm::cont::Initialize(argc, argv, opts);

  int benchmarks = 0;
  if (argc < 2)
  {
    benchmarks = vtkm::benchmarking::ALL;
  }
  else
  {
    for (int i = 1; i < argc; ++i)
    {
      std::string arg = argv[i];
      std::transform(arg.begin(), arg.end(), arg.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      });
      if (arg == "blackscholes")
      {
        benchmarks |= vtkm::benchmarking::BLACK_SCHOLES;
      }
      else if (arg == "math")
      {
        benchmarks |= vtkm::benchmarking::MATH;
      }
      else if (arg == "fusedmath")
      {
        benchmarks |= vtkm::benchmarking::FUSED_MATH;
      }
      else if (arg == "interpolate")
      {
        benchmarks |= vtkm::benchmarking::INTERPOLATE_FIELD;
      }
      else if (arg == "implicit_function")
      {
        benchmarks |= vtkm::benchmarking::IMPLICIT_FUNCTION;
      }
      else
      {
        std::cerr << "Unrecognized benchmark: " << argv[i] << std::endl;
        std::cerr << "Usage: " << argv[0] << " [options] [benchmarks]" << std::endl;
        std::cerr << "options are:" << std::endl;
        std::cerr << config.Usage;
        std::cerr << "available benchmarks are:" << std::endl;
        std::cerr << "  blackscholes, math, fusedmath, interpolate, implicit_function" << std::endl;
        std::cerr << "If no benchmarks are specified, all are run." << std::endl;
        return 1;
      }
    }
  }

  //now actually execute the benchmarks

  return vtkm::benchmarking::BenchmarkFieldAlgorithms::Run(benchmarks, config.Device);
}
