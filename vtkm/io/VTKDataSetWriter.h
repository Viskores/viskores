//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_DataSetWriter_h
#define vtk_m_io_DataSetWriter_h

#include <vtkm/cont/DataSet.h>

#include <vtkm/io/vtkm_io_export.h>

namespace vtkm
{
namespace io
{

struct VTKM_IO_EXPORT VTKDataSetWriter
{
public:
  VTKM_CONT VTKDataSetWriter(const char* fileName);
  VTKM_CONT VTKDataSetWriter(const std::string& fileName);

  VTKM_CONT void WriteDataSet(const vtkm::cont::DataSet& dataSet) const;

private:
  std::string FileName;

}; //struct VTKDataSetWriter
}
} //namespace vtkm::io

#endif //vtk_m_io_DataSetWriter_h
