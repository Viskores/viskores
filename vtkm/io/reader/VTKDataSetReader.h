//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_reader_VTKDataSetReader_h
#define vtk_m_io_reader_VTKDataSetReader_h

#include <vtkm/Deprecated.h>
#include <vtkm/io/VTKDataSetReader.h>

namespace vtkm
{
namespace io
{
namespace reader
{

class VTKM_DEPRECATED(1.6, "Please use vtkm::io::VTKDataSetReader.") VTKDataSetReader
  : public io::VTKDataSetReader
{
public:
  explicit VTKDataSetReader(const char* fileName)
    : io::VTKDataSetReader(fileName)
  {
  }

  explicit VTKDataSetReader(const std::string& fileName)
    : io::VTKDataSetReader(fileName)
  {
  }
};
}
}
} // vtkm::io::reader

#endif // vtk_m_io_reader_VTKReader_h
