//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/TypeTraits.h>

#include "Benchmarker.h"

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/internal/Configure.h>

#include <vtkm/testing/Testing.h>

#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef VTKM_ENABLE_TBB
#include <tbb/task_scheduler_init.h>
#endif // TBB

// For the TBB implementation, the number of threads can be customized using a
// "NumThreads [numThreads]" argument.

namespace vtkm
{
namespace benchmarking
{

const vtkm::UInt64 COPY_SIZE_MIN = (1 << 10); // 1 KiB
const vtkm::UInt64 COPY_SIZE_MAX = (1 << 29); // 512 MiB
const vtkm::UInt64 COPY_SIZE_INC = 1;         // Used as 'size <<= INC'

const size_t COL_WIDTH = 32;

template <typename ValueType, typename DeviceAdapter>
struct MeasureCopySpeed
{
  using Algo = vtkm::cont::Algorithm;

  vtkm::cont::ArrayHandle<ValueType> Source;
  vtkm::cont::ArrayHandle<ValueType> Destination;
  vtkm::UInt64 NumBytes;

  VTKM_CONT
  MeasureCopySpeed(vtkm::UInt64 bytes)
    : NumBytes(bytes)
  {
    vtkm::Id numValues = static_cast<vtkm::Id>(bytes / sizeof(ValueType));
    this->Source.Allocate(numValues);
  }

  VTKM_CONT vtkm::Float64 operator()()
  {
    vtkm::cont::Timer timer{ DeviceAdapter() };
    timer.Start();
    Algo::Copy(this->Source, this->Destination);

    return timer.GetElapsedTime();
  }

  VTKM_CONT std::string Description() const
  {
    vtkm::UInt64 actualSize = sizeof(ValueType);
    actualSize *= static_cast<vtkm::UInt64>(this->Source.GetNumberOfValues());
    std::ostringstream out;
    out << "Copying " << vtkm::cont::GetHumanReadableSize(this->NumBytes)
        << " (actual=" << vtkm::cont::GetHumanReadableSize(actualSize) << ") of "
        << vtkm::testing::TypeName<ValueType>::Name() << "\n";
    return out.str();
  }
};

void PrintRow(std::ostream& out, const std::string& label, const std::string& data)
{
  out << "| " << std::setw(COL_WIDTH) << label << " | " << std::setw(COL_WIDTH) << data << " |"
      << std::endl;
}

void PrintDivider(std::ostream& out)
{
  const std::string fillStr(COL_WIDTH, '-');

  out << "|-" << fillStr << "-|-" << fillStr << "-|" << std::endl;
}

template <typename ValueType, typename DeviceAdapter>
void BenchmarkValueType(vtkm::cont::DeviceAdapterId id)
{
  PrintRow(std::cout, vtkm::testing::TypeName<ValueType>::Name(), id.GetName());

  PrintDivider(std::cout);

  Benchmarker bench(15, 100);
  for (vtkm::UInt64 size = COPY_SIZE_MIN; size <= COPY_SIZE_MAX; size <<= COPY_SIZE_INC)
  {
    MeasureCopySpeed<ValueType, DeviceAdapter> functor(size);
    bench.Reset();

    std::string speedStr;

    try
    {
      bench.GatherSamples(functor);
      vtkm::Float64 speed = static_cast<Float64>(size) / stats::Mean(bench.GetSamples());
      speedStr = vtkm::cont::GetHumanReadableSize(static_cast<UInt64>(speed)) + std::string("/s");
    }
    catch (vtkm::cont::ErrorBadAllocation&)
    {
      speedStr = "[allocation too large]";
    }

    PrintRow(std::cout, vtkm::cont::GetHumanReadableSize(size), speedStr);
  }

  std::cout << "\n";
}
}
} // end namespace vtkm::benchmarking

namespace
{
using namespace vtkm::benchmarking;

struct BenchmarkValueTypeFunctor
{
  template <typename DeviceAdapter>
  bool operator()(DeviceAdapter id)
  {
    BenchmarkValueType<vtkm::UInt8, DeviceAdapter>(id);
    BenchmarkValueType<vtkm::Vec<vtkm::UInt8, 2>, DeviceAdapter>(id);
    BenchmarkValueType<vtkm::Vec<vtkm::UInt8, 3>, DeviceAdapter>(id);
    BenchmarkValueType<vtkm::Vec<vtkm::UInt8, 4>, DeviceAdapter>(id);

    BenchmarkValueType<vtkm::UInt32, DeviceAdapter>(id);
    BenchmarkValueType<vtkm::Vec<vtkm::UInt32, 2>, DeviceAdapter>(id);

    BenchmarkValueType<vtkm::UInt64, DeviceAdapter>(id);
    BenchmarkValueType<vtkm::Vec<vtkm::UInt64, 2>, DeviceAdapter>(id);

    BenchmarkValueType<vtkm::Float32, DeviceAdapter>(id);
    BenchmarkValueType<vtkm::Vec<vtkm::Float32, 2>, DeviceAdapter>(id);

    BenchmarkValueType<vtkm::Float64, DeviceAdapter>(id);
    BenchmarkValueType<vtkm::Vec<vtkm::Float64, 2>, DeviceAdapter>(id);

    BenchmarkValueType<vtkm::Pair<vtkm::UInt32, vtkm::Float32>, DeviceAdapter>(id);
    BenchmarkValueType<vtkm::Pair<vtkm::UInt32, vtkm::Float64>, DeviceAdapter>(id);
    BenchmarkValueType<vtkm::Pair<vtkm::UInt64, vtkm::Float32>, DeviceAdapter>(id);
    BenchmarkValueType<vtkm::Pair<vtkm::UInt64, vtkm::Float64>, DeviceAdapter>(id);

    return true;
  }
};
}

int main(int argc, char* argv[])
{
  auto opts = vtkm::cont::InitializeOptions::RequireDevice |
    vtkm::cont::InitializeOptions::ErrorOnBadOption | vtkm::cont::InitializeOptions::AddHelp;
  auto config = vtkm::cont::Initialize(argc, argv, opts);


#ifdef VTKM_ENABLE_TBB
  int numThreads = tbb::task_scheduler_init::automatic;
#endif // TBB

  if (argc == 3)
  {
    if (std::string(argv[1]) == "NumThreads")
    {
#ifdef VTKM_ENABLE_TBB
      std::istringstream parse(argv[2]);
      parse >> numThreads;
      std::cout << "Selected " << numThreads << " TBB threads." << std::endl;
#else
      std::cerr << "NumThreads valid only on TBB. Ignoring." << std::endl;
#endif // TBB
    }
  }

#ifdef VTKM_ENABLE_TBB
  // Must not be destroyed as long as benchmarks are running:
  tbb::task_scheduler_init init(numThreads);
#endif // TBB

  BenchmarkValueTypeFunctor functor;
  vtkm::cont::TryExecuteOnDevice(config.Device, functor);
}
