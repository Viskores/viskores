This document describes how to use the command-line options for Google Benchmarks (GBench) to control the behavior of BenchmarkInSitu.

Generally, "BenchmarkInSitu --help" will provide the list of standard benchmarks along with the associated ones from GBench.

As a refresher, GBench iterates a test defined in the application, in this case, Contour, Streamlines, ..., a number of times until two criteria are met, a statistically stable set of samples have been generated, and the test ran for a specified minimum amount of time (by default, 0.5 seconds).

There are three ways to run the InSitu benchmark that control the number of iterations run by GBench. These are independent of the "standard" arguments passed to the benchmark (we'll define the standard arguments as: --vtkm-device, --size, --image-size, plus other not defined by GBench).

1. BenchmarkInSitu <standard arguments>
	- Under this scenario, the iterations are controlled completely by GBench. Generally, each test will be run between 1 and N iterations depending on how long each test runs.

2. BenchmarkInSitu <standard arguments> --benchmark_min_time=<min_time>
	- This will ensure that the test will run for at least <min_time> seconds. You will set this option if you don't care about the actual number of iterations, but only that each test runs for at least a specified time.

3. BenchmarkInSitu <standard arguments> --benchmark_repetitions=<reps>
	- The purpose of this option is to *exactly* control the number of iterations performed by GBench. Internally, this does two things:
		- Sets the minimum time to a very small value ("--benchmark_min_time=0.00000001")
		- Sets the output to only report aggregate statistics for each test, e.g., mean, median, standard deviation (--benchmark_report_aggregates_only=true)
	Both of these arguments can be overridden by providing different values on the command-line. With the current setting, all test runs have resulted in only <reps> repetitions being executed.

