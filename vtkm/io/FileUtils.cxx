//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/io/FileUtils.h>

#include <algorithm>

namespace vtkm
{
namespace io
{

bool EndsWith(const std::string& value, const std::string& ending)
{
  if (ending.size() > value.size())
  {
    return false;
  }
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

} // namespace vtkm::io
} // namespace vtkm
