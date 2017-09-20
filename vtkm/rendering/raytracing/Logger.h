//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
