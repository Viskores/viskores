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

#include <vtkm/cont/Timer.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/internal/Windows.h>
namespace
{

void Time()
{
  vtkm::cont::Timer<> timer;

#ifdef VTKM_WINDOWS
  Sleep(1000);
#else
  sleep(1);
#endif

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();

  std::cout << "Elapsed time: " << elapsedTime << std::endl;

  VTKM_TEST_ASSERT(elapsedTime > 0.999, "Timer did not capture full second wait.");
  VTKM_TEST_ASSERT(elapsedTime < 2.0, "Timer counted too far or system really busy.");
}

} // anonymous namespace

int UnitTestTimer(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(Time);
}
