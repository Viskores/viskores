//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/Initialize.h>

#include <vtkm/cont/Logging.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/internal/OptionParser.h>

#include <memory>
#include <sstream>

namespace opt = vtkm::cont::internal::option;

namespace
{

enum OptionIndex
{
  PREAMBLE, // usage header strings
  DEVICE,
  LOGLEVEL, // not parsed by this parser, but by loguru
  HELP
};

struct VtkmArg : public opt::Arg
{
  static opt::ArgStatus IsDevice(const opt::Option& option, bool msg)
  {
    // Device must be specified if option is present:
    if (option.arg == nullptr)
    {
      if (msg)
      {
        VTKM_LOG_ALWAYS_S(vtkm::cont::LogLevel::Error,
                          "Missing device after option '"
                            << std::string(option.name, static_cast<size_t>(option.namelen))
                            << "'.\nValid devices are: "
                            << VtkmArg::GetValidDeviceNames()
                            << "\n");
      }
      return opt::ARG_ILLEGAL;
    }

    auto id = vtkm::cont::make_DeviceAdapterId(option.arg);

    if (!VtkmArg::DeviceIsAvailable(id))
    {
      VTKM_LOG_ALWAYS_S(vtkm::cont::LogLevel::Error,
                        "Unavailable device specificed after option '"
                          << std::string(option.name, static_cast<size_t>(option.namelen))
                          << "': '"
                          << option.arg
                          << "'.\nValid devices are: "
                          << VtkmArg::GetValidDeviceNames()
                          << "\n");
      return opt::ARG_ILLEGAL;
    }

    return opt::ARG_OK;
  }

  static bool DeviceIsAvailable(vtkm::cont::DeviceAdapterId id)
  {
    if (id == vtkm::cont::DeviceAdapterTagAny{})
    {
      return true;
    }

    if (id.GetValue() <= 0 || id.GetValue() >= VTKM_MAX_DEVICE_ADAPTER_ID ||
        id == vtkm::cont::DeviceAdapterTagUndefined{})
    {
      return false;
    }

    auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
    bool result = false;
    try
    {
      result = tracker.CanRunOn(id);
    }
    catch (...)
    {
      result = false;
    }
    return result;
  }

  static std::string GetValidDeviceNames()
  {
    std::ostringstream names;
    names << "\"Any\" ";

    for (vtkm::Int8 i = 0; i < VTKM_MAX_DEVICE_ADAPTER_ID; ++i)
    {
      auto id = vtkm::cont::make_DeviceAdapterId(i);
      if (VtkmArg::DeviceIsAvailable(id))
      {
        names << "\"" << id.GetName() << "\" ";
      }
    }
    return names.str();
  }
};

static opt::Descriptor Usage[] = {
  { PREAMBLE, 0, "", "", opt::Arg::None, "Usage information:\n" },
  { DEVICE,
    0,
    "d",
    "device",
    VtkmArg::IsDevice,
    "  --device, -d [dev] \tForce device to dev. Omit device to list available devices." },
  { LOGLEVEL, 0, "v", "", opt::Arg::None, "  -v \tSpecify a log level (when logging is enabled)." },
  { HELP, 0, "h", "help", opt::Arg::None, "  --help, -h \tPrint usage information." },
  { 0, 0, 0, 0, 0, 0 }
};

} // end anon namespace

namespace vtkm
{
namespace cont
{

VTKM_CONT
InitializeResult Initialize(int& argc, char* argv[], InitializeOptions opts)
{
  InitializeResult config;

  // initialize logging first -- it'll pop off the options it consumes:
  if (argc == 0 || argv == nullptr)
  {
    vtkm::cont::InitLogging();
  }
  else
  {
    vtkm::cont::InitLogging(argc, argv);
  }

  { // Parse VTKm options:
    // Remove argv[0] (executable name) if present:
    int vtkmArgc = argc > 0 ? argc - 1 : 0;
    char** vtkmArgv = vtkmArgc > 0 ? argv + 1 : argv;

    opt::Stats stats(Usage, argc, argv);
    std::unique_ptr<opt::Option[]> options{ new opt::Option[stats.options_max] };
    std::unique_ptr<opt::Option[]> buffer{ new opt::Option[stats.buffer_max] };
    opt::Parser parse(Usage, vtkmArgc, vtkmArgv, options.get(), buffer.get());

    if (parse.error())
    {
      opt::printUsage(std::cerr, Usage);
      exit(1);
    }

    if (options[HELP])
    {
      opt::printUsage(std::cout, Usage);
      exit(0);
    }

    if (options[DEVICE])
    {
      auto id = vtkm::cont::make_DeviceAdapterId(options[DEVICE].arg);
      auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
      tracker.ForceDevice(id);
      config.Device = id;
    }
    else if ((opts & InitializeOptions::RequireDevice) != InitializeOptions::None)
    {
      auto devices = VtkmArg::GetValidDeviceNames();
      VTKM_LOG_ALWAYS_S(vtkm::cont::LogLevel::Error,
                        "Target device must be specified via -d or --device.\n"
                        "Valid devices: "
                          << devices
                          << "\n");
      opt::printUsage(std::cerr, Usage);
      exit(1);
    }

    for (int i = 0; i < parse.nonOptionsCount(); i++)
    {
      config.Arguments.emplace_back(parse.nonOption(i));
    }
  }

  return config;
}

VTKM_CONT
InitializeResult Initialize()
{
  vtkm::cont::InitLogging();
  return InitializeResult{};
}
}
} // end namespace vtkm::cont
