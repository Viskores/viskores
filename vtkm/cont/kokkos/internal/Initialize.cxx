//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/kokkos/internal/Initialize.h>

#include <vtkm/Assert.h>
#include <vtkm/internal/Configure.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <Kokkos_Core.hpp>
VTKM_THIRDPARTY_POST_INCLUDE

#include <cstdlib>
#include <string>
#include <vector>

namespace
{
// Performs an in-place change to the name of an argument in the parameter list.
// Requires `newName` length to be <= `origName` length.
inline void ChangeArgumentName(const std::string& origName,
                               const std::string& newName,
                               int argc,
                               char* argv[])
{
  VTKM_ASSERT(newName.length() <= origName.length());

  for (int i = 0; i < argc; ++i)
  {
    auto argStr = std::string(argv[i]);
    auto argName = argStr.substr(0, argStr.find_first_of('='));
    if (argName == origName)
    {
      auto newArg = newName + argStr.substr(argName.length());
      newArg.copy(argv[i], newArg.length());
      argv[i][newArg.length()] = '\0';
    }
  }
}
} // anonymous namespace

void vtkm::cont::kokkos::internal::Initialize(int& argc, char* argv[])
{
  // mangle --device to prevent conflict
  ChangeArgumentName("--device", "--vtkm_d", argc, argv);
  // rename to what is expected by kokkos
  ChangeArgumentName("--kokkos_device", "--device", argc, argv);
  ChangeArgumentName("--kokkos_device-id", "--device-id", argc, argv);

  if (!Kokkos::is_initialized())
  {
    Kokkos::initialize(argc, argv);
    std::atexit(Kokkos::finalize);
  }

  // de-mangle
  ChangeArgumentName("--vtkm_d", "--device", argc, argv);
}
