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

#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/internal/brigand.hpp>

#include <benchmark/benchmark.h>

#include <ostream>

/// \file Benchmarker.h
/// \brief Benchmarking utilities
///
/// VTK-m's benchmarking framework is built on top of Google Benchmark.
///
/// A benchmark is now a single function, which is passed to a macro:
///
/// ```
/// void MyBenchmark(::benchmark::State& state)
/// {
///   MyClass someClass;
///
///   // Optional: Add a descriptive label with additional benchmark details:
///   state.SetLabel("Blah blah blah.");
///
///   // Must use a vtkm timer to properly capture eg. CUDA execution times.
///   vtkm::cont::Timer timer;
///   for (auto _ : state)
///   {
///     someClass.Reset();
///
///     timer.Start();
///     someClass.DoWork();
///     timer.Stop();
///
///     state.SetIterationTime(timer.GetElapsedTime());
///   }
///
///   // Optional: Report items and/or bytes processed per iteration in output:
///   state.SetItemsProcessed(state.iterations() * someClass.GetNumberOfItems());
///   state.SetBytesProcessed(state.iterations() * someClass.GetNumberOfBytes());
/// }
/// }
/// VTKM_BENCHMARK(MyBenchmark);
/// ```
///
/// Google benchmark also makes it easy to implement parameter sweep benchmarks:
///
/// ```
/// void MyParameterSweep(::benchmark::State& state)
/// {
///   // The current value in the sweep:
///   const vtkm::Id currentValue = state.range(0);
///
///   MyClass someClass;
///   someClass.SetSomeParameter(currentValue);
///
///   vtkm::cont::Timer timer;
///   for (auto _ : state)
///   {
///     someClass.Reset();
///
///     timer.Start();
///     someClass.DoWork();
///     timer.Stop();
///
///     state.SetIterationTime(timer.GetElapsedTime());
///   }
/// }
/// VTKM_BENCHMARK_OPTS(MyBenchmark, ->ArgName("Param")->Range(32, 1024 * 1024));
/// ```
///
/// will generate and launch several benchmarks, exploring the parameter space of
/// `SetSomeParameter` between the values of 32 and (1024*1024). The chain of
///   functions calls in the second argument is applied to an instance of
/// ::benchmark::internal::Benchmark. See Google Benchmark's documentation for
/// more details.
///
/// For more complex benchmark configurations, the VTKM_BENCHMARK_APPLY macro
///   accepts a function with the signature
/// `void Func(::benchmark::internal::Benchmark*)` that may be used to generate
/// more complex configurations.
///
/// To instantiate a templated benchmark across a list of types, the
/// VTKM_BENCHMARK_TEMPLATE* macros take a vtkm::List of types as an additional
/// parameter. The templated benchmark function will be instantiated and called
/// for each type in the list:
///
/// ```
/// template <typename T>
/// void MyBenchmark(::benchmark::State& state)
/// {
///   MyClass<T> someClass;
///
///   // Must use a vtkm timer to properly capture eg. CUDA execution times.
///   vtkm::cont::Timer timer;
///   for (auto _ : state)
///   {
///     someClass.Reset();
///
///     timer.Start();
///     someClass.DoWork();
///     timer.Stop();
///
///     state.SetIterationTime(timer.GetElapsedTime());
///   }
/// }
/// }
/// VTKM_BENCHMARK_TEMPLATE(MyBenchmark, vtkm::List<vtkm::Float32, vtkm::Vec3f_32>);
/// ```
///
/// The benchmarks are executed by calling the `VTKM_EXECUTE_BENCHMARKS(argc, argv)`
/// macro from `main`. There is also a `VTKM_EXECUTE_BENCHMARKS_PREAMBLE(argc, argv, some_string)`
/// macro that appends the contents of `some_string` to the Google Benchmark preamble.
///
/// If a benchmark is not compatible with some configuration, it may call
/// `state.SkipWithError("Error message");` on the `::benchmark::State` object and return. This is
/// useful, for instance in the filter tests when the input is not compatible with the filter.
///
/// When launching a benchmark executable, the following options are supported by Google Benchmark:
///
/// - `--benchmark_list_tests`: List all available tests.
/// - `--benchmark_filter="[regex]"`: Only run benchmark with names that match `[regex]`.
/// - `--benchmark_filter="-[regex]"`: Only run benchmark with names that DON'T match `[regex]`.
/// - `--benchmark_min_time=[float]`: Make sure each benchmark repetition gathers `[float]` seconds
///   of data.
/// - `--benchmark_repetitions=[int]`: Run each benchmark `[int]` times and report aggregate statistics
///   (mean, stdev, etc). A "repetition" refers to a single execution of the benchmark function, not
///   an "iteration", which is a loop of the `for(auto _:state){...}` section.
/// - `--benchmark_report_aggregates_only="true|false"`: If true, only the aggregate statistics are
///   reported (affects both console and file output). Requires `--benchmark_repetitions` to be useful.
/// - `--benchmark_display_aggregates_only="true|false"`: If true, only the aggregate statistics are
///   printed to the terminal. Any file output will still contain all repetition info.
/// - `--benchmark_format="console|json|csv"`: Specify terminal output format: human readable
///   (`console`) or `csv`/`json` formats.
/// - `--benchmark_out_format="console|json|csv"`: Specify file output format: human readable
///   (`console`) or `csv`/`json` formats.
/// - `--benchmark_out=[filename]`: Specify output file.
/// - `--benchmark_color="true|false"`: Toggle color output in terminal when using `console` output.
/// - `--benchmark_counters_tabular="true|false"`: Print counter information (e.g. bytes/sec, items/sec)
///   in the table, rather than appending them as a label.
///
/// For more information and examples of practical usage, take a look at the existing benchmarks in
/// vtk-m/benchmarking/.

