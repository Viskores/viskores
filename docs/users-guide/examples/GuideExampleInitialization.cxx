//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

////
//// BEGIN-EXAMPLE BasicInitialize
////
#include <viskores/cont/Initialize.h>
//// PAUSE-EXAMPLE
#include <viskores/Version.h>
#include <viskores/cont/Logging.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

namespace InitExample
{

//// RESUME-EXAMPLE

int main(int argc, char** argv)
{
  viskores::cont::InitializeOptions options =
    viskores::cont::InitializeOptions::ErrorOnBadOption |
    viskores::cont::InitializeOptions::DefaultAnyDevice;
  viskores::cont::InitializeResult config =
    viskores::cont::Initialize(argc, argv, options);

  if (argc != 2)
  {
    std::cerr << "USAGE: " << argv[0] << " [options] filename" << std::endl;
    std::cerr << "Available options are:" << std::endl;
    std::cerr << config.Usage << std::endl;
    return 1;
  }
  std::string filename = argv[1];

  // Do something cool with Viskores
  // ...

  return 0;
}
////
//// END-EXAMPLE BasicInitialize
////

} // namespace InitExample

namespace LoggingExample
{

////
//// BEGIN-EXAMPLE InitializeLogging
////
static const viskores::cont::LogLevel CustomLogLevel =
  viskores::cont::LogLevel::UserFirst;

int main(int argc, char** argv)
{
  viskores::cont::SetLogLevelName(CustomLogLevel, "custom");

  // For this example we will set the log level manually.
  // The user can override this with the --viskores-log-level command line flag.
  viskores::cont::SetStderrLogLevel(CustomLogLevel);

  viskores::cont::Initialize(argc, argv);

  // Do interesting stuff...
  ////
  //// END-EXAMPLE InitializeLogging
  ////

  return 0;
}

////
//// BEGIN-EXAMPLE ScopedFunctionLogging
////
void TestFunc()
{
  VISKORES_LOG_SCOPE_FUNCTION(viskores::cont::LogLevel::Info);
  VISKORES_LOG_S(viskores::cont::LogLevel::Info, "Showcasing function logging");
}
////
//// END-EXAMPLE ScopedFunctionLogging
////

////
//// BEGIN-EXAMPLE HelperLogFunctions
////
template<typename T>
void DoSomething(T&& x)
{
  VISKORES_LOG_S(CustomLogLevel,
                 "Doing something with type " << viskores::cont::TypeToString<T>());

  viskores::Id arraySize = 100000 * sizeof(T);
  VISKORES_LOG_S(CustomLogLevel,
                 "Size of array is " << viskores::cont::GetHumanReadableSize(arraySize));
  VISKORES_LOG_S(CustomLogLevel,
                 "More precisely it is " << viskores::cont::GetSizeString(arraySize, 4));

  VISKORES_LOG_S(CustomLogLevel, "Stack location: " << viskores::cont::GetStackTrace());
  ////
  //// END-EXAMPLE HelperLogFunctions
  ////

  (void)x;
}

void ExampleLogging()
{
  ////
  //// BEGIN-EXAMPLE BasicLogging
  ////
  VISKORES_LOG_F(viskores::cont::LogLevel::Info,
                 "Base Viskores version: %d.%d",
                 VISKORES_VERSION_MAJOR,
                 VISKORES_VERSION_MINOR);
  VISKORES_LOG_S(viskores::cont::LogLevel::Info,
                 "Full Viskores version: " << VISKORES_VERSION_FULL);
  ////
  //// END-EXAMPLE BasicLogging
  ////

  ////
  //// BEGIN-EXAMPLE ConditionalLogging
  ////
  for (viskores::Id i = 0; i < 5; i++)
  {
    VISKORES_LOG_IF_S(
      viskores::cont::LogLevel::Info, i % 2 == 0, "Found an even number: " << i);
  }
  ////
  //// END-EXAMPLE ConditionalLogging
  ////

  constexpr viskores::IdComponent numTrials = 3;

  ////
  //// BEGIN-EXAMPLE ScopedLogging
  ////
  for (viskores::IdComponent trial = 0; trial < numTrials; ++trial)
  {
    VISKORES_LOG_SCOPE(CustomLogLevel, "Trial %d", trial);

    VISKORES_LOG_F(CustomLogLevel, "Do thing 1");

    VISKORES_LOG_F(CustomLogLevel, "Do thing 2");

    //...
  }
  ////
  //// END-EXAMPLE ScopedLogging
  ////

  TestFunc();

  DoSomething(viskores::Vec<viskores::Id3, 3>{});

#if 0
  Error context was removed in Viskores 2.0 (and was disabled long before then)
  //
  // BEGIN-EXAMPLE LoggingErrorContext
  //
  // This message is only logged if a crash occurs
  VISKORES_LOG_ERROR_CONTEXT("Some variable value", 42);
  //
  // END-EXAMPLE LoggingErrorContext
  //
  std::cerr << viskores::cont::GetLogErrorContext() << "\n";
#endif
}
} // namespace LoggingExample

void Test(int argc, char** argv)
{
  LoggingExample::main(argc, argv);
  LoggingExample::ExampleLogging();

  std::string arg0 = "command-name";
  std::string arg1 = "--viskores-device=any";
  std::string arg2 = "filename";
  std::vector<char*> fakeArgv;
  fakeArgv.push_back(const_cast<char*>(arg0.c_str()));
  fakeArgv.push_back(const_cast<char*>(arg1.c_str()));
  fakeArgv.push_back(const_cast<char*>(arg2.c_str()));
  InitExample::main(3, &fakeArgv.front());
}

} // anonymous namespace

int GuideExampleInitialization(int argc, char* argv[])
{
  // Do not use standard testing run because that also calls Initialize
  // and will foul up the other calls.
  try
  {
    Test(argc, argv);
  }
  catch (...)
  {
    std::cerr << "Uncaught exception" << std::endl;
    return 1;
  }

  return 0;
}
