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

#include <vtkm/cont/Logging.h>

#ifdef VTKM_ENABLE_LOGGING

// disable MSVC warnings in loguru.hpp
#ifdef VTKM_MSVC
#pragma warning(push)
#pragma warning(disable : 4722)
#endif // VTKM_MSVC

#define LOGURU_IMPLEMENTATION 1
#include <vtkm/thirdparty/loguru/vtkmloguru/loguru.hpp>

#ifdef VTKM_MSVC
#pragma warning(pop)
#endif // VTKM_MSVC

#endif // VTKM_ENABLE_LOGGING

#include <vtkm/testing/Testing.h> // for HumanSize

#include <cassert>
#include <unordered_map>

#ifdef VTKM_ENABLE_LOGGING
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

void setLogLevelName(vtkm::cont::LogLevel level, const std::string& name)
{
  // if the log has been initialized, prevent modifications of the name map
  // to prevent race conditions.
  if (!Initialized)
  {
    LogLevelNames[level] = name;
  }
}

const char* verbosityToNameCallback(loguru::Verbosity verbosity)
{
  const LevelMapType& names = LogLevelNames;
  auto name = names.find(static_cast<vtkm::cont::LogLevel>(verbosity));
  return name != names.end() ? name->second.c_str() : nullptr;
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

} // end anon namespace
#endif // VTKM_ENABLE_LOGGING

namespace vtkm
{
namespace cont
{

VTKM_CONT
void InitLogging(int& argc, char* argv[])
{
#ifdef VTKM_ENABLE_LOGGING
  SetLogLevelName(vtkm::cont::LogLevel::Off, "Off");
  SetLogLevelName(vtkm::cont::LogLevel::Fatal, "FATL");
  SetLogLevelName(vtkm::cont::LogLevel::Error, "ERR");
  SetLogLevelName(vtkm::cont::LogLevel::Warn, "WARN");
  SetLogLevelName(vtkm::cont::LogLevel::Info, "Info");
  SetLogLevelName(vtkm::cont::LogLevel::Perf, "Perf");
  SetLogLevelName(vtkm::cont::LogLevel::MemCont, "MemC");
  SetLogLevelName(vtkm::cont::LogLevel::MemExec, "MemE");
  SetLogLevelName(vtkm::cont::LogLevel::MemTransfer, "MemT");
  SetLogLevelName(vtkm::cont::LogLevel::Cast, "Cast");

  loguru::set_verbosity_to_name_callback(&verbosityToNameCallback);
  loguru::set_name_to_verbosity_callback(&nameToVerbosityCallback);

  loguru::init(argc, argv);

  // Prevent LogLevelNames from being modified (makes thread safety easier)
  Initialized = true;

  LOG_F(INFO, "Logging initialized.");
#else  // VTKM_ENABLE_LOGGING
  (void)argc;
  (void)argv;
#endif // VTKM_ENABLE_LOGGING
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
#ifdef VTKM_ENABLE_LOGGING
  if (Initialized)
  {
    VTKM_LOG_F(LogLevel::Error, "SetLogLevelName called after InitLogging.");
    return;
  }
  setLogLevelName(level, name);
#else  // VTKM_ENABLE_LOGGING
  (void)level;
  (void)name;
#endif // VTKM_ENABLE_LOGGING
}
}
} // end namespace vtkm::cont
