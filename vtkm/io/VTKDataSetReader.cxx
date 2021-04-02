//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/VTKDataSetReader.h>

#include <vtkm/io/VTKPolyDataReader.h>
#include <vtkm/io/VTKRectilinearGridReader.h>
#include <vtkm/io/VTKStructuredGridReader.h>
#include <vtkm/io/VTKStructuredPointsReader.h>
#include <vtkm/io/VTKUnstructuredGridReader.h>

#include <memory>

namespace vtkm
{
namespace io
{

VTKDataSetReader::VTKDataSetReader(const char* fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKDataSetReader::VTKDataSetReader(const std::string& fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKDataSetReader::~VTKDataSetReader() {}

void VTKDataSetReader::PrintSummary(std::ostream& out) const
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

void VTKDataSetReader::CloseFile()
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

void VTKDataSetReader::Read()
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
}
} // namespace vtkm::io
