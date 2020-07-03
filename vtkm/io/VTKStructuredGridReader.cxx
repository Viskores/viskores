//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/VTKStructuredGridReader.h>

namespace vtkm
{
namespace io
{

VTKStructuredGridReader::VTKStructuredGridReader(const char* fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKStructuredGridReader::VTKStructuredGridReader(const std::string& fileName)
  : VTKDataSetReaderBase(fileName)
{
}

void VTKStructuredGridReader::Read()
{
  if (this->DataFile->Structure != vtkm::io::internal::DATASET_STRUCTURED_GRID)
  {
    throw vtkm::io::ErrorIO("Incorrect DataSet type");
  }

  std::string tag;

  //We need to be able to handle VisIt files which dump Field data
  //at the top of a VTK file
  this->DataFile->Stream >> tag;
  if (tag == "FIELD")
  {
    this->ReadGlobalFields();
    this->DataFile->Stream >> tag;
  }

  // Read structured grid specific meta-data
  internal::parseAssert(tag == "DIMENSIONS");
  vtkm::Id3 dim;
  this->DataFile->Stream >> dim[0] >> dim[1] >> dim[2] >> std::ws;

  this->DataSet.SetCellSet(internal::CreateCellSetStructured(dim));

  // Read the points
  this->DataFile->Stream >> tag;
  internal::parseAssert(tag == "POINTS");
  this->ReadPoints();

  // Read points and cell attributes
  this->ReadAttributes();
}
}
} // namespace vtkm::io
