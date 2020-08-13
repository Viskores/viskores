//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Logger_h
#define vtk_m_filter_Logger_h

#include <iostream>
#include <map>
#include <string>
#include <vtkm/filter/particleadvection/Logger.h>

namespace vtkm
{
namespace filter
{

class Logger
{
public:
  static Logger* GetInstance(const std::string& name)
  {
    if (Logger::Loggers.find(name) == Logger::Loggers.end())
      Logger::Loggers[name] = new vtkm::filter::Logger(name);
    return Logger::Loggers[name];
  }

  ~Logger();

  std::ofstream& GetStream() { return Stream; }

protected:
  Logger(const std::string& name);

  std::ofstream Stream;
  static std::map<std::string, Logger*> Loggers;
};
}
}; // namespace vtkm::filter

#endif
