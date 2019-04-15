//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_benchmarking_Benchmarker_h
#define vtk_m_benchmarking_Benchmarker_h

#include <vtkm/ListTag.h>
#include <vtkm/Math.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <iostream>
#include <vector>

/*
 * Writing a Benchmark
 * -------------------
 * To write a benchmark you must provide a functor that will run the operations
 * you want to time and return the run time of those operations using the timer
 * for the device. The benchmark should also be templated on the value type being
 * operated on. Then use VTKM_MAKE_BENCHMARK to generate a maker functor and
 * VTKM_RUN_BENCHMARK to run the benchmark on a list of types.
 *
 * For Example:
 *
 * template<typename Value>
 * struct BenchSilly {
 *   // Setup anything that doesn't need to change per run in the constructor
 *   VTKM_CONT BenchSilly(){}
 *
 *   // The overloaded call operator will run the operations being timed and
 *   // return the execution time
 *   VTKM_CONT
 *   vtkm::Float64 operator()(){
 *     return 0.05;
 *   }
 *
 *   // The benchmark must also provide a method describing itself, this is
 *   // used when printing out run time statistics
 *   VTKM_CONT
 *   std::string Description() const {
 *     return "A silly benchmark";
 *   }
 * };
 *
 * // Now use the VTKM_MAKE_BENCHMARK macro to generate a maker functor for
 * // your benchmark. This lets us generate the benchmark functor for each type
 * // we want to test
 * VTKM_MAKE_BENCHMARK(Silly, BenchSilly);
 *
 * // You can also optionally pass arguments to the constructor like so:
 * // VTKM_MAKE_BENCHMARK(Blah, BenchBlah, 1, 2, 3);
 * // Note that benchmark names (the first argument) must be unique so different
 * // parameters to the constructor should have different names
 *
 * // We can now run our benchmark using VTKM_RUN_BENCHMARK, passing the
 * // benchmark name and type list to run on
 * int main(int, char**){
 *   VTKM_RUN_BENCHMARK(Silly, vtkm::ListTagBase<vtkm::Float32>());
 *   return 0;
 * }
 *
 * Check out vtkm/benchmarking/BenchmarkDeviceAdapter.h for some example usage
 */

/*
 * Use the VTKM_MAKE_BENCHMARK macro to define a maker functor for your benchmark.
 * This is used to allow you to template the benchmark functor on the type being benchmarked
 * and the device adapter so you can write init code in the constructor. Then the maker will
 * return a constructed instance of your benchmark for the type being benchmarked.
 * The VA_ARGS are used to pass any extra arguments needed by your benchmark
 */
#define VTKM_MAKE_BENCHMARK(Name, Bench, ...)                                                      \
  struct MakeBench##Name                                                                           \
  {                                                                                                \
    template <typename Value, typename DeviceAdapter>                                              \
    VTKM_CONT Bench<Value, DeviceAdapter> operator()(const Value vtkmNotUsed(v),                   \
                                                     DeviceAdapter vtkmNotUsed(id)) const          \
    {                                                                                              \
      return Bench<Value, DeviceAdapter>(__VA_ARGS__);                                             \
    }                                                                                              \
  }

/*
 * Use the VTKM_RUN_BENCHMARK macro to run your benchmark on the type list passed.
 * You must have previously defined a maker functor with VTKM_MAKE_BENCHMARK that this
 * macro will look for and use
 */
