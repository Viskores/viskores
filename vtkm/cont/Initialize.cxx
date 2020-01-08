//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  UNKNOWN,
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

  static opt::ArgStatus Required(const opt::Option& option, bool msg)
  {
    if (option.arg == nullptr)
    {
      if (msg)
      {
        VTKM_LOG_ALWAYS_S(vtkm::cont::LogLevel::Error,
                          "Missing argument after option '"
                            << std::string(option.name, static_cast<size_t>(option.namelen))
                            << "'.\n");
      }
      return opt::ARG_ILLEGAL;
    }
    else
    {
      return opt::ARG_OK;
    }
  }

  // Method used for guessing whether an option that do not support (perhaps that calling
  // program knows about it) has an option attached to it (which should also be ignored).
  static opt::ArgStatus UnknownOption(const opt::Option& option, bool msg)
  {
    // If we don't have an arg, obviously we don't have an arg.
    if (option.arg == nullptr)
    {
      return opt::ARG_NONE;
    }

    // The opt::Arg::Optional method will return that the ARG is OK if and only if
    // the argument is attached to the option (e.g. --foo=bar). If that is the case,
    // then we definitely want to report that the argument is OK.
    if (opt::Arg::Optional(option, msg) == opt::ARG_OK)
    {
      return opt::ARG_OK;
    }

    // Now things get tricky. Maybe the next argument is an option or maybe it is an
    // argument for this option. We will guess that if the next argument does not
    // look like an option, we will treat it as such.
    if (option.arg[0] == '-')
    {
      return opt::ARG_NONE;
    }
    else
    {
      return opt::ARG_OK;
    }
  }
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

  { // Parse VTKm options
    std::vector<opt::Descriptor> usage;
    if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
    {
      usage.push_back({ UNKNOWN, 0, "", "", VtkmArg::UnknownOption, "Usage information:\n" });
    }
    usage.push_back(
      { DEVICE,
        0,
        "d",
        "device",
        VtkmArg::IsDevice,
        "  --device, -d <dev> \tForce device to dev. Omit device to list available devices." });
    usage.push_back(
      { LOGLEVEL,
        0,
        "v",
        "",
        VtkmArg::Required,
        "  -v <#|INFO|WARNING|ERROR|FATAL|OFF> \tSpecify a log level (when logging is enabled)." });
    if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
    {
      usage.push_back(
        { HELP, 0, "h", "help", opt::Arg::None, "  --help, -h \tPrint usage information." });
    }
    // Required to collect unknown arguments when help is off.
    usage.push_back({ UNKNOWN, 0, "", "", VtkmArg::UnknownOption, "" });
    usage.push_back({ 0, 0, 0, 0, 0, 0 });

    {
      std::stringstream streamBuffer;
      opt::printUsage(streamBuffer, usage.data());
      config.Usage = streamBuffer.str();
      // Remove trailing newline as one more than we want is added.
      config.Usage = config.Usage.substr(0, config.Usage.length() - 1);
    }

    // Remove argv[0] (executable name) if present:
    int vtkmArgc = argc > 0 ? argc - 1 : 0;
    char** vtkmArgv = vtkmArgc > 0 ? argv + 1 : argv;

    opt::Stats stats(usage.data(), vtkmArgc, vtkmArgv);
    std::unique_ptr<opt::Option[]> options{ new opt::Option[stats.options_max] };
    std::unique_ptr<opt::Option[]> buffer{ new opt::Option[stats.buffer_max] };
    opt::Parser parse(usage.data(), vtkmArgc, vtkmArgv, options.get(), buffer.get());

    if (parse.error())
    {
      std::cerr << config.Usage;
      exit(1);
    }

    if (options[HELP])
    {
      std::cerr << config.Usage;
      exit(0);
    }

    if (options[DEVICE])
    {
      auto id = vtkm::cont::make_DeviceAdapterId(options[DEVICE].arg);
      if (id != vtkm::cont::DeviceAdapterTagAny{})
      {
        vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(id);
      }
      else
      {
        vtkm::cont::GetRuntimeDeviceTracker().Reset();
      }
      config.Device = id;
    }
    else if ((opts & InitializeOptions::DefaultAnyDevice) != InitializeOptions::None)
    {
      vtkm::cont::GetRuntimeDeviceTracker().Reset();
      config.Device = vtkm::cont::DeviceAdapterTagAny{};
    }
    else if ((opts & InitializeOptions::RequireDevice) != InitializeOptions::None)
    {
      auto devices = VtkmArg::GetValidDeviceNames();
      VTKM_LOG_S(vtkm::cont::LogLevel::Error, "Device not given on command line.");
      std::cerr << "Target device must be specified via -d or --device.\n"
                   "Valid devices: "
                << devices << std::endl;
      if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
      {
        std::cerr << config.Usage;
      }
      exit(1);
    }

    for (const opt::Option* opt = options[UNKNOWN]; opt != nullptr; opt = opt->next())
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Unknown option to Initialize: " << opt->name << "\n");
      if ((opts & InitializeOptions::ErrorOnBadOption) != InitializeOptions::None)
      {
        std::cerr << "Unknown option: " << opt->name << std::endl;
        if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
        {
          std::cerr << config.Usage;
        }
        exit(1);
      }
    }

    for (int nonOpt = 0; nonOpt < parse.nonOptionsCount(); ++nonOpt)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "Unknown argument to Initialize: " << parse.nonOption(nonOpt) << "\n");
      if ((opts & InitializeOptions::ErrorOnBadArgument) != InitializeOptions::None)
      {
        std::cerr << "Unknown argument: " << parse.nonOption(nonOpt) << std::endl;
        if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
        {
          std::cerr << config.Usage;
        }
        exit(1);
      }
    }

    // Now go back through the arg list and remove anything that is not in the list of
    // unknown options or non-option arguments.
    int destArg = 1;
    for (int srcArg = 1; srcArg < argc; ++srcArg)
    {
      std::string thisArg{ argv[srcArg] };
      bool copyArg = false;

      // Special case: "--" gets removed by optionparser but should be passed.
      if (thisArg == "--")
      {
        copyArg = true;
      }
      for (const opt::Option* opt = options[UNKNOWN]; !copyArg && opt != nullptr; opt = opt->next())
      {
        if (thisArg == opt->name)
        {
          copyArg = true;
        }
        if ((opt->arg != nullptr) && (thisArg == opt->arg))
        {
          copyArg = true;
        }
        // Special case: optionparser sometimes removes a single "-" from an option
        if (thisArg.substr(1) == opt->name)
        {
          copyArg = true;
        }
      }
      for (int nonOpt = 0; !copyArg && nonOpt < parse.nonOptionsCount(); ++nonOpt)
      {
        if (thisArg == parse.nonOption(nonOpt))
        {
          copyArg = true;
        }
      }
      if (copyArg)
      {
        if (destArg != srcArg)
        {
          argv[destArg] = argv[srcArg];
        }
        ++destArg;
      }
    }
    argc = destArg;
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
