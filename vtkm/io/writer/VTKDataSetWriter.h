//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_writer_DataSetWriter_h
#define vtk_m_io_writer_DataSetWriter_h

#include <vtkm/Deprecated.h>
#include <vtkm/io/VTKDataSetWriter.h>

namespace vtkm
{
namespace io
{
namespace writer
{

struct VTKM_DEPRECATED(1.6, "Please use vtkm::io::VTKDataSetWriter") VTKDataSetWriter
  : vtkm::io::VTKDataSetWriter
{
public:
  VTKDataSetWriter(const std::string& filename)
    : vtkm::io::VTKDataSetWriter(filename)
  {
  }
};
}
}
} //namespace vtkm::io::writer

#endif //vtk_m_io_writer_DataSetWriter_h
