//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_reader_BOVDataSetReader_h
#define vtk_m_io_reader_BOVDataSetReader_h

#include <vtkm/Deprecated.h>
#include <vtkm/io/BOVDataSetReader.h>

namespace vtkm
{
namespace io
{
namespace reader
{

class VTKM_DEPRECATED(1.6, "Please use vtkm::io::BOVDataSetReader.") BOVDataSetReader
  : public io::BOVDataSetReader
{
public:
  BOVDataSetReader(const char* fileName)
    : io::BOVDataSetReader(fileName)
  {
  }
  BOVDataSetReader(const std::string& fileName)
    : io::BOVDataSetReader(fileName)
  {
  }
};
}
}
} // vtkm::io::reader

#endif // vtk_m_io_reader_BOVReader_h
