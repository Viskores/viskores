//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_BOVDataSetReader_h
#define vtk_m_io_BOVDataSetReader_h

#include <vtkm/cont/DataSet.h>

#include <vtkm/io/vtkm_io_export.h>

namespace vtkm
{
namespace io
{

class VTKM_IO_EXPORT BOVDataSetReader
{
public:
  VTKM_CONT BOVDataSetReader(const char* fileName);
  VTKM_CONT BOVDataSetReader(const std::string& fileName);

  VTKM_CONT const vtkm::cont::DataSet& ReadDataSet();

private:
  VTKM_CONT void LoadFile();

  std::string FileName;
  bool Loaded;
  vtkm::cont::DataSet DataSet;
};
}
} // vtkm::io

#endif // vtk_m_io_BOVReader_h
