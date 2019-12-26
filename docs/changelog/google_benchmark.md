# Updated Benchmark Framework

The benchmarking framework has been updated to use Google Benchmark.

A benchmark is now a single function, which is passed to a macro:

```
void MyBenchmark(::benchmark::State& state)
{
  MyClass someClass;

  // Optional: Add a descriptive label with additional benchmark details:
  state.SetLabel("Blah blah blah.");

  // Must use a vtkm timer to properly capture eg. CUDA execution times.
  vtkm::cont::Timer timer;
  for (auto _ : state)
  {
    someClass.Reset();

    timer.Start();
    someClass.DoWork();
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }

  // Optional: Report items and/or bytes processed per iteration in output:
  state.SetItemsProcessed(state.iterations() * someClass.GetNumberOfItems());
  state.SetBytesProcessed(state.iterations() * someClass.GetNumberOfBytes());
}
}
VTKM_BENCHMARK(MyBenchmark);
```

Google benchmark also makes it easy to implement parameter sweep benchmarks:

```
void MyParameterSweep(::benchmark::State& state)
{
  // The current value in the sweep:
  const vtkm::Id currentValue = state.range(0);

  MyClass someClass;
  someClass.SetSomeParameter(currentValue);

  vtkm::cont::Timer timer;
  for (auto _ : state)
  {
    someClass.Reset();

    timer.Start();
    someClass.DoWork();
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
VTKM_BENCHMARK_OPTS(MyBenchmark, ->ArgName("Param")->Range(32, 1024 * 1024));
```

will generate and launch several benchmarks, exploring the parameter space of
`SetSomeParameter` between the values of 32 and (1024*1024). The chain of
functions calls in the second argument is applied to an instance of
::benchmark::internal::Benchmark. See Google Benchmark's documentation for
more details.

For more complex benchmark configurations, the VTKM_BENCHMARK_APPLY macro
accepts a function with the signature
`void Func(::benchmark::internal::Benchmark*)` that may be used to generate
more complex configurations.

To instantiate a templated benchmark across a list of types, the
VTKM_BENCHMARK_TEMPLATE* macros take a vtkm::List of types as an additional
parameter. The templated benchmark function will be instantiated and called
for each type in the list:

```
template <typename T>
void MyBenchmark(::benchmark::State& state)
{
  MyClass<T> someClass;

  // Must use a vtkm timer to properly capture eg. CUDA execution times.
  vtkm::cont::Timer timer;
  for (auto _ : state)
  {
    someClass.Reset();

    timer.Start();
    someClass.DoWork();
    timer.Stop();

    state.SetIterationTime(timer.GetElapsedTime());
  }
}
}
VTKM_BENCHMARK_TEMPLATE(MyBenchmark, vtkm::List<vtkm::Float32, vtkm::Vec3f_32>);
```

The benchmarks are executed by calling the `VTKM_EXECUTE_BENCHMARKS(argc, argv)`
macro from `main`. There is also a `VTKM_EXECUTE_BENCHMARKS_PREAMBLE(argc, argv, some_string)`
macro that appends the contents of `some_string` to the Google Benchmark preamble.

If a benchmark is not compatible with some configuration, it may call 
`state.SkipWithError("Error message");` on the `::benchmark::State` object and return. This is
useful, for instance in the filter tests when the input is not compatible with the filter.

When launching a benchmark executable, the following options are supported by Google Benchmark:

- `--benchmark_list_tests`: List all available tests.
- `--benchmark_filter="[regex]"`: Only run benchmark with names that match `[regex]`.
- `--benchmark_filter="-[regex]"`: Only run benchmark with names that DON'T match `[regex]`.
- `--benchmark_min_time=[float]`: Make sure each benchmark repetition gathers `[float]` seconds
  of data.
- `--benchmark_repetitions=[int]`: Run each benchmark `[int]` times and report aggregate statistics 
  (mean, stdev, etc). A "repetition" refers to a single execution of the benchmark function, not
  an "iteration", which is a loop of the `for(auto _:state){...}` section.
- `--benchmark_report_aggregates_only="true|false"`: If true, only the aggregate statistics are
  reported (affects both console and file output). Requires `--benchmark_repetitions` to be useful.
- `--benchmark_display_aggregates_only="true|false"`: If true, only the aggregate statistics are
  printed to the terminal. Any file output will still contain all repetition info.
- `--benchmark_format="console|json|csv"`: Specify terminal output format: human readable 
  (`console`) or `csv`/`json` formats.
- `--benchmark_out_format="console|json|csv"`: Specify file output format: human readable 
  (`console`) or `csv`/`json` formats.
- `--benchmark_out=[filename]`: Specify output file.
- `--benchmark_color="true|false"`: Toggle color output in terminal when using `console` output.
- `--benchmark_counters_tabular="true|false"`: Print counter information (e.g. bytes/sec, items/sec)
  in the table, rather than appending them as a label.

For more information and examples of practical usage, take a look at the existing benchmarks in
vtk-m/benchmarking/.
