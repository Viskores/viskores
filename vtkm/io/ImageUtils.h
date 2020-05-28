//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_io_ImageUtils_h
#define vtk_m_io_ImageUtils_h

#include <vtkm/cont/DataSet.h>
#include <vtkm/io/vtkm_io_export.h>

namespace vtkm
{
namespace io
{

VTKM_IO_EXPORT
void WriteImageFile(const vtkm::cont::DataSet& dataSet,
                    const std::string& fullPath,
                    const std::string& fieldName);

VTKM_IO_EXPORT
vtkm::cont::DataSet ReadImageFile(const std::string& fullPath, const std::string& fieldName);

} // namespace vtkm::io
} // namespace vtkm:

#endif //vtk_m_io_ImageUtils_h
