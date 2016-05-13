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

#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/benchmarking/BenchmarkDeviceAdapter.h>

#include <iostream>
#include <algorithm>
#include <string>
#include <cctype>

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
      if (arg == "lowerbounds"){
        benchmarks |= vtkm::benchmarking::LOWER_BOUNDS;
      }
      else if (arg == "reduce"){
        benchmarks |= vtkm::benchmarking::REDUCE;
      }
      else if (arg == "reducebykey"){
        benchmarks |= vtkm::benchmarking::REDUCE_BY_KEY;
      }
      else if (arg == "scaninclusive"){
        benchmarks |= vtkm::benchmarking::SCAN_INCLUSIVE;
      }
      else if (arg == "scanexclusive"){
        benchmarks |= vtkm::benchmarking::SCAN_EXCLUSIVE;
      }
      else if (arg == "sort"){
        benchmarks |= vtkm::benchmarking::SORT;
      }
      else if (arg == "sortbykey"){
        benchmarks |= vtkm::benchmarking::SORT_BY_KEY;
      }
      else if (arg == "streamcompact"){
        benchmarks |= vtkm::benchmarking::STREAM_COMPACT;
      }
      else if (arg == "unique"){
        benchmarks |= vtkm::benchmarking::UNIQUE;
      }
      else if (arg == "upperbounds"){
        benchmarks |= vtkm::benchmarking::UPPER_BOUNDS;
      }
      else {
        std::cout << "Unrecognized benchmark: " << argv[i] << std::endl;
        return 1;
      }
    }
  }

  //now actually execute the benchmarks
  return vtkm::benchmarking::BenchmarkDeviceAdapter
    <VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Run(benchmarks);
}

