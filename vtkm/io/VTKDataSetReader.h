//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_VTKDataSetReader_h
#define vtk_m_io_VTKDataSetReader_h

#include <vtkm/io/VTKDataSetReaderBase.h>

namespace vtkm
{
namespace io
{

/// @brief Reads a legacy VTK file.
///
/// By convention, legacy VTK files have a `.vtk` extension.
/// This class should be constructed with a filename, and the data
/// read with `ReadDataSet`.
class VTKM_IO_EXPORT VTKDataSetReader : public VTKDataSetReaderBase
{
public:
  VTKM_CONT VTKDataSetReader(const char* fileName);
  /// @brief Construct a reader to load data from the given file.
  VTKM_CONT VTKDataSetReader(const std::string& fileName);
  VTKM_CONT ~VTKDataSetReader() override;

  VTKDataSetReader(const VTKDataSetReader&) = delete;
  void operator=(const VTKDataSetReader&) = delete;

  VTKM_CONT void PrintSummary(std::ostream& out) const override;

private:
  VTKM_CONT void CloseFile() override;
  VTKM_CONT void Read() override;

  std::unique_ptr<VTKDataSetReaderBase> Reader;
};

}
} // vtkm::io

#endif // vtk_m_io_VTKReader_h
