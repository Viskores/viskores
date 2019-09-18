#!/usr/bin/env python
#
# Prints a concise summary of a benchmark output as a TSV blob.
#
# Example usage:
#
# $ BenchmarkXXX_DEVICE > bench.out
# $ benchSummary.py bench.out
#
# Options SortByType, SortByName, or SortByMean may be passed after the
# filename to sort the output by the indicated quantity. If no sort option
# is provided, the output order matches the input. If multiple options are
# specified, the list will be sorted repeatedly in the order requested.

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

benchmarks = {}
parseFile(benchFile, benchmarks)

# Sort keys by type:
keys = benchmarks.keys()
if sortOpt:
  for opt in sortOpt:
    if opt.lower() == "sortbytype":
      keys = sorted(keys, key=lambda k: k.type)
    elif opt.lower() == "sortbyname":
      keys = sorted(keys, key=lambda k: k.name)
    elif opt.lower() == "sortbymean":
      keys = sorted(keys, key=lambda k: benchmarks[k].mean)

print("# Summary: (%s)"%filename)
print("%-9s\t%-9s\t%-9s\t%-s"%("Mean", "Stdev", "Stdev%", "Benchmark (type)"))
for key in keys:
  data = benchmarks[key]
  print("%9.6f\t%9.6f\t%9.6f\t%s (%s)"%(data.mean, data.stdDev, data.stdDev / data.mean * 100., key.name, key.type))
