#!/usr/bin/env python
#
# Prints a concise summary of a benchmark output as a TSV blob. Benchmarks are
# expected to have "Baseline" in the name, and a matching benchmark with the
# same name but Baseline replaced with something else. For example,
#
# Baseline benchmark name: "Some benchmark: Baseline, Size=4"
# Test benchmark name:     "Some benchmark: Blahblah, Size=4"
#
# The output will print the baseline, test, and overhead times for the
# benchmarks.
#
# Example usage:
#
# $ BenchmarkXXX_DEVICE > bench.out
# $ benchSummaryWithBaselines.py bench.out
#
# Options SortByType, SortByName, SortByOverhead, or SortByRatio
# (testtime/baseline) may be passed after the filename to sort the output by
# the indicated quantity. If no sort option is provided, the output order
# matches the input. If multiple options are specified, the list will be sorted
# repeatedly in the order requested.

import re
import sys

assert(len(sys.argv) >= 2)

# Parses "*** vtkm::Float64 ***************" --> vtkm::Float64
typeParser = re.compile("\\*{3} ([^*]+) \\*{15}")

# Parses "Benchmark 'Benchmark name' results:" --> Benchmark name
nameParser = re.compile("Benchmark '([^-]+)' results:")

# Parses "mean = 0.0125s" --> 0.0125
meanParser = re.compile("\\s+mean = ([0-9.Ee+-]+)s")

# Parses "std dev = 0.0125s" --> 0.0125
stdDevParser = re.compile("\\s+std dev = ([naN0-9.Ee+-]+)s")

# Parses "SomeText Baseline Other Text" --> ("SomeText ", " Other Text")
baselineParser = re.compile("(.*)Baseline(.*)")

filename = sys.argv[1]
benchFile = open(filename, 'r')

sortOpt = None
if len(sys.argv) > 2:
  sortOpt = sys.argv[2:]

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
    typeRes = typeParser.match(line)
    if typeRes:
      type = typeRes.group(1)
      continue

    nameRes = nameParser.match(line)
    if nameRes:
      name = nameRes.group(1)
      continue

    meanRes = meanParser.match(line)
    if meanRes:
      mean = float(meanRes.group(1))
      continue

    stdDevRes = stdDevParser.match(line)
    if stdDevRes:
      stdDev = float(stdDevRes.group(1))

      # stdDev is always the last parse for a given benchmark, add entry now
      benchmarks[BenchKey(name, type)] = BenchData(mean, stdDev)

      mean = -1.
      stdDev = -1.

      continue

class BaselinedBenchData:
  def __init__(self, baseline, test):
    self.baseline = baseline.mean
    self.test = test.mean
    self.overhead = test.mean - baseline.mean

def findBaselines(benchmarks):
  result = {}

  for baseKey in benchmarks.keys():
    # Look for baseline entries
    baselineRes = baselineParser.match(baseKey.name)
    if baselineRes:
      prefix = baselineRes.group(1)
      suffix = baselineRes.group(2)

      # Find the test entry matching the baseline:
      for testKey in benchmarks.keys():
        if baseKey.type != testKey.type: # Need same type
          continue
        if baseKey.name == testKey.name: # Skip the base key
          continue
        if testKey.name.startswith(prefix) and testKey.name.endswith(suffix):
          newName = (prefix + suffix).replace(", ,", ",")
          newKey = BenchKey(newName, testKey.type)
          newVal = BaselinedBenchData(benchmarks[baseKey], benchmarks[testKey])
          result[newKey] = newVal
  return result

benchmarks = {}
parseFile(benchFile, benchmarks)
benchmarks = findBaselines(benchmarks)

# Sort keys by type:
keys = benchmarks.keys()
if sortOpt:
  for opt in sortOpt:
    if opt.lower() == "sortbytype":
      keys = sorted(keys, key=lambda k: k.type)
    elif opt.lower() == "sortbyname":
      keys = sorted(keys, key=lambda k: k.name)
    elif opt.lower() == "sortbyoverhead":
      keys = sorted(keys, key=lambda k: benchmarks[k].overhead)
    elif opt.lower() == "sortbyratio":
      keys = sorted(keys, key=lambda k: benchmarks[k].overhead / benchmarks[k].baseline)

print("# Summary: (%s)"%filename)
print("%-9s\t%-9s\t%-9s\t%-9s\t%-s"%("Baseline", "TestTime", "Overhead", "Test/Base", "Benchmark (type)"))
for key in keys:
  data = benchmarks[key]
  print("%9.6f\t%9.6f\t%9.6f\t%9.6f\t%s (%s)"%(data.baseline, data.test,
        data.overhead, data.test / data.baseline, key.name, key.type))