#define VTKM_RUN_BENCHMARK(Name, Types, Id)                                                        \
  vtkm::benchmarking::BenchmarkTypes(MakeBench##Name(), (Types), (Id))

namespace vtkm
{
namespace benchmarking
{
namespace stats
{
// Checks that the sequence is sorted, returns true if it's sorted, false
// otherwise
template <typename ForwardIt>
bool is_sorted(ForwardIt first, ForwardIt last)
{
  ForwardIt next = first;
  ++next;
  for (; next != last; ++next, ++first)
  {
    if (*first > *next)
    {
      return false;
    }
  }
  return true;
}

// Get the value representing the `percent` percentile of the
// sorted samples using linear interpolation
vtkm::Float64 PercentileValue(const std::vector<vtkm::Float64>& samples,
                              const vtkm::Float64 percent)
{
  VTKM_ASSERT(!samples.empty());
  if (samples.size() == 1)
  {
    return samples.front();
  }
  VTKM_ASSERT(percent >= 0.0);
  VTKM_ASSERT(percent <= 100.0);
  VTKM_ASSERT(vtkm::benchmarking::stats::is_sorted(samples.begin(), samples.end()));
  if (percent == 100.0)
  {
    return samples.back();
  }
  // Find the two nearest percentile values and linearly
  // interpolate between them
  const vtkm::Float64 rank = percent / 100.0 * (static_cast<vtkm::Float64>(samples.size()) - 1.0);
  const vtkm::Float64 low_rank = vtkm::Floor(rank);
  const vtkm::Float64 dist = rank - low_rank;
  const size_t k = static_cast<size_t>(low_rank);
  const vtkm::Float64 low = samples[k];
  const vtkm::Float64 high = samples[k + 1];
  return low + (high - low) * dist;
}
// Winsorize the samples to clean up any very extreme outliers
// Will replace all samples below `percent` and above 100 - `percent` percentiles
// with the value at the percentile
// NOTE: Assumes the samples have been sorted, as we make use of PercentileValue
void Winsorize(std::vector<vtkm::Float64>& samples, const vtkm::Float64 percent)
{
  const vtkm::Float64 low_percentile = PercentileValue(samples, percent);
  const vtkm::Float64 high_percentile = PercentileValue(samples, 100.0 - percent);
  for (std::vector<vtkm::Float64>::iterator it = samples.begin(); it != samples.end(); ++it)
  {
    if (*it < low_percentile)
    {
      *it = low_percentile;
    }
    else if (*it > high_percentile)
    {
      *it = high_percentile;
    }
  }
}
// Compute the mean value of the dataset
vtkm::Float64 Mean(const std::vector<vtkm::Float64>& samples)
{
  vtkm::Float64 mean = 0;
  for (std::vector<vtkm::Float64>::const_iterator it = samples.begin(); it != samples.end(); ++it)
  {
    mean += *it;
  }
  return mean / static_cast<vtkm::Float64>(samples.size());
}
// Compute the sample variance of the samples
vtkm::Float64 Variance(const std::vector<vtkm::Float64>& samples)
{
  vtkm::Float64 mean = Mean(samples);
  vtkm::Float64 square_deviations = 0;
  for (std::vector<vtkm::Float64>::const_iterator it = samples.begin(); it != samples.end(); ++it)
  {
    square_deviations += vtkm::Pow(*it - mean, 2.0);
  }
  return square_deviations / (static_cast<vtkm::Float64>(samples.size()) - 1.0);
}
// Compute the standard deviation of the samples
vtkm::Float64 StandardDeviation(const std::vector<vtkm::Float64>& samples)
{
  return vtkm::Sqrt(Variance(samples));
}
// Compute the median absolute deviation of the dataset
vtkm::Float64 MedianAbsDeviation(const std::vector<vtkm::Float64>& samples)
{
  std::vector<vtkm::Float64> abs_deviations;
  abs_deviations.reserve(samples.size());
  const vtkm::Float64 median = PercentileValue(samples, 50.0);
  for (std::vector<vtkm::Float64>::const_iterator it = samples.begin(); it != samples.end(); ++it)
  {
    abs_deviations.push_back(vtkm::Abs(*it - median));
  }
  std::sort(abs_deviations.begin(), abs_deviations.end());
  return PercentileValue(abs_deviations, 50.0);
}
} // stats

/*
 * The benchmarker takes a functor to benchmark and runs it multiple times,
 * printing out statistics of the run time at the end.
 * The functor passed should return the run time of the thing being benchmarked
 * in seconds, this lets us avoid including any per-run setup time in the benchmark.
 * However any one-time setup should be done in the functor's constructor
 */
struct Benchmarker
{
  std::vector<vtkm::Float64> Samples;
  std::string BenchmarkName;

  const vtkm::Float64 MaxRuntime;
  const size_t MaxIterations;

public:
  VTKM_CONT
  Benchmarker(vtkm::Float64 maxRuntime = 30, std::size_t maxIterations = 100)
    : MaxRuntime(maxRuntime)
    , MaxIterations(maxIterations)
  {
  }

  template <typename Functor>
  VTKM_CONT void GatherSamples(Functor func)
  {
    this->Samples.clear();
    this->BenchmarkName = func.Description();

    // Do a warm-up run. If the benchmark allocates any additional memory
    // eg. storage for output results, this will let it do that and
    // allow us to avoid measuring the allocation time in the actual benchmark run
    func();

    this->Samples.reserve(this->MaxIterations);

    // Run each benchmark for MAX_RUNTIME seconds or MAX_ITERATIONS iterations, whichever
    // takes less time. This kind of assumes that running for 500 iterations or 30s will give
    // good statistics, but if median abs dev and/or std dev are too high both these limits
    // could be increased
    size_t iter = 0;
    for (vtkm::Float64 elapsed = 0.0; elapsed < this->MaxRuntime && iter < this->MaxIterations;
         elapsed += this->Samples.back(), ++iter)
    {
      this->Samples.push_back(func());
    }

    std::sort(this->Samples.begin(), this->Samples.end());
    stats::Winsorize(this->Samples, 5.0);
  }

  VTKM_CONT void PrintSummary(std::ostream& out = std::cout)
  {
    out << "Benchmark \'" << this->BenchmarkName << "\' results:\n";

    if (this->Samples.empty())
    {
      out << "\tNo samples gathered!\n";
      return;
    }

    out << "\tnumSamples = " << this->Samples.size() << "\n"
        << "\tmedian = " << stats::PercentileValue(this->Samples, 50.0) << "s\n"
        << "\tmedian abs dev = " << stats::MedianAbsDeviation(this->Samples) << "s\n"
        << "\tmean = " << stats::Mean(this->Samples) << "s\n"
        << "\tstd dev = " << stats::StandardDeviation(this->Samples) << "s\n"
        << "\tmin = " << this->Samples.front() << "s\n"
        << "\tmax = " << this->Samples.back() << "s\n";
  }

  template <typename DeviceAdapter, typename MakerFunctor, typename T>
  VTKM_CONT bool operator()(DeviceAdapter id, MakerFunctor&& makerFunctor, T t)
  {
    auto func = makerFunctor(t, id);
    this->GatherSamples(func);
    this->PrintSummary();
    return true;
  }

  VTKM_CONT const std::vector<vtkm::Float64>& GetSamples() const { return this->Samples; }

  VTKM_CONT void Reset()
  {
    this->Samples.clear();
    this->BenchmarkName.clear();
  }
};

template <typename MakerFunctor>
class InternalPrintTypeAndBench
{
  MakerFunctor Maker;

public:
  VTKM_CONT
  InternalPrintTypeAndBench(MakerFunctor maker)
    : Maker(maker)
  {
  }

  template <typename T>
  VTKM_CONT void operator()(T t, vtkm::cont::DeviceAdapterId id) const
  {
    std::cout << "*** " << vtkm::testing::TypeName<T>::Name() << " on device " << id.GetName()
              << " ***************" << std::endl;
    Benchmarker bench;
    try
    {
      vtkm::cont::TryExecuteOnDevice(id, bench, Maker, t);
    }
    catch (std::exception& e)
    {
      std::cout << "\n"
                << "An exception occurring during a benchmark:\n\t" << e.what() << "\n"
                << "Attempting to continue with remaining benchmarks...\n\n";
    }
  }
};

template <class MakerFunctor, class TypeList>
VTKM_CONT void BenchmarkTypes(MakerFunctor&& maker, TypeList, vtkm::cont::DeviceAdapterId id)
{
  vtkm::ListForEach(
    InternalPrintTypeAndBench<MakerFunctor>(std::forward<MakerFunctor>(maker)), TypeList(), id);
}
}
}

#endif
