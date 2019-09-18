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

#include <vtkm/io/reader/VTKDataSetReaderBase.h>
#include <vtkm/io/reader/VTKPolyDataReader.h>
#include <vtkm/io/reader/VTKRectilinearGridReader.h>
#include <vtkm/io/reader/VTKStructuredGridReader.h>
#include <vtkm/io/reader/VTKStructuredPointsReader.h>
#include <vtkm/io/reader/VTKUnstructuredGridReader.h>

#include <memory>

namespace vtkm
{
namespace io
{
namespace reader
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

class VTKDataSetReader : public VTKDataSetReaderBase
{
public:
  explicit VTKDataSetReader(const char* fileName)
    : VTKDataSetReaderBase(fileName)
  {
  }

  explicit VTKDataSetReader(const std::string& fileName)
    : VTKDataSetReaderBase(fileName)
  {
  }

  virtual void PrintSummary(std::ostream& out) const
  {
    if (this->Reader)
    {
      this->Reader->PrintSummary(out);
    }
    else
    {
      VTKDataSetReaderBase::PrintSummary(out);
    }
  }

private:
  virtual void CloseFile()
  {
    if (this->Reader)
    {
      this->Reader->CloseFile();
    }
    else
    {
      VTKDataSetReaderBase::CloseFile();
    }
  }

  virtual void Read()
  {
    switch (this->DataFile->Structure)
    {
      case vtkm::io::internal::DATASET_STRUCTURED_POINTS:
        this->Reader.reset(new VTKStructuredPointsReader(""));
        break;
      case vtkm::io::internal::DATASET_STRUCTURED_GRID:
        this->Reader.reset(new VTKStructuredGridReader(""));
        break;
      case vtkm::io::internal::DATASET_RECTILINEAR_GRID:
        this->Reader.reset(new VTKRectilinearGridReader(""));
        break;
      case vtkm::io::internal::DATASET_POLYDATA:
        this->Reader.reset(new VTKPolyDataReader(""));
        break;
      case vtkm::io::internal::DATASET_UNSTRUCTURED_GRID:
        this->Reader.reset(new VTKUnstructuredGridReader(""));
        break;
      default:
        throw vtkm::io::ErrorIO("Unsupported DataSet type.");
    }

    this->TransferDataFile(*this->Reader.get());
    this->Reader->Read();
    this->DataSet = this->Reader->GetDataSet();
  }

  std::unique_ptr<VTKDataSetReaderBase> Reader;
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // vtkm::io::reader

#endif // vtk_m_io_reader_VTKReader_h