/// \def VTKM_EXECUTE_BENCHMARKS(argc, argv)
///
/// Run the benchmarks defined in the current file. Benchmarks may be filtered
/// and modified using the passed arguments; see the Google Benchmark documentation
/// for more details.
#define VTKM_EXECUTE_BENCHMARKS(argc, argv) vtkm::bench::detail::ExecuteBenchmarks(argc, argv)

/// \def VTKM_EXECUTE_BENCHMARKS_PREAMBLE(argc, argv, preamble)
///
/// Run the benchmarks defined in the current file. Benchmarks may be filtered
/// and modified using the passed arguments; see the Google Benchmark documentation
/// for more details. The `preamble` string may be used to supply additional
/// information that will be appended to the output's preamble.
#define VTKM_EXECUTE_BENCHMARKS_PREAMBLE(argc, argv, preamble)                                     \
  vtkm::bench::detail::ExecuteBenchmarks(argc, argv, preamble)

/// \def VTKM_BENCHMARK(BenchFunc)
///
/// Define a simple benchmark. A single benchmark will be generated that executes
/// `BenchFunc`. `BenchFunc` must have the signature:
///
/// ```
/// void BenchFunc(::benchmark::State& state)
/// ```
#define VTKM_BENCHMARK(BenchFunc) BENCHMARK(BenchFunc)->UseManualTime()

/// \def VTKM_BENCHMARK_OPTS(BenchFunc, Args)
///
/// Similar to `VTKM_BENCHMARK`, but allows additional options to be specified
/// on the `::benchmark::internal::Benchmark` object. Example usage:
///
/// ```
/// VTKM_BENCHMARK_OPTS(MyBenchmark, ->ArgName("MyParam")->Range(32, 1024*1024));
/// ```
///
/// Note the similarity to the raw Google Benchmark usage of
/// `BENCHMARK(MyBenchmark)->ArgName("MyParam")->Range(32, 1024*1024);`. See
/// the Google Benchmark documentation for more details on the available options.
#define VTKM_BENCHMARK_OPTS(BenchFunc, options) BENCHMARK(BenchFunc)->UseManualTime() options

