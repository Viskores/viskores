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
#include <vtkm/cont/Initialize.h>
//// PAUSE-EXAMPLE
#include <vtkm/Version.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

namespace InitExample
{

//// RESUME-EXAMPLE

int main(int argc, char** argv)
{
  vtkm::cont::InitializeOptions options =
    vtkm::cont::InitializeOptions::ErrorOnBadOption |
    vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, options);

  if (argc != 2)
  {
    std::cerr << "USAGE: " << argv[0] << " [options] filename" << std::endl;
    std::cerr << "Available options are:" << std::endl;
    std::cerr << config.Usage << std::endl;
    return 1;
  }
  std::string filename = argv[1];

  // Do something cool with VTK-m
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
static const vtkm::cont::LogLevel CustomLogLevel = vtkm::cont::LogLevel::UserFirst;

int main(int argc, char** argv)
{
  vtkm::cont::SetLogLevelName(CustomLogLevel, "custom");

  // For this example we will set the log level manually.
  // The user can override this with the --vtkm-log-level command line flag.
  vtkm::cont::SetStderrLogLevel(CustomLogLevel);

  vtkm::cont::Initialize(argc, argv);

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
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Info);
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Showcasing function logging");
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
  VTKM_LOG_S(CustomLogLevel,
             "Doing something with type " << vtkm::cont::TypeToString<T>());

  vtkm::Id arraySize = 100000 * sizeof(T);
  VTKM_LOG_S(CustomLogLevel,
             "Size of array is " << vtkm::cont::GetHumanReadableSize(arraySize));
  VTKM_LOG_S(CustomLogLevel,
             "More precisely it is " << vtkm::cont::GetSizeString(arraySize, 4));

  VTKM_LOG_S(CustomLogLevel, "Stack location: " << vtkm::cont::GetStackTrace());
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
  VTKM_LOG_F(vtkm::cont::LogLevel::Info,
             "Base VTK-m version: %d.%d",
             VTKM_VERSION_MAJOR,
             VTKM_VERSION_MINOR);
  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Full VTK-m version: " << VTKM_VERSION_FULL);
  ////
  //// END-EXAMPLE BasicLogging
  ////

  ////
  //// BEGIN-EXAMPLE ConditionalLogging
  ////
  for (vtkm::Id i = 0; i < 5; i++)
  {
    VTKM_LOG_IF_S(vtkm::cont::LogLevel::Info, i % 2 == 0, "Found an even number: " << i);
  }
  ////
  //// END-EXAMPLE ConditionalLogging
  ////

  constexpr vtkm::IdComponent numTrials = 3;

  ////
  //// BEGIN-EXAMPLE ScopedLogging
  ////
  for (vtkm::IdComponent trial = 0; trial < numTrials; ++trial)
  {
    VTKM_LOG_SCOPE(CustomLogLevel, "Trial %d", trial);

    VTKM_LOG_F(CustomLogLevel, "Do thing 1");

    VTKM_LOG_F(CustomLogLevel, "Do thing 2");

    //...
  }
  ////
  //// END-EXAMPLE ScopedLogging
  ////

  TestFunc();

  DoSomething(vtkm::Vec<vtkm::Id3, 3>{});

#if 0
  Error context was removed in VTK-m 2.0 (and was disabled long before then)
  //
  // BEGIN-EXAMPLE LoggingErrorContext
  //
  // This message is only logged if a crash occurs
  VTKM_LOG_ERROR_CONTEXT("Some variable value", 42);
  //
  // END-EXAMPLE LoggingErrorContext
  //
  std::cerr << vtkm::cont::GetLogErrorContext() << "\n";
#endif
}
} // namespace LoggingExample

void Test(int argc, char** argv)
{
  LoggingExample::main(argc, argv);
  LoggingExample::ExampleLogging();

  std::string arg0 = "command-name";
  std::string arg1 = "--vtkm-device=any";
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
