//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/VTKStructuredPointsReader.h>

namespace vtkm
{
namespace io
{

VTKStructuredPointsReader::VTKStructuredPointsReader(const char* fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKStructuredPointsReader::VTKStructuredPointsReader(const std::string& fileName)
  : VTKDataSetReaderBase(fileName)
{
}

void VTKStructuredPointsReader::Read()
{
  if (this->DataFile->Structure != vtkm::io::internal::DATASET_STRUCTURED_POINTS)
  {
    throw vtkm::io::ErrorIO("Incorrect DataSet type");
  }

  // Read structured points specific meta-data
  vtkm::Id3 dim;
  vtkm::Vec3f origin;
  vtkm::Vec3f spacing;

  // The specification for VTK Legacy files says the order of fields should be
  // DIMENSIONS, ORIGIN, SPACING. However, it is common for these to be in
  // different orders. In particular, SPACING is often put before ORIGIN (even
  // in VTK's writer). Also, VisIt writes the DIMENSIONS in a different way.
  // Account for these differences.

  bool readDim = false;
  bool readOrigin = false;
  bool readSpacing = false;
  std::vector<vtkm::Float32> visitBounds;

  while (!(readDim && readOrigin && readSpacing))
  {
    std::string tag;
    this->DataFile->Stream >> tag;
    if (tag == "DIMENSIONS")
    {
      this->DataFile->Stream >> dim[0] >> dim[1] >> dim[2] >> std::ws;
      readDim = true;
    }
    else if (tag == "ORIGIN")
    {
      this->DataFile->Stream >> origin[0] >> origin[1] >> origin[2] >> std::ws;
      readOrigin = true;
    }
    else if (tag == "SPACING")
    {
      this->DataFile->Stream >> spacing[0] >> spacing[1] >> spacing[2] >> std::ws;
      readSpacing = true;
    }
    else if (tag == "FIELD")
    {
      // VisIt adds its own metadata in a FIELD tag.
      this->ReadGlobalFields(&visitBounds);
    }
    else
    {
      std::stringstream message("Parse error: unexpected tag ");
      message << tag;
      throw vtkm::io::ErrorIO(message.str());
    }

    //Two ways the file can describe the dimensions. The proper way is by
    //using the DIMENSIONS keyword, but VisIt written VTK files spicify data
    //bounds instead, as a FIELD
    if (readSpacing && !visitBounds.empty())
    {
      //now with spacing and physical bounds we can back compute the dimensions
      dim[0] = static_cast<vtkm::Id>((visitBounds[1] - visitBounds[0]) / spacing[0]);
      dim[1] = static_cast<vtkm::Id>((visitBounds[3] - visitBounds[2]) / spacing[1]);
      dim[2] = static_cast<vtkm::Id>((visitBounds[5] - visitBounds[4]) / spacing[2]);
      readDim = true;
      visitBounds.clear();
    }
  }

  this->DataSet.SetCellSet(internal::CreateCellSetStructured(dim));
  this->DataSet.AddCoordinateSystem(
    vtkm::cont::CoordinateSystem("coordinates", dim, origin, spacing));

  // Read points and cell attributes
  this->ReadAttributes();
}
}
} // namespace vtkm::io
