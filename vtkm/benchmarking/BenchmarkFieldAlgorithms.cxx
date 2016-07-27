//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================


#include <vtkm/Math.h>
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/benchmarking/Benchmarker.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/random.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <string>

namespace vtkm {
namespace benchmarking {

#define ARRAY_SIZE (1 << 22)
#define CUBE_SIZE 256
const static std::string DIVIDER(40, '-');

enum BenchmarkName {
  BLACK_SCHOLES = 1,
  MATH = 1 << 1,
  FUSED_MATH = 1 << 3,
  INTERPOLATE_FIELD = 1 << 4,
  ALL = BLACK_SCHOLES |
        MATH |
        FUSED_MATH |
        INTERPOLATE_FIELD
};

template<typename T>
class BlackScholes : public vtkm::worklet::WorkletMapField
{
  T Riskfree;
  T Volatility;
public:
  typedef void ControlSignature(FieldIn<Scalar>, FieldIn<Scalar>,
                                FieldIn<Scalar>, FieldOut<Scalar>,
                                FieldOut<Scalar>);
  typedef void ExecutionSignature(_1,_2,_3,_4,_5);

  BlackScholes(T risk, T volatility):
    Riskfree(risk),
    Volatility(volatility)
  {
  }

  VTKM_EXEC_EXPORT
  T CumulativeNormalDistribution(T d) const
  {
    const vtkm::Float32       A1 = 0.31938153f;
    const vtkm::Float32       A2 = -0.356563782f;
    const vtkm::Float32       A3 = 1.781477937f;
    const vtkm::Float32       A4 = -1.821255978f;
    const vtkm::Float32       A5 = 1.330274429f;
    const vtkm::Float32       RSQRT2PI = 0.39894228040143267793994605993438f;

    const T K = T(1.0f) / ( T(1.0f) + T(0.2316419f) * vtkm::Abs(d));

    T cnd = RSQRT2PI * vtkm::Exp(-0.5f * d * d) *
    (K * (A1 + K * (A2 + K * (A3 + K * (A4 + K * A5)))));

    if(d > 0)
      {
      cnd = 1.0f - cnd;
      }

    return cnd;
  }


  VTKM_EXEC_EXPORT
  void operator()(const T& stockPrice, const T& optionStrike, const T& optionYears,
                  T& callResult, T& putResult) const
  {
  // Black-Scholes formula for both call and put
  const T sqrtYears = vtkm::Sqrt(optionYears);
  const T volMultSqY = this->Volatility * sqrtYears;

  const T    d1 = ( vtkm::Log(stockPrice / optionStrike) + (this->Riskfree + 0.5f * Volatility * Volatility) * optionYears) / (volMultSqY);
  const T    d2 = d1 - volMultSqY;
  const T CNDD1 = CumulativeNormalDistribution(d1);
  const T CNDD2 = CumulativeNormalDistribution(d2);

  //Calculate Call and Put simultaneously
  T expRT = vtkm::Exp( - this->Riskfree * optionYears);
  callResult = stockPrice * CNDD1 - optionStrike * expRT * CNDD2;
  putResult = optionStrike * expRT * (1.0f - CNDD2) - stockPrice * (1.0f - CNDD1);
  }

};

class Mag : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<Vec3>, FieldOut<Scalar>);
  typedef void ExecutionSignature(_1,_2);

  template<typename T>
  VTKM_EXEC_EXPORT
  void operator()(const vtkm::Vec<T,3>& vec, T& result) const
  {
    result = vtkm::Magnitude(vec);
  }
};

class Square : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<Scalar>, FieldOut<Scalar>);
  typedef void ExecutionSignature(_1,_2);

  template<typename T>
  VTKM_EXEC_EXPORT
  void operator()(T input, T& output) const
  {
    output = input * input;
  }
};

class Sin : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<Scalar>, FieldOut<Scalar>);
  typedef void ExecutionSignature(_1,_2);

  template<typename T>
  VTKM_EXEC_EXPORT
  void operator()(T input, T& output) const
  {
    output = vtkm::Sin(input);
  }
};

class Cos : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<Scalar>, FieldOut<Scalar>);
  typedef void ExecutionSignature(_1,_2);

  template<typename T>
  VTKM_EXEC_EXPORT
  void operator()(T input, T& output) const
  {
    output = vtkm::Cos(input);
  }
};

class FusedMath : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<Vec3>, FieldOut<Scalar>);
  typedef void ExecutionSignature(_1,_2);

  template<typename T>
  VTKM_EXEC_EXPORT
  void operator()(const vtkm::Vec<T,3>& vec, T& result) const
  {
    const T m = vtkm::Magnitude(vec);
    result = vtkm::Cos( vtkm::Sin(m) * vtkm::Sin(m) );
  }
};

