//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/rendering/raytracing/Logger.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{

Logger* Logger::Instance = nullptr;

Logger::Logger() {}

Logger::~Logger()
{
  Stream.str("");
}

Logger* Logger::GetInstance()
{
  if (Instance == nullptr)
  {
    Instance = new Logger();
  }
  return Instance;
}

std::stringstream& Logger::GetStream()
{
  return Stream;
}

void Logger::Clear()
{
  Stream.str("");
  while (!Entries.empty())
  {
    Entries.pop();
  }
}

void Logger::OpenLogEntry(const std::string& entryName)
{
  Stream << entryName << " "
         << "<\n";
  Entries.push(entryName);
}
void Logger::CloseLogEntry(const viskores::Float64& entryTime)
{
  this->Stream << "total_time " << entryTime << "\n";
  this->Stream << this->Entries.top() << " >\n";
  Entries.pop();
}
}
}
} // namespace viskores::renderng::raytracing
