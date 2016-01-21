//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_io_reader_VTKStructuredPointsReader_h
#define vtk_m_io_reader_VTKStructuredPointsReader_h

#include <vtkm/io/reader/VTKDataSetReaderBase.h>

namespace vtkm {
namespace io {
namespace reader {

class VTKStructuredPointsReader : public VTKDataSetReaderBase
{
public:
  explicit VTKStructuredPointsReader(const char *fileName)
    : VTKDataSetReaderBase(fileName)
  { }

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
    vtkm::Vec<vtkm::Float32, 3> origin, spacing;

    //Two ways the file can describe the dimensions. The proper way is by
    //using the DIMENSIONS keyword, but VisIt written VTK files use a totally
    //different way
    this->DataFile->Stream >> tag;
    if(tag == "DIMENSIONS")
    { //vtk way
      this->DataFile->Stream >> dim[0] >> dim[1] >> dim[2] >> std::ws;
      internal::parseAssert(this->DataFile->Stream.good());

      this->DataFile->Stream >> tag >> spacing[0] >> spacing[1] >> spacing[2] >> std::ws;
      internal::parseAssert(this->DataFile->Stream.good() && tag == "SPACING");
    }
    else
    { //visit way
      std::vector< vtkm::Float32 > bounds(static_cast<std::size_t>(6)); //need have size, not capacity set

      std::string fieldData; int numArrays;
      this->DataFile->Stream >> fieldData >> numArrays >> std::ws;
      internal::parseAssert(this->DataFile->Stream.good() && numArrays > 0);

      for (vtkm::Id i = 0; i < numArrays; ++i)
      {
        std::size_t numTuples;
        vtkm::IdComponent numComponents;
        std::string arrayName, dataType;
        this->DataFile->Stream >> arrayName >> numComponents >> numTuples
                               >> dataType >> std::ws;

        if(arrayName == "avtOriginalBounds")
        {
          internal::parseAssert(numComponents == 1 && numTuples == 6);
          //now we can parse the bounds and fill the bounds vector
          this->ReadArray(bounds);
        }
        else
        {
          this->DoSkipDynamicArray(dataType, numTuples, numComponents);
        }
      }
      internal::parseAssert(this->DataFile->Stream.good());

      //now we need the spacing
      this->DataFile->Stream >> tag >> spacing[0] >> spacing[1] >> spacing[2] >> std::ws;
      internal::parseAssert(this->DataFile->Stream.good() && tag == "SPACING");

      //now with spacing and physical bounds we can back compute the dimensions
      dim[0] = static_cast<vtkm::Id>((bounds[1] - bounds[0]) / spacing[0]);
      dim[1] = static_cast<vtkm::Id>((bounds[3] - bounds[2]) / spacing[1]);
      dim[2] = static_cast<vtkm::Id>((bounds[5] - bounds[4]) / spacing[2]);
    }


    this->DataFile->Stream >> tag >> origin[0] >> origin[1] >> origin[2] >> std::ws;
    internal::parseAssert(this->DataFile->Stream.good() && tag == "ORIGIN");

    vtkm::cont::CellSetStructured<3> cs("cells");
    cs.SetPointDimensions(vtkm::make_Vec(dim[0], dim[1], dim[2]));

    this->DataSet.AddCellSet(cs);
    this->DataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates",
        1, dim, origin, spacing));

    // Read points and cell attributes
    this->ReadAttributes();
  }
};

}
}
} // namespace vtkm::io:reader

#endif // vtk_m_io_reader_VTKStructuredPointsReader_h
