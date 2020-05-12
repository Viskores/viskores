# BENCHMARKING VTK-m

## TL;DR

When configuring _VTM-m_ with _CMake_ pass the flag `-DVTKm_ENABLE_BENCHMARKS=1`
. In the build directory you will see the following binaries:

    $ ls bin/Benchmark*
    bin/BenchmarkArrayTransfer*  bin/BenchmarkCopySpeeds* bin/BenchmarkFieldAlgorithms*
    bin/BenchmarkRayTracing* bin/BenchmarkAtomicArray*    bin/BenchmarkDeviceAdapter*
    bin/BenchmarkFilters* bin/BenchmarkTopologyAlgorithms*

Taking as an example `BenchmarkArrayTransfer`, we can run it as:

    $ bin/BenchmarkArrayTransfer -d Any

---

## Parts of this Documents

0. [TL;DR](#TL;DR)
1. [Devices](#choosing-devices)
2. [Filters](#run-a-subset-of-your-benchmarks)
4. [Compare with baseline](#compare-with-baseline)
5. [Installing compare.py](#installing-compare-benchmarkspy)

---

## Choosing devices

Taking as an example `BenchmarkArrayTransfer`, we can determine in which
device we can run it by simply:

    $ bin/BenchmarkArrayTransfer
    ...
    Valid devices: "Any" "Serial"
    ...

Upon the _Valid devices_ you can chose in which device to run the benchmark by:

    $ bin/BenchmarkArrayTransfer -d Serial


## Run a subset of your benchmarks

_VTK-m_ benchmarks uses [Google Benchmarks] which allows you to choose a subset
of benchmaks by using the flag `--benchmark_filter=REGEX`

For instance, if you want to run all the benchmarks that writes something you
would run:

    $ bin/BenchmarkArrayTransfer -d Serial --benchmark_filter='Write'

Note you can list all of the available benchmarks with the option:
`--benchmark_list_tests`.

## Compare with baseline

_VTM-m_ ships with a helper script based in [Google Benchmarks] `compare.py`
named `compare-benchmarks.py` which lets you compare benchmarks using different
devices, filters, and binaries. After building `VTM-m` it must appear on the 
`bin` directory within your `build` directory.

When running `compare-benchmarks.py`:
 - You can specify the baseline benchmark binary path and its arguments in 
   `--benchmark1=`
 - The contender benchmark binary path and its arguments in `--benchmark2=`
 - Extra options to be passed to `compare.py` must come after `--`

### Compare between filters

When comparing filters, we only can use one benchmark binary with a single device
as shown in the following example:

```sh
$ ./compare-benchmarks.py --benchmark1='./BenchmarkArrayTransfer -d Any
--benchmark_filter=1024' --filter1='Read' --filter2=Write -- filters

# It will output something like this:

Benchmark                                                                          Time             CPU      Time Old      Time New       CPU Old       CPU New
---------------------------------------------------------------------------------------------------------------------------------------------------------------
BenchContToExec[Read vs. Write]<F32>/Bytes:1024/manual_time                     +0.2694         +0.2655         18521         23511         18766         23749
BenchExecToCont[Read vs. Write]<F32>/Bytes:1024/manual_time                     +0.0212         +0.0209         25910         26460         26152         26698
```

### Compare between devices

When comparing two benchmarks using two devices use the _option_ `benchmark`
after `--` and call `./compare-benchmarks.py` as follows:

```sh
$ ./compare-benchmarks.py --benchmark1='./BenchmarkArrayTransfer -d Serial
--benchmark_filter=1024' --benchmark2='./BenchmarkArrayTransfer -d Cuda
--benchmark_filter=1024' -- benchmarks


# It will output something like this:

Benchmark                                                              Time             CPU      Time Old      Time New       CPU Old       CPU New
---------------------------------------------------------------------------------------------------------------------------------------------------
BenchContToExecRead<F32>/Bytes:1024/manual_time                     +0.0127         +0.0120         18388         18622         18632         18856
BenchContToExecWrite<F32>/Bytes:1024/manual_time                    +0.0010         +0.0006         23471         23496         23712         23726
BenchContToExecReadWrite<F32>/Bytes:1024/manual_time                -0.0034         -0.0041         26363         26274         26611         26502
BenchRoundTripRead<F32>/Bytes:1024/manual_time                      +0.0055         +0.0056         20635         20748         21172         21291
BenchRoundTripReadWrite<F32>/Bytes:1024/manual_time                 +0.0084         +0.0082         29288         29535         29662         29905
BenchExecToContRead<F32>/Bytes:1024/manual_time                     +0.0025         +0.0021         25883         25947         26122         26178
BenchExecToContWrite<F32>/Bytes:1024/manual_time                    -0.0027         -0.0038         26375         26305         26622         26522
BenchExecToContReadWrite<F32>/Bytes:1024/manual_time                +0.0041         +0.0039         25639         25745         25871         25972
```

## Installing compare-benchmarks.py

`compare-benchmarks.py` relies on `compare.py` from Google Benchmarks which also
relies in `SciPy`, you can find instructions [here][SciPy] regarding its
installation.

[Google Benchmarks]: https://github.com/google/benchmark
[Compare.py]:        https://github.com/google/benchmark/blob/master/tools/compare.py
[SciPy]:             https://www.scipy.org/install.html
