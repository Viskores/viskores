//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_reader_VTKStructuredPointsReader_h
#define vtk_m_io_reader_VTKStructuredPointsReader_h

#include <vtkm/io/reader/VTKDataSetReaderBase.h>

namespace vtkm
{
namespace io
{
namespace reader
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

class VTKStructuredPointsReader : public VTKDataSetReaderBase
{
public:
  explicit VTKStructuredPointsReader(const char* fileName)
    : VTKDataSetReaderBase(fileName)
  {
  }

private:
  virtual void Read()
  {
    if (this->DataFile->Structure != vtkm::io::internal::DATASET_STRUCTURED_POINTS)
    {
      throw vtkm::io::ErrorIO("Incorrect DataSet type");
    }

    std::string tag;

    // Read structured points specific meta-data
    vtkm::Id3 dim;
    vtkm::Vec3f_32 origin, spacing;

    //Two ways the file can describe the dimensions. The proper way is by
    //using the DIMENSIONS keyword, but VisIt written VTK files spicify data
    //bounds instead, as a FIELD
    std::vector<vtkm::Float32> visitBounds;
    this->DataFile->Stream >> tag;
    if (tag == "FIELD")
    {
      this->ReadGlobalFields(&visitBounds);
      this->DataFile->Stream >> tag;
    }
    if (visitBounds.empty())
    {
      internal::parseAssert(tag == "DIMENSIONS");
      this->DataFile->Stream >> dim[0] >> dim[1] >> dim[2] >> std::ws;
      this->DataFile->Stream >> tag;
    }

    internal::parseAssert(tag == "SPACING");
    this->DataFile->Stream >> spacing[0] >> spacing[1] >> spacing[2] >> std::ws;
    if (!visitBounds.empty())
    {
      //now with spacing and physical bounds we can back compute the dimensions
      dim[0] = static_cast<vtkm::Id>((visitBounds[1] - visitBounds[0]) / spacing[0]);
      dim[1] = static_cast<vtkm::Id>((visitBounds[3] - visitBounds[2]) / spacing[1]);
      dim[2] = static_cast<vtkm::Id>((visitBounds[5] - visitBounds[4]) / spacing[2]);
    }

    this->DataFile->Stream >> tag >> origin[0] >> origin[1] >> origin[2] >> std::ws;
    internal::parseAssert(tag == "ORIGIN");

    this->DataSet.SetCellSet(internal::CreateCellSetStructured(dim));
    this->DataSet.AddCoordinateSystem(
      vtkm::cont::CoordinateSystem("coordinates", dim, origin, spacing));

    // Read points and cell attributes
    this->ReadAttributes();
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // namespace vtkm::io:reader

#endif // vtk_m_io_reader_VTKStructuredPointsReader_h
