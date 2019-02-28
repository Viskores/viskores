//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_Initialize_h
#define vtk_m_cont_Initialize_h

#include <vtkm/cont/internal/DeviceAdapterTag.h>
#include <vtkm/cont/vtkm_cont_export.h>
#include <vtkm/internal/ExportMacros.h>

#include <string>
#include <type_traits>
#include <vector>

namespace vtkm
{
namespace cont
{

struct InitializeResult
{
  /// Device passed into -d, or undefined
  DeviceAdapterId Device = DeviceAdapterTagUndefined{};

  /// Non-option arguments
  std::vector<std::string> Arguments;
};

enum class InitializeOptions
{
  None = 0x0,
  RequireDevice = 0x1
};

// Allow options to be used as a bitfield
inline InitializeOptions operator|(const InitializeOptions& lhs, const InitializeOptions& rhs)
{
  using T = std::underlying_type<InitializeOptions>::type;
  return static_cast<InitializeOptions>(static_cast<T>(lhs) | static_cast<T>(rhs));
}
inline InitializeOptions operator&(const InitializeOptions& lhs, const InitializeOptions& rhs)
{
  using T = std::underlying_type<InitializeOptions>::type;
  return static_cast<InitializeOptions>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

/**
 * Initialize the VTKm library, parsing arguments when provided:
 * - Sets log level names when logging is configured.
 * - Sets the calling thread as the main thread for logging purposes.
 * - Sets the default log level to the argument provided to -v.
 * - Forces usage of the device name passed to -d or --device.
 * - Prints usage when -h is passed.
 *
 * The parameterless version only sets up log level names.
 *
 * Additional options may be supplied via the @a opts argument, such as
 * requiring the -d option.
 *
 * Results are available in the returned InitializeResult.
 *
 * @note This method may call exit() on parse error.
 * @{
 */
VTKM_CONT_EXPORT
VTKM_CONT
InitializeResult Initialize(int& argc,
                            char* argv[],
                            InitializeOptions opts = InitializeOptions::None);
VTKM_CONT_EXPORT
VTKM_CONT
InitializeResult Initialize();
/**@}*/
}
} // end namespace vtkm::cont


#endif // vtk_m_cont_Initialize_h