class GenerateEdges : public vtkm::worklet::WorkletMapPointToCell
{
public:
  typedef void ControlSignature( CellSetIn cellset, WholeArrayOut< > edgeIds);
  typedef void ExecutionSignature(PointIndices, ThreadIndices, _2);
  typedef _1 InputDomain;

  template<typename ConnectivityInVec,
           typename ThreadIndicesType,
           typename IdPairTableType>
  VTKM_EXEC_EXPORT
  void operator()(const ConnectivityInVec &connectivity,
                  const ThreadIndicesType threadIndices,
                  const IdPairTableType &edgeIds) const
  {
  const vtkm::Id writeOffset = (threadIndices.GetInputIndex() * 12);

  const vtkm::IdComponent edgeTable[24] = { 0, 1, 1, 2, 3, 2, 0, 3,
                                            4, 5, 5, 6, 7, 6, 4, 7,
                                            0, 4, 1, 5, 2, 6, 3, 7 };

  for(vtkm::Id i=0; i < 12; ++i)
    {
    const vtkm::Id offset = (i*2);
    const vtkm::Id2 edge( connectivity[ edgeTable[offset] ],
                          connectivity[ edgeTable[offset+1] ] );
    edgeIds.Set(writeOffset+i, edge);
    }
  }
};

class InterpolateField : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn< Id2Type > interpolation_ids,
                                FieldIn< Scalar > interpolation_weights,
                                WholeArrayIn<> inputField,
                                FieldOut<> output
                                );
  typedef void ExecutionSignature(_1, _2, _3, _4);
  typedef _1 InputDomain;

  template <typename WeightType, typename InFieldPortalType, typename OutFieldType>
  VTKM_EXEC_EXPORT
  void operator()(const vtkm::Id2& low_high,
                  const WeightType &weight,
                  const InFieldPortalType& inPortal,
                  OutFieldType &result) const
  {
    //fetch the low / high values from inPortal
    result = vtkm::Lerp(inPortal.Get(low_high[0]),
                        inPortal.Get(low_high[1]),
                        weight);
  }
};


/// This class runs a series of micro-benchmarks to measure
/// performance of different field operations
template<class DeviceAdapterTag>
class BenchmarkFieldAlgorithms {
  typedef vtkm::cont::StorageTagBasic StorageTag;

  typedef vtkm::cont::ArrayHandle<vtkm::Id, StorageTag> IdArrayHandle;

  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag> Algorithm;

  typedef vtkm::cont::Timer<DeviceAdapterTag> Timer;

private:
  template<typename Value>
  struct BenchBlackScholes {
    typedef vtkm::cont::ArrayHandle<Value, StorageTag> ValueArrayHandle;

    ValueArrayHandle StockPrice;
    ValueArrayHandle OptionStrike;
    ValueArrayHandle OptionYears;

    std::vector<Value> price;
    std::vector<Value> strike;
    std::vector<Value> years;

    VTKM_CONT_EXPORT
    BenchBlackScholes()
    {
      typedef boost::uniform_real<Value> ValueRange;
      typedef boost::mt19937 MTGenerator;
      typedef boost::variate_generator<MTGenerator&, ValueRange> Generator;

      boost::mt19937 rng;

      boost::uniform_real<Value> price_range(Value(5.0f),Value(30.0f));
      boost::uniform_real<Value> strike_range(Value(1.0f),Value(100.0f));
      boost::uniform_real<Value> year_range(Value(0.25f),Value(10.0f));

      Generator priceGenerator(rng, price_range);
      Generator strikeGenerator(rng, strike_range);
      Generator yearGenerator(rng, year_range);

      this->price.resize(ARRAY_SIZE);
      this->strike.resize(ARRAY_SIZE);
      this->years.resize(ARRAY_SIZE);
      for(std::size_t i=0; i < ARRAY_SIZE; ++i )
      {
        this->price[i] = priceGenerator();
        this->strike[i] = strikeGenerator();
        this->years[i] = yearGenerator();
      }

      this->StockPrice = vtkm::cont::make_ArrayHandle(this->price);
      this->OptionStrike = vtkm::cont::make_ArrayHandle(this->strike);
      this->OptionYears = vtkm::cont::make_ArrayHandle(this->years);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()()
    {
      vtkm::cont::ArrayHandle<Value> callResultHandle, putResultHandle;
      const Value RISKFREE = 0.02f;
      const Value VOLATILITY = 0.30f;

      Timer timer;
      BlackScholes<Value> worklet(RISKFREE, VOLATILITY);
      vtkm::worklet::DispatcherMapField< BlackScholes<Value> > dispatcher(worklet);

      dispatcher.Invoke(  this->StockPrice,
                          this->OptionStrike,
                          this->OptionYears,
                          callResultHandle,
                          putResultHandle
                        );

      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "BlackScholes " << ARRAY_SIZE << " stocks";
      return description.str();
    }
  };