/// \def VTKM_BENCHMARK_APPLY(BenchFunc, ConfigFunc)
///
/// Similar to `VTKM_BENCHMARK`, but allows advanced benchmark configuration
/// via a supplied ConfigFunc, similar to Google Benchmark's
/// `BENCHMARK(BenchFunc)->Apply(ConfigFunc)`. `ConfigFunc` must have the
/// signature:
///
/// ```
/// void ConfigFunc(::benchmark::internal::Benchmark*);
/// ```
///
/// See the Google Benchmark documentation for more details on the available options.
#define VTKM_BENCHMARK_APPLY(BenchFunc, applyFunctor)                                              \
  BENCHMARK(BenchFunc)->Apply(applyFunctor)->UseManualTime()

/// \def VTKM_BENCHMARK_TEMPLATES(BenchFunc, TypeList)
///
/// Define a family of benchmark that vary by template argument. A single
/// benchmark will be generated for each type in `TypeList` (a vtkm::List of
/// types) that executes `BenchFunc<T>`. `BenchFunc` must have the signature:
///
/// ```
/// template <typename T>
/// void BenchFunc(::benchmark::State& state)
/// ```
#define VTKM_BENCHMARK_TEMPLATES(BenchFunc, TypeList)                                              \
  VTKM_BENCHMARK_TEMPLATES_APPLY(BenchFunc, vtkm::bench::detail::NullApply, TypeList)

/// \def VTKM_BENCHMARK_TEMPLATES_OPTS(BenchFunc, Args, TypeList)
///
/// Similar to `VTKM_BENCHMARK_TEMPLATES`, but allows additional options to be specified
/// on the `::benchmark::internal::Benchmark` object. Example usage:
///
/// ```
/// VTKM_BENCHMARK_TEMPLATES_OPTS(MyBenchmark,
///                                ->ArgName("MyParam")->Range(32, 1024*1024),
///                              vtkm::List<vtkm::Float32, vtkm::Vec3f_32>);
/// ```
#define VTKM_BENCHMARK_TEMPLATES_OPTS(BenchFunc, options, TypeList)                                \
  VTKM_BENCHMARK_TEMPLATES_APPLY(                                                                  \
    BenchFunc, [](::benchmark::internal::Benchmark* bm) { bm options; }, TypeList)

