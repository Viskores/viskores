//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Logging.h>

#ifdef VTKM_ENABLE_LOGGING

// disable MSVC warnings in loguru.hpp
#ifdef VTKM_MSVC
#pragma warning(push)
#pragma warning(disable : 4722)
#endif // VTKM_MSVC

#include <vtkm/thirdparty/loguru/vtkmloguru/loguru.cpp>

#ifdef VTKM_MSVC
#pragma warning(pop)
#endif // VTKM_MSVC

#endif // VTKM_ENABLE_LOGGING

#include <cassert>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

namespace
{

// This won't be needed under C++14, as strongly typed enums are automatically
// hashed then. But for now...
struct LogHasher
{
  std::size_t operator()(vtkm::cont::LogLevel level) const
  {
    return static_cast<std::size_t>(level);
  }
};

using LevelMapType = std::unordered_map<vtkm::cont::LogLevel, std::string, LogHasher>;

static bool Initialized = false;
static LevelMapType LogLevelNames;

void setLogLevelName(vtkm::cont::LogLevel level, const std::string& name) noexcept
{
  // if the log has been initialized, prevent modifications of the name map
  // to prevent race conditions.
  if (!Initialized)
  {
    LogLevelNames[level] = name;
  }
}

// Throws std::out_of_range if level not found.
const std::string& getLogLevelName(vtkm::cont::LogLevel level)
{
  const LevelMapType& names = LogLevelNames;
  return names.at(static_cast<vtkm::cont::LogLevel>(level));
}

#ifdef VTKM_ENABLE_LOGGING
const char* verbosityToNameCallback(loguru::Verbosity v)
{
  try
  {
    // Calling c_str on const string&.
    return getLogLevelName(static_cast<vtkm::cont::LogLevel>(v)).c_str();
  }
  catch (std::out_of_range&)
  {
    return nullptr;
  }
}

loguru::Verbosity nameToVerbosityCallback(const char* name)
{
  const LevelMapType& names = LogLevelNames;
  for (auto& kv : names)
  {
    if (kv.second == name)
    {
      return static_cast<loguru::Verbosity>(kv.first);
    }
  }
  return loguru::Verbosity_INVALID;
}
#endif // VTKM_ENABLE_LOGGING

} // end anon namespace

