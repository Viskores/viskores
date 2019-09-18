#!/usr/bin/env python3
#
# Compares the output from BenchmarkDeviceAdapter from the serial
# device to a parallel device and prints a table containing the results.
#
# Example usage:
#
# $ BenchmarkDeviceAdapter_SERIAL > serial.out
# $ BenchmarkDeviceAdapter_TBB > tbb.out
# $ benchCompare.py serial.out tbb.out
#
#
# The number of threads (optional -- only used to generate the "Warn" column)
maxThreads = 4
#
# Print debugging output:
doDebug = False
#
# End config options.

import re
import sys

assert(len(sys.argv) == 3)

def debug(str):
  if (doDebug): print(str)

# Parses "*** vtkm::Float64 ***************" --> vtkm::Float64
typeParser = re.compile("\\*{3} ([^*]+) on device ([^*]+) \\*{15}")

# Parses "Benchmark 'Benchmark name' results:" --> Benchmark name
nameParser = re.compile("Benchmark '([^-]+)' results:")

# Parses "mean = 0.0125s" --> 0.0125
meanParser = re.compile("\\s+mean = ([0-9.Ee+-]+)s")

# Parses "std dev = 0.0125s" --> 0.0125
stdDevParser = re.compile("\\s+std dev = ([naN0-9.Ee+-]+)s")

serialFilename = sys.argv[1]
parallelFilename = sys.argv[2]

serialFile = open(serialFilename, 'r')
parallelFile = open(parallelFilename, 'r')

class BenchKey:
  def __init__(self, name_, type_):
    self.name = name_
    self.type = type_

  def __eq__(self, other):
    return self.name == other.name and self.type == other.type

  def __lt__(self, other):
    if self.name < other.name: return True
    elif self.name > other.name: return False
    else: return self.type < other.type

  def __hash__(self):
    return (self.name + self.type).__hash__()

class BenchData:
  def __init__(self, mean_, stdDev_):
    self.mean = mean_
    self.stdDev = stdDev_

def parseFile(f, benchmarks):
  type = ""
  bench = ""
  mean = -1.
  stdDev = -1.
  for line in f:
    debug("Line: {}".format(line))

    typeRes = typeParser.match(line)
    if typeRes:
      type = typeRes.group(1)
      debug("Found type: {}".format(type))
      continue

    nameRes = nameParser.match(line)
    if nameRes:
      name = nameRes.group(1)
      debug("Found name: {}".format(name))
      continue

    meanRes = meanParser.match(line)
    if meanRes:
      mean = float(meanRes.group(1))
      debug("Found mean: {}".format(mean))
      continue

    stdDevRes = stdDevParser.match(line)
    if stdDevRes:
      stdDev = float(stdDevRes.group(1))
      debug("Found stddev: {}".format(stdDev))

      # stdDev is always the last parse for a given benchmark, add entry now
      benchmarks[BenchKey(name, type)] = BenchData(mean, stdDev)
      debug("{} records found.".format(len(benchmarks)))

      mean = -1.
      stdDev = -1.

      continue

serialBenchmarks = {}
parallelBenchmarks = {}

parseFile(serialFile, serialBenchmarks)
parseFile(parallelFile, parallelBenchmarks)

serialKeys = set(serialBenchmarks.keys())
parallelKeys = set(parallelBenchmarks.keys())

commonKeys = sorted(list(serialKeys.intersection(parallelKeys)))

serialOnlyKeys = sorted(list(serialKeys.difference(parallelKeys)))
parallelOnlyKeys = sorted(list(parallelKeys.difference(serialKeys)))

debug("{} serial keys\n{} parallel keys\n{} common keys\n{} serialOnly keys\n{} parallelOnly keys.".format(
        len(serialKeys), len(parallelKeys), len(commonKeys), len(serialOnlyKeys), len(parallelOnlyKeys)))

if len(serialOnlyKeys) > 0:
  print("Keys found only in serial:")
  for k in serialOnlyKeys:
    print("%s (%s)"%(k.name, k.type))
  print("")

if len(parallelOnlyKeys) > 0:
  print("Keys found only in parallel:")
  for k in parallelOnlyKeys:
    print("%s (%s)"%(k.name, k.type))
  print("")

print("Comparison:")
print("| %7s | %4s | %8s    %8s | %8s    %8s | %s (%s) |"%(
        "Speedup", "Warn", "serial", "", "parallel", "", "Benchmark", "Type"))
print("|-%7s-|-%4s-|-%8s----%8s-|-%8s----%8s-|-%s--%s--|"%(
        "-"*7, "-"*4, "-"*8, "-"*8, "-"*8, "-"*8, "-"*9, "-"*4))
for key in commonKeys:
  sData = serialBenchmarks[key]
  pData = parallelBenchmarks[key]
  speedup = sData.mean / pData.mean if pData.mean != 0. else 0.
  if speedup > maxThreads * .9:
    flag = "    "
  elif speedup > maxThreads * .75:
    flag = "!   "
  elif speedup > maxThreads * .5:
    flag = "!!  "
  elif speedup > maxThreads * .25:
    flag = "!!! "
  else:
    flag = "!!!!"
  print("| %7.3f | %4s | %08.6f +- %08.6f | %08.6f +- %08.6f | %s (%s) |"%(
          speedup, flag, sData.mean, sData.stdDev, pData.mean, pData.stdDev, key.name, key.type))
