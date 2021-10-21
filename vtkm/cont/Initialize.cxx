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
#include <vtkm/cont/internal/OptionParserArguments.h>

#include <memory>
#include <sstream>

namespace opt = vtkm::cont::internal::option;

namespace
{

struct VtkmDeviceArg : public opt::Arg
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
                            << "'.\nValid devices are: " << VtkmDeviceArg::GetValidDeviceNames()
                            << "\n");
      }
      return opt::ARG_ILLEGAL;
    }

    auto id = vtkm::cont::make_DeviceAdapterId(option.arg);

    if (!VtkmDeviceArg::DeviceIsAvailable(id))
    {
      VTKM_LOG_ALWAYS_S(vtkm::cont::LogLevel::Error,
                        "Unavailable device specificed after option '"
                          << std::string(option.name, static_cast<size_t>(option.namelen)) << "': '"
                          << option.arg << "'.\nValid devices are: "
                          << VtkmDeviceArg::GetValidDeviceNames() << "\n");
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
      if (VtkmDeviceArg::DeviceIsAvailable(id))
      {
        names << "\"" << id.GetName() << "\" ";
      }
    }
    return names.str();
  }
};

} // namespace

namespace vtkm
{
namespace cont
{

VTKM_CONT
InitializeResult Initialize(int& argc, char* argv[], InitializeOptions opts)
{
  InitializeResult config;
  const std::string loggingFlagName = "vtkm-log-level";
  const std::string loggingFlag = "--" + loggingFlagName;
  const std::string loggingHelp = "  " + loggingFlag +
    " <#|INFO|WARNING|ERROR|FATAL|OFF> \tSpecify a log level (when logging is enabled).";

  // initialize logging first -- it'll pop off the options it consumes:
  if (argc == 0 || argv == nullptr)
  {
    vtkm::cont::InitLogging();
  }
  else
  {
    vtkm::cont::InitLogging(argc, argv, loggingFlag);
  }

  { // Parse VTKm options
    std::vector<opt::Descriptor> usage;
    if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
    {
      usage.push_back({ opt::OptionIndex::UNKNOWN,
                        0,
                        "",
                        "",
                        opt::VtkmArg::UnknownOption,
                        "Usage information:\n" });
      usage.push_back({ opt::OptionIndex::HELP,
                        0,
                        "h",
                        "vtkm-help",
                        opt::Arg::None,
                        "  --vtkm-help, -h \tPrint usage information." });
    }
    usage.push_back(
      { opt::OptionIndex::DEVICE,
        0,
        "",
        "vtkm-device",
        VtkmDeviceArg::IsDevice,
        "  --vtkm-device <dev> \tForce device to dev. Omit device to list available devices." });
    usage.push_back({ opt::OptionIndex::LOGLEVEL,
                      0,
                      "",
                      loggingFlagName.c_str(),
                      opt::VtkmArg::Required,
                      loggingHelp.c_str() });

    // TODO: remove deprecated options on next vtk-m release
    usage.push_back({ opt::OptionIndex::DEPRECATED_DEVICE,
                      0,
                      "d",
                      "device",
                      VtkmDeviceArg::IsDevice,
                      "  --device, -d <dev> \tDEPRECATED: use --vtkm-device to set the device" });
    usage.push_back({ opt::OptionIndex::DEPRECATED_LOGLEVEL,
                      0,
                      "v",
                      "",
                      opt::VtkmArg::Required,
                      "  -v <#|INFO|WARNING|ERROR|FATAL|OFF> \tDEPRECATED: use --vtkm-log-level to "
                      "set the log level" });

    // Bring in extra args used by the runtime device configuration options
    vtkm::cont::internal::RuntimeDeviceConfigurationOptions runtimeDeviceOptions(usage);

    // Required to collect unknown arguments when help is off.
    usage.push_back({ opt::OptionIndex::UNKNOWN, 0, "", "", opt::VtkmArg::UnknownOption, "" });
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

    if (options[opt::OptionIndex::HELP])
    {
      std::cerr << config.Usage;
      exit(0);
    }

    // The RuntimeDeviceConfiguration must be completed before calling GetRuntimeDeviceTracker()
    // for all the devices. This is because GetRuntimeDeviceTracker will construct a given
    // device's DeviceAdapterRuntimeDetector to determine if it exists and this constructor may
    // call `GetRuntimeConfiguration` for the specific device in order to query things such as
    // available threads/devices.
    {
      runtimeDeviceOptions.Initialize(options.get());
      vtkm::cont::RuntimeDeviceInformation runtimeDevice;
      runtimeDevice.GetRuntimeConfiguration(
        vtkm::cont::DeviceAdapterTagAny{}, runtimeDeviceOptions, argc, argv);
    }

    if (options[opt::OptionIndex::DEPRECATED_LOGLEVEL])
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Error,
                 "Supplied Deprecated log level flag: "
                   << std::string{ options[opt::OptionIndex::DEPRECATED_LOGLEVEL].name } << ", use "
                   << loggingFlag << " instead.");
#ifdef VTKM_ENABLE_LOGGING
      loguru::g_stderr_verbosity =
        loguru::get_verbosity_from_name(options[opt::OptionIndex::DEPRECATED_LOGLEVEL].arg);
#endif // VTKM_ENABLE_LOGGING
    }

    if (options[opt::OptionIndex::DEVICE] || options[opt::OptionIndex::DEPRECATED_DEVICE])
    {
      const char* arg = nullptr;
      if (options[opt::OptionIndex::DEPRECATED_DEVICE])
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Error,
                   "Supplied Deprecated device flag "
                     << std::string{ options[opt::OptionIndex::DEPRECATED_DEVICE].name }
                     << ", use --vtkm-device instead");
        arg = options[opt::OptionIndex::DEPRECATED_DEVICE].arg;
      }
      if (options[opt::OptionIndex::DEVICE])
      {
        arg = options[opt::OptionIndex::DEVICE].arg;
      }
      auto id = vtkm::cont::make_DeviceAdapterId(arg);
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
      auto devices = VtkmDeviceArg::GetValidDeviceNames();
      VTKM_LOG_S(vtkm::cont::LogLevel::Error, "Device not given on command line.");
      std::cerr << "Target device must be specified via --vtkm-device.\n"
                   "Valid devices: "
                << devices << std::endl;
      if ((opts & InitializeOptions::AddHelp) != InitializeOptions::None)
      {
        std::cerr << config.Usage;
      }
      exit(1);
    }


    for (const opt::Option* opt = options[opt::OptionIndex::UNKNOWN]; opt != nullptr;
         opt = opt->next())
    {
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
      for (const opt::Option* opt = options[opt::OptionIndex::UNKNOWN]; !copyArg && opt != nullptr;
           opt = opt->next())
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
