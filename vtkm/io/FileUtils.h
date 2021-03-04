//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_FileUtils_h
#define vtk_m_io_FileUtils_h

#include <vtkm/io/vtkm_io_export.h>

#include <string>

namespace vtkm
{
namespace io
{

VTKM_IO_EXPORT bool EndsWith(const std::string& value, const std::string& ending);

} // namespace vtkm::io
} // namespace vtkm

#endif //vtk_m_io_FileUtils_h