  VTKM_MAKE_BENCHMARK(BlackScholes, BenchBlackScholes);

  template<typename Value>
  struct BenchMath {
    std::vector< vtkm::Vec<Value, 3> > input;
    vtkm::cont::ArrayHandle< vtkm::Vec<Value, 3>, StorageTag> InputHandle;

    VTKM_CONT_EXPORT
    BenchMath()
    {
      typedef boost::uniform_real<Value> ValueRange;
      typedef boost::mt19937 MTGenerator;
      typedef boost::variate_generator<MTGenerator&, ValueRange> Generator;

      boost::mt19937 rng;
      boost::uniform_real<Value> range;
      Generator generator(rng, range);

      this->input.resize(ARRAY_SIZE);
      for(std::size_t i=0; i < ARRAY_SIZE; ++i )
      {
        this->input[i] = vtkm::Vec<Value, 3>(generator(),
                                             generator(),
                                             generator());
      }

      this->InputHandle = vtkm::cont::make_ArrayHandle(this->input);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()()
    {
      vtkm::cont::ArrayHandle<Value> tempHandle1;
      vtkm::cont::ArrayHandle<Value> tempHandle2;

      Timer timer;

      vtkm::worklet::DispatcherMapField< Mag >().Invoke(    InputHandle, tempHandle1 );
      vtkm::worklet::DispatcherMapField< Sin >().Invoke(    tempHandle1, tempHandle2 );
      vtkm::worklet::DispatcherMapField< Square >().Invoke( tempHandle2, tempHandle1 );
      vtkm::worklet::DispatcherMapField< Cos >().Invoke(    tempHandle1, tempHandle2 );

      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "Magnitude -> Sine -> Square -> Cosine " << ARRAY_SIZE << " values";
      return description.str();
    }
  };

  VTKM_MAKE_BENCHMARK(Math, BenchMath);

  template<typename Value>
  struct BenchFusedMath {
    std::vector< vtkm::Vec<Value, 3> > input;
    vtkm::cont::ArrayHandle< vtkm::Vec<Value, 3>, StorageTag> InputHandle;

    VTKM_CONT_EXPORT
    BenchFusedMath()
    {
      typedef boost::uniform_real<Value> ValueRange;
      typedef boost::mt19937 MTGenerator;
      typedef boost::variate_generator<MTGenerator&, ValueRange> Generator;

      boost::mt19937 rng;
      boost::uniform_real<Value> range;
      Generator generator(rng, range);

      this->input.resize(ARRAY_SIZE);
      for(std::size_t i=0; i < ARRAY_SIZE; ++i )
      {
        this->input[i] = vtkm::Vec<Value, 3>(generator(),
                                             generator(),
                                             generator());
      }

      this->InputHandle = vtkm::cont::make_ArrayHandle(this->input);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()()
    {
      vtkm::cont::ArrayHandle<Value> result;

      Timer timer;
      vtkm::worklet::DispatcherMapField< FusedMath >().Invoke( this->InputHandle, result );
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      description << "Fused Magnitude -> Sine -> Square -> Cosine " << ARRAY_SIZE << " values";
      return description.str();
    }
  };

  VTKM_MAKE_BENCHMARK(FusedMath, BenchFusedMath);

  template<typename Value>
  struct BenchEdgeInterp {
    std::vector<vtkm::Float32> weight;
    std::vector<Value> field;

    vtkm::cont::ArrayHandle< vtkm::Float32, StorageTag> WeightHandle;
    vtkm::cont::ArrayHandle< Value, StorageTag> FieldHandle;
    vtkm::cont::ArrayHandle< vtkm::Id2, StorageTag> EdgePairHandle;

