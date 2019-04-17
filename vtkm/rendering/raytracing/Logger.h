//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Loggable_h
#define vtk_m_rendering_raytracing_Loggable_h

#include <sstream>
#include <stack>

#include <vtkm/Types.h>
#include <vtkm/rendering/vtkm_rendering_export.h>

namespace vtkm
{
namespace rendering
{
namespace raytracing
{

class VTKM_RENDERING_EXPORT Logger
{
public:
  ~Logger();
  static Logger* GetInstance();
  void OpenLogEntry(const std::string& entryName);
  void CloseLogEntry(const vtkm::Float64& entryTime);
  void Clear();
  template <typename T>
  void AddLogData(const std::string key, const T& value)
  {
    this->Stream << key << " " << value << "\n";
  }

  std::stringstream& GetStream();

protected:
  Logger();
  Logger(Logger const&);
  std::stringstream Stream;
  static class Logger* Instance;
  std::stack<std::string> Entries;
};
}
}
} // namespace vtkm::rendering::raytracing
#endif
