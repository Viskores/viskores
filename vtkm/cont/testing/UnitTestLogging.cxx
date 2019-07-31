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
#include <vtkm/cont/testing/Testing.h>

#include <chrono>
#include <thread>

namespace
{

void DoWork()
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Info);
  VTKM_LOG_F(vtkm::cont::LogLevel::Info, "Sleeping for half a second...");
  std::this_thread::sleep_for(std::chrono::milliseconds{ 500 });
}

void Scopes(int level = 0)
{
  VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Info, "Called Scope (level=%d)", level);

  DoWork();

  VTKM_LOG_IF_F(vtkm::cont::LogLevel::Info,
                level % 2 != 0,
                "Printing extra log message because level is odd (%d)",
                level);
  if (level < 5)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Recursing to level " << level + 1);
    Scopes(level + 1);
  }
  else
  {
    VTKM_LOG_F(vtkm::cont::LogLevel::Warn, "Reached limit for Scopes test recursion.");
  }
}

void ErrorContext()
{
  // These variables are only logged if a crash occurs.
  // Only supports POD by default, but can be extended (see loguru docs)
  VTKM_LOG_ERROR_CONTEXT("Some Int", 3);
  VTKM_LOG_ERROR_CONTEXT("A Double", 236.7521);
  VTKM_LOG_ERROR_CONTEXT("A C-String", "Hiya!");

  // The error-tracking should work automatically on linux (maybe mac?) but on
  // windows it doesn't trigger automatically (see loguru #74). But we can
  // manually dump the error context log like so:
  std::cerr << vtkm::cont::GetLogErrorContext() << "\n";
}

void UserDefined()
{
  VTKM_DEFINE_USER_LOG_LEVEL(CustomLevel, 0);
  VTKM_DEFINE_USER_LOG_LEVEL(CustomLevel2, 2);
  VTKM_DEFINE_USER_LOG_LEVEL(AnotherCustomLevel2, 2);
  VTKM_DEFINE_USER_LOG_LEVEL(BigLevel, 300);

  vtkm::cont::SetStderrLogLevel(vtkm::cont::LogLevel::UserLast);
  VTKM_LOG_S(CustomLevel, "CustomLevel");
  VTKM_LOG_S(CustomLevel2, "CustomLevel2");
  VTKM_LOG_S(AnotherCustomLevel2, "AnotherCustomLevel2");

  vtkm::cont::SetStderrLogLevel(vtkm::cont::LogLevel::UserFirst);
  VTKM_LOG_S(BigLevel, "BigLevel"); // should log nothing

  vtkm::cont::SetStderrLogLevel(vtkm::cont::LogLevel::UserLast);
  VTKM_LOG_S(BigLevel, "BigLevel");
}

void RunTests()
{
  VTKM_LOG_F(vtkm::cont::LogLevel::Info, "Running tests.");

  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Running Scopes test...");
  Scopes();

  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Running ErrorContext test...");
  ErrorContext();

  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Running UserDefined test...");
  UserDefined();
}

} // end anon namespace

int UnitTestLogging(int, char* [])
{
  // Test that parameterless init works:
  vtkm::cont::InitLogging();

  RunTests();
  return 0;
}