    VTKM_CONT_EXPORT
    BenchEdgeInterp()
    {
      typedef boost::mt19937 MTGenerator;
      typedef boost::variate_generator<MTGenerator&, boost::uniform_real<Value> > Generator;
      typedef boost::variate_generator<MTGenerator&, boost::uniform_real<vtkm::Float32> > WGenerator;

      boost::mt19937 rng;
      boost::uniform_real<vtkm::Float32> weight_range(0.0f,1.0f);
      WGenerator wgenerator(rng, weight_range);

      boost::uniform_real<Value> field_range;
      Generator fgenerator(rng, field_range);

      //basically the core challenge is to generate an array whose
      //indexing pattern matches that of a edge based algorithm.
      //
      //So for this kind of problem we generate the 12 edges of each
      //cell and place them into array.
      //
      vtkm::cont::CellSetStructured<3> cellSet;
      cellSet.SetPointDimensions( vtkm::Id3(CUBE_SIZE,CUBE_SIZE,CUBE_SIZE) );

      const vtkm::Id numberOfEdges = cellSet.GetNumberOfCells() * 12;
      const std::size_t esize = static_cast<std::size_t>(numberOfEdges);
      const std::size_t psize = static_cast<std::size_t>(cellSet.GetNumberOfPoints());

      this->EdgePairHandle.Allocate( numberOfEdges );
      vtkm::worklet::DispatcherMapTopology< GenerateEdges >().Invoke( cellSet,
                                                                      this->EdgePairHandle );

      this->weight.resize( esize );
      for(std::size_t i=0; i < esize; ++i )
      {
        this->weight[i] = wgenerator();
      }

      this->field.resize( psize );
      for(std::size_t i=0; i < psize; ++i )
      {
        this->field[i] = fgenerator();
      }

      this->FieldHandle = vtkm::cont::make_ArrayHandle(this->field);
      this->WeightHandle = vtkm::cont::make_ArrayHandle(this->weight);
    }

    VTKM_CONT_EXPORT
    vtkm::Float64 operator()()
    {
      vtkm::cont::ArrayHandle<Value> result;

      Timer timer;
      vtkm::worklet::DispatcherMapField<
          InterpolateField,
          DeviceAdapterTag>().Invoke(this->EdgePairHandle,
                                     this->WeightHandle,
                                     this->FieldHandle,
                                     result);
      return timer.GetElapsedTime();
    }

    VTKM_CONT_EXPORT
    std::string Description() const {
      std::stringstream description;
      const std::size_t size = (CUBE_SIZE-1)*(CUBE_SIZE-1)*(CUBE_SIZE-1)*12;
      description << "Edge Interpolation of an array of " << size << " values";
      return description.str();
    }
  };

  VTKM_MAKE_BENCHMARK(EdgeInterp, BenchEdgeInterp);



public:

  struct ValueTypes : vtkm::ListTagBase<vtkm::Float32, vtkm::Float64>{};

  struct InterpValueTypes : vtkm::ListTagBase<vtkm::Float32,
                                              vtkm::Float64,
                                              vtkm::Vec< vtkm::Float32, 3>,
                                              vtkm::Vec< vtkm::Float64, 3>
                                              >{};


  static VTKM_CONT_EXPORT int Run(int benchmarks){
    std::cout << DIVIDER << "\nRunning Field Algorithm benchmarks\n";

    if (benchmarks & BLACK_SCHOLES) {
      std::cout << DIVIDER << "\nBenchmarking BlackScholes\n";
      VTKM_RUN_BENCHMARK(BlackScholes, ValueTypes());
    }

    if (benchmarks & MATH){
      std::cout << DIVIDER << "\nBenchmarking Multiple Math Worklets\n";
      VTKM_RUN_BENCHMARK(Math, ValueTypes());
    }

    if (benchmarks & FUSED_MATH){
      std::cout << DIVIDER << "\nBenchmarking Single Fused Math Worklet\n";
      VTKM_RUN_BENCHMARK(FusedMath, ValueTypes());
    }

    if (benchmarks & INTERPOLATE_FIELD){
      std::cout << DIVIDER << "\nBenchmarking Edge Based Field InterpolationWorklet\n";
      VTKM_RUN_BENCHMARK(EdgeInterp, ValueTypes());
    }

    return 0;
  }
};


#undef ARRAY_SIZE

}
} // namespace vtkm::benchmarking

int main(int argc, char *argv[])
{
  int benchmarks = 0;
  if (argc < 2){
    benchmarks = vtkm::benchmarking::ALL;
  }
  else {
    for (int i = 1; i < argc; ++i){
      std::string arg = argv[i];
      std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);
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
      else
      {
        std::cout << "Unrecognized benchmark: " << argv[i] << std::endl;
        return 1;
      }
    }
  }

  //now actually execute the benchmarks
  return vtkm::benchmarking::BenchmarkFieldAlgorithms
    <VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Run(benchmarks);
}