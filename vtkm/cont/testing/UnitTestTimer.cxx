//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/openmp/internal/DeviceAdapterTagOpenMP.h>
#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/internal/Windows.h>
namespace
{

void Time()
{
  vtkm::cont::Timer timer;
  timer.Start();
  VTKM_TEST_ASSERT(timer.Started(), "Timer fails to track started status");
  VTKM_TEST_ASSERT(!timer.Stopped(), "Timer fails to track non stopped status");

#ifdef VTKM_WINDOWS
  Sleep(1000);
#else
  sleep(1);
#endif

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();
  VTKM_TEST_ASSERT(!timer.Stopped(), "Timer fails to track stopped status");

  std::cout << "Elapsed time measured by any Tag: " << elapsedTime << std::endl;
  VTKM_TEST_ASSERT(elapsedTime > 0.999, "General Timer did not capture full second wait.");
  VTKM_TEST_ASSERT(elapsedTime < 2.0, "General Timer counted too far or system really busy.");

  vtkm::cont::RuntimeDeviceTracker tracker;

  vtkm::Float64 elapsedTimeCuda = timer.GetElapsedTime(vtkm::cont::DeviceAdapterTagCuda());
  if (tracker.CanRunOn(vtkm::cont::DeviceAdapterTagCuda()))
  {
    std::cout << " can on run cuda?: true" << std::endl;
    std::cout << "Elapsed time measured by cuda Tag: " << elapsedTime << std::endl;
    VTKM_TEST_ASSERT(elapsedTimeCuda > 0.999, "Cuda Timer did not capture full second wait.");
    VTKM_TEST_ASSERT(elapsedTimeCuda < 2.0, "Cuda Timer counted too far or system really busy.");
  }
  else
  {
    VTKM_TEST_ASSERT(elapsedTimeCuda == 0.0, "Disabled Cuda Timer should return nothing.");
  }

  vtkm::Float64 elapsedTimeSerial = timer.GetElapsedTime(vtkm::cont::DeviceAdapterTagSerial());
  std::cout << "Elapsed time measured by serial Tag: " << elapsedTime << std::endl;
  VTKM_TEST_ASSERT(elapsedTimeSerial > 0.999, "Serial Timer did not capture full second wait.");
  VTKM_TEST_ASSERT(elapsedTimeSerial < 2.0, "Serial Timer counted too far or system really busy.");

  vtkm::Float64 elapsedTimeOpenMP = timer.GetElapsedTime(vtkm::cont::DeviceAdapterTagOpenMP());
  if (vtkm::cont::DeviceAdapterTagOpenMP::IsEnabled)
  {
    std::cout << "Elapsed time measured by openmp Tag: " << elapsedTime << std::endl;
    VTKM_TEST_ASSERT(elapsedTimeOpenMP > 0.999, "OpenMP Timer did not capture full second wait.");
    VTKM_TEST_ASSERT(elapsedTimeOpenMP < 2.0,
                     "OpenMP Timer counted too far or system really busy.");
  }
  else
  {
    VTKM_TEST_ASSERT(elapsedTimeOpenMP == 0.0, "Disabled OpenMP Timer should return nothing.");
  }

  vtkm::Float64 elapsedTimeTBB = timer.GetElapsedTime(vtkm::cont::DeviceAdapterTagTBB());
  if (vtkm::cont::DeviceAdapterTagTBB::IsEnabled)
  {
    std::cout << "Elapsed time measured by tbb Tag: " << elapsedTime << std::endl;
    VTKM_TEST_ASSERT(elapsedTimeTBB > 0.999, "TBB Timer did not capture full second wait.");
    VTKM_TEST_ASSERT(elapsedTimeTBB < 2.0, "TBB Timer counted too far or system really busy.");
  }
  else
  {
    VTKM_TEST_ASSERT(elapsedTimeOpenMP == 0.0, "Disabled TBB Timer should return nothing.");
  }

  std::cout << "Reuse the timer to test continuous timing." << std::endl;
#ifdef VTKM_WINDOWS
  Sleep(1000);
#else
  sleep(1);
#endif

  elapsedTime = timer.GetElapsedTime();

  std::cout << "Elapsed time measured by any Tag: " << elapsedTime << std::endl;
  VTKM_TEST_ASSERT(elapsedTime > 1.999,
                   "General Timer did not capture two full seconds wait in second launch.");
  VTKM_TEST_ASSERT(elapsedTime < 3.0,
                   "General Timer counted too far or system really busy in second launch.");

  timer.Stop();
  VTKM_TEST_ASSERT(timer.Stopped(), "Timer fails to track stopped status");
#ifdef VTKM_WINDOWS
  Sleep(1000);
#else
  sleep(1);
#endif

  std::cout << "Elapsed time measured by any Tag with an early stop: " << elapsedTime << std::endl;
  VTKM_TEST_ASSERT(elapsedTime > 1.999,
                   "General Timer did not capture two full seconds wait in second launch.");
  VTKM_TEST_ASSERT(elapsedTime < 3.0,
                   "General Timer counted too far or system really busy in second launch.");
}

} // anonymous namespace

int UnitTestTimer(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Time, argc, argv);
}
