# More performance test options

More options are available for adding performance regression tests. These
options allow you to pass custom options to the benchmark test so that you
are not limited to the default values. They also allow multiple tests to be
created from the same benchmark executable. Separating out the benchmarks
allows the null hypothesis testing to better catch performance problems
when only one of the tested filters regresses. It also allows passing
different arguments to different benchmarks.
