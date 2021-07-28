//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_VTKPolyDataReader_h
#define vtk_m_io_VTKPolyDataReader_h

#include <vtkm/io/VTKDataSetReaderBase.h>
#include <vtkm/io/internal/VTKDataSetCells.h>

#include <vtkm/cont/ArrayPortalToIterators.h>

#include <iterator>

namespace vtkm
{
namespace io
{

class VTKM_IO_EXPORT VTKPolyDataReader : public VTKDataSetReaderBase
{
public:
  explicit VTKM_CONT VTKPolyDataReader(const char* fileName);
  explicit VTKM_CONT VTKPolyDataReader(const std::string& fileName);

private:
  void Read() override;
};
}
} // namespace vtkm::io

#endif // vtk_m_io_VTKPolyDataReader_h
