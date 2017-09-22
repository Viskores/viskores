//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_io_reader_VTKRectilinearGridReader_h
#define vtk_m_io_reader_VTKRectilinearGridReader_h

#include <vtkm/io/reader/VTKDataSetReaderBase.h>

namespace vtkm
{
namespace io
{
namespace reader
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

class VTKRectilinearGridReader : public VTKDataSetReaderBase
{
public:
  explicit VTKRectilinearGridReader(const char* fileName)
    : VTKDataSetReaderBase(fileName)
  {
  }

private:
  virtual void Read()
  {
    if (this->DataFile->Structure != vtkm::io::internal::DATASET_RECTILINEAR_GRID)
      throw vtkm::io::ErrorIO("Incorrect DataSet type");

    //We need to be able to handle VisIt files which dump Field data
    //at the top of a VTK file
    std::string tag;
    this->DataFile->Stream >> tag;
    if (tag == "FIELD")
    {
      std::string name;
      this->ReadFields(name);
      this->DataFile->Stream >> tag;
    }

    // Read structured grid specific meta-data
    internal::parseAssert(tag == "DIMENSIONS");
    vtkm::Id3 dim;
    this->DataFile->Stream >> dim[0] >> dim[1] >> dim[2] >> std::ws;

    //Read the points.
    std::string dataType;
    std::size_t numPoints[3];
    vtkm::cont::DynamicArrayHandle X, Y, Z;

    // Always read coordinates as vtkm::FloatDefault
    std::string readDataType = vtkm::io::internal::DataTypeName<vtkm::FloatDefault>::Name();

    this->DataFile->Stream >> tag >> numPoints[0] >> dataType >> std::ws;
    if (tag != "X_COORDINATES")
      throw vtkm::io::ErrorIO("X_COORDINATES tag not found");
    this->DoReadDynamicArray(readDataType, numPoints[0], 1, X);

    this->DataFile->Stream >> tag >> numPoints[1] >> dataType >> std::ws;
    if (tag != "Y_COORDINATES")
      throw vtkm::io::ErrorIO("Y_COORDINATES tag not found");
    this->DoReadDynamicArray(readDataType, numPoints[1], 1, Y);

    this->DataFile->Stream >> tag >> numPoints[2] >> dataType >> std::ws;
    if (tag != "Z_COORDINATES")
      throw vtkm::io::ErrorIO("Z_COORDINATES tag not found");
    this->DoReadDynamicArray(readDataType, numPoints[2], 1, Z);

    if (dim != vtkm::Id3(static_cast<vtkm::Id>(numPoints[0]),
                         static_cast<vtkm::Id>(numPoints[1]),
                         static_cast<vtkm::Id>(numPoints[2])))
      throw vtkm::io::ErrorIO("DIMENSIONS not equal to number of points");

    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>
      coords;

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> Xc, Yc, Zc;
    X.CopyTo(Xc);
    Y.CopyTo(Yc);
    Z.CopyTo(Zc);
    coords = vtkm::cont::make_ArrayHandleCartesianProduct(Xc, Yc, Zc);
    vtkm::cont::CoordinateSystem coordSys("coordinates", coords);
    this->DataSet.AddCoordinateSystem(coordSys);

    this->DataSet.AddCellSet(internal::CreateCellSetStructured(dim));

    // Read points and cell attributes
    this->ReadAttributes();
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // namespace vtkm::io:reader

#endif // vtk_m_io_reader_VTKRectilinearGridReader_h