namespace vtkm
{
namespace cont
{

VTKM_CONT
void InitLogging(int& argc, char* argv[])
{
  SetLogLevelName(vtkm::cont::LogLevel::Off, "Off");
  SetLogLevelName(vtkm::cont::LogLevel::Fatal, "FATL");
  SetLogLevelName(vtkm::cont::LogLevel::Error, "ERR");
  SetLogLevelName(vtkm::cont::LogLevel::Warn, "WARN");
  SetLogLevelName(vtkm::cont::LogLevel::Info, "Info");
  SetLogLevelName(vtkm::cont::LogLevel::DevicesEnabled, "Dev");
  SetLogLevelName(vtkm::cont::LogLevel::Perf, "Perf");
  SetLogLevelName(vtkm::cont::LogLevel::MemCont, "MemC");
  SetLogLevelName(vtkm::cont::LogLevel::MemExec, "MemE");
  SetLogLevelName(vtkm::cont::LogLevel::MemTransfer, "MemT");
  SetLogLevelName(vtkm::cont::LogLevel::KernelLaunches, "Kern");
  SetLogLevelName(vtkm::cont::LogLevel::Cast, "Cast");


#ifdef VTKM_ENABLE_LOGGING
  loguru::set_verbosity_to_name_callback(&verbosityToNameCallback);
  loguru::set_name_to_verbosity_callback(&nameToVerbosityCallback);

  // Set the default log level to warning
  SetStderrLogLevel(vtkm::cont::LogLevel::Warn);
  loguru::init(argc, argv);

  LOG_F(INFO, "Logging initialized.");
#else  // VTKM_ENABLE_LOGGING
  (void)argc;
  (void)argv;
#endif // VTKM_ENABLE_LOGGING

  // Prevent LogLevelNames from being modified (makes thread safety easier)
  Initialized = true;
}

void InitLogging()
{
  int argc = 1;
  char dummy[1] = { '\0' };
  char* argv[2] = { dummy, nullptr };
  InitLogging(argc, argv);
}

VTKM_CONT
void SetStderrLogLevel(LogLevel level)
{
#ifdef VTKM_ENABLE_LOGGING
  loguru::g_stderr_verbosity = static_cast<loguru::Verbosity>(level);
#else  // VTKM_ENABLE_LOGGING
  (void)level;
#endif // VTKM_ENABLE_LOGGING
}

VTKM_CONT
vtkm::cont::LogLevel GetStderrLogLevel()
{
#ifdef VTKM_ENABLE_LOGGING
  return static_cast<vtkm::cont::LogLevel>(loguru::g_stderr_verbosity);
#else  // VTKM_ENABLE_LOGGING
  return vtkm::cont::LogLevel::Off;
#endif // VTKM_ENABLE_LOGGING
}

VTKM_CONT
void SetLogThreadName(const std::string& name)
{
#ifdef VTKM_ENABLE_LOGGING
  loguru::set_thread_name(name.c_str());
#else  // VTKM_ENABLE_LOGGING
  (void)name;
#endif // VTKM_ENABLE_LOGGING
}

VTKM_CONT
std::string GetLogThreadName()
{
#ifdef VTKM_ENABLE_LOGGING
  char buffer[128];
  loguru::get_thread_name(buffer, 128, false);
  return buffer;
#else  // VTKM_ENABLE_LOGGING
  return "N/A";
#endif // VTKM_ENABLE_LOGGING
}

VTKM_CONT
std::string GetLogErrorContext()
{
#ifdef VTKM_ENABLE_LOGGING
  auto ctx = loguru::get_error_context();
  return ctx.c_str();
#else  // VTKM_ENABLE_LOGGING
  return "N/A";
#endif // VTKM_ENABLE_LOGGING
}

VTKM_CONT
std::string GetStackTrace(vtkm::Int32 skip)
{
  (void)skip; // unsed when logging disabled.

  std::string result;

#ifdef VTKM_ENABLE_LOGGING
  result = loguru::stacktrace(skip + 2).c_str();
#endif // VTKM_ENABLE_LOGGING

  if (result.empty())
  {
    result = "(Stack trace unavailable)";
  }

  return result;
}


namespace
{
/// Convert a size in bytes to a human readable string (e.g. "64 bytes",
/// "1.44 MiB", "128 GiB", etc). @a prec controls the fixed point precision
/// of the stringified number.
inline VTKM_CONT std::string HumanSize(vtkm::UInt64 bytes, int prec = 2)
{
  vtkm::UInt64 current = bytes;
  vtkm::UInt64 previous = bytes;

  constexpr const char* units[] = { "bytes", "KiB", "MiB", "GiB", "TiB", "PiB" };

  //this way reduces the number of float divisions we do
  int i = 0;
  while (current > 1024)
  {
    previous = current;
    current = current >> 10; //shift up by 1024
    ++i;
  }

  const double bytesf =
    (i == 0) ? static_cast<double>(previous) : static_cast<double>(previous) / 1024.;
  std::ostringstream out;
  out << std::fixed << std::setprecision(prec) << bytesf << " " << units[i];
  return out.str();
}
}

VTKM_CONT
std::string GetHumanReadableSize(vtkm::UInt64 bytes, int prec)
{
  return HumanSize(bytes, prec);
}

VTKM_CONT
std::string GetSizeString(vtkm::UInt64 bytes, int prec)
{
  return HumanSize(bytes, prec) + " (" + std::to_string(bytes) + " bytes)";
}

VTKM_CONT
void SetLogLevelName(LogLevel level, const std::string& name)
{
  if (Initialized)
  {
    VTKM_LOG_F(LogLevel::Error, "SetLogLevelName called after InitLogging.");
    return;
  }
  setLogLevelName(level, name);
}

VTKM_CONT
std::string GetLogLevelName(LogLevel level)
{
#ifdef VTKM_ENABLE_LOGGING
  { // Check loguru lookup first:
    const char* name = loguru::get_verbosity_name(static_cast<loguru::Verbosity>(level));
    if (name)
    {
      return name;
    }
  }
#else
  {
    try
    {
      return getLogLevelName(level);
    }
    catch (std::out_of_range&)
    { /* fallthrough */
    }
  }
#endif

  // Create a string from the numeric value otherwise:
  using T = std::underlying_type<LogLevel>::type;
  return std::to_string(static_cast<T>(level));
}
}
} // end namespace vtkm::cont