/// \def VTKM_BENCHMARK_TEMPLATES_APPLY(BenchFunc, ConfigFunc, TypeList)
///
/// Similar to `VTKM_BENCHMARK_TEMPLATES`, but allows advanced benchmark configuration
/// via a supplied ConfigFunc, similar to Google Benchmark's
/// `BENCHMARK(BenchFunc)->Apply(ConfigFunc)`. `ConfigFunc` must have the
/// signature:
///
/// ```
/// void ConfigFunc(::benchmark::internal::Benchmark*);
/// ```
///
/// See the Google Benchmark documentation for more details on the available options.
#define VTKM_BENCHMARK_TEMPLATES_APPLY(BenchFunc, ApplyFunctor, TypeList)                                                                                                             \
  namespace                                                                                                                                                                           \
  { /* A template function cannot be used as a template parameter, so wrap the function with       \
     * a template struct to get it into the GenerateTemplateBenchmarks class. */ \
  template <typename... Ts>                                                                                                                                                           \
  struct VTKM_BENCHMARK_WRAPPER_NAME(BenchFunc)                                                                                                                                       \
  {                                                                                                                                                                                   \
    static ::benchmark::internal::Function* GetFunction() { return BenchFunc<Ts...>; }                                                                                                \
  };                                                                                                                                                                                  \
  } /* end anon namespace */                                                                                                                                                          \
  int BENCHMARK_PRIVATE_NAME(BenchFunc) = vtkm::bench::detail::GenerateTemplateBenchmarks<                                                                                            \
    brigand::bind<VTKM_BENCHMARK_WRAPPER_NAME(BenchFunc)>,                                                                                                                            \
    TypeList>::Register(#BenchFunc, ApplyFunctor)

// Internal use only:
#define VTKM_BENCHMARK_WRAPPER_NAME(BenchFunc)                                                     \
  BENCHMARK_PRIVATE_CONCAT(_wrapper_, BenchFunc, __LINE__)

namespace vtkm
{
namespace bench
{
namespace detail
{

static inline void NullApply(::benchmark::internal::Benchmark*)
{
}

/// Do not use directly. The VTKM_BENCHMARK_TEMPLATES macros should be used
/// instead.
// TypeLists could be expanded to compute cross products if we ever have that
// need.
template <typename BoundBench, typename TypeLists>
struct GenerateTemplateBenchmarks;

template <template <typename...> class BenchType, typename TypeList>
struct GenerateTemplateBenchmarks<brigand::bind<BenchType>, TypeList>
{
private:
  template <typename T>
  using MakeBenchType = BenchType<T>;

  using Benchmarks = brigand::transform<TypeList, brigand::bind<MakeBenchType, brigand::_1>>;

  template <typename ApplyFunctor>
  struct RegisterImpl
  {
    std::string BenchName;
    ApplyFunctor Apply;

    template <typename P>
    void operator()(brigand::type_<BenchType<P>>) const
    {
      std::ostringstream name;
      name << this->BenchName << "<" << vtkm::testing::TypeName<P>::Name() << ">";
      auto bm = ::benchmark::internal::RegisterBenchmarkInternal(
        new ::benchmark::internal::FunctionBenchmark(name.str().c_str(),
                                                     BenchType<P>::GetFunction()));
      this->Apply(bm);

      // Always use manual time with vtkm::cont::Timer to capture CUDA times accurately.
      bm->UseManualTime();
    }
  };

public:
  template <typename ApplyFunctor>
  static int Register(const std::string& benchName, ApplyFunctor&& apply)
  {
    brigand::for_each<Benchmarks>(
      RegisterImpl<ApplyFunctor>{ benchName, std::forward<ApplyFunctor>(apply) });
    return 0;
  }
};

class VTKmConsoleReporter : public ::benchmark::ConsoleReporter
{
  std::string UserPreamble;

public:
  VTKmConsoleReporter() = default;

  explicit VTKmConsoleReporter(const std::string& preamble)
    : UserPreamble{ preamble }
  {
  }

  bool ReportContext(const Context& context) override
  {
    if (!::benchmark::ConsoleReporter::ReportContext(context))
    {
      return false;
    }

    // The rest of the preamble is printed to the error stream, so be consistent:
    auto& out = this->GetErrorStream();

    // Print list of devices:
    out << "VTK-m Device State:\n";
    vtkm::cont::GetRuntimeDeviceTracker().PrintSummary(out);
    if (!this->UserPreamble.empty())
    {
      out << this->UserPreamble << "\n";
    }
    out.flush();

    return true;
  }
};

// Returns the number of executed benchmarks:
static inline vtkm::Id ExecuteBenchmarks(int& argc,
                                         char* argv[],
                                         const std::string& preamble = std::string{})
{
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
  {
    return 1;
  }

  VTKmConsoleReporter reporter{ preamble };

  vtkm::cont::Timer timer;
  timer.Start();
  std::size_t num = ::benchmark::RunSpecifiedBenchmarks(&reporter);
  timer.Stop();

  reporter.GetOutputStream().flush();
  reporter.GetErrorStream().flush();

  reporter.GetErrorStream() << "Ran " << num << " benchmarks in " << timer.GetElapsedTime()
                            << " seconds." << std::endl;

  return static_cast<vtkm::Id>(num);
}
}
}
} // end namespace vtkm::bench::detail

#endif
