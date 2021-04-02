//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/io/VTKRectilinearGridReader.h>

#include <vtkm/cont/ArrayCopy.h>

namespace vtkm
{
namespace io
{

VTKRectilinearGridReader::VTKRectilinearGridReader(const char* fileName)
  : VTKDataSetReaderBase(fileName)
{
}

VTKRectilinearGridReader::VTKRectilinearGridReader(const std::string& fileName)
  : VTKDataSetReaderBase(fileName)
{
}

void VTKRectilinearGridReader::Read()
{
  if (this->DataFile->Structure != vtkm::io::internal::DATASET_RECTILINEAR_GRID)
    throw vtkm::io::ErrorIO("Incorrect DataSet type");

  //We need to be able to handle VisIt files which dump Field data
  //at the top of a VTK file
  std::string tag;
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

  //Read the points.
  std::string fileStorageDataType;
  std::size_t numPoints[3];
  vtkm::cont::UnknownArrayHandle X, Y, Z;

  this->DataFile->Stream >> tag >> numPoints[0] >> fileStorageDataType >> std::ws;
  if (tag != "X_COORDINATES")
    throw vtkm::io::ErrorIO("X_COORDINATES tag not found");

  // In binary mode, we must read the data as they are stored in the file.
  // In text mode we can parse as FloatDefault no matter the precision of the storage.
  X = this->DoReadArrayVariant(
    vtkm::cont::Field::Association::ANY, fileStorageDataType, numPoints[0], 1);


  this->DataFile->Stream >> tag >> numPoints[1] >> fileStorageDataType >> std::ws;
  if (tag != "Y_COORDINATES")
    throw vtkm::io::ErrorIO("Y_COORDINATES tag not found");

  Y = this->DoReadArrayVariant(
    vtkm::cont::Field::Association::ANY, fileStorageDataType, numPoints[1], 1);

  this->DataFile->Stream >> tag >> numPoints[2] >> fileStorageDataType >> std::ws;
  if (tag != "Z_COORDINATES")
    throw vtkm::io::ErrorIO("Z_COORDINATES tag not found");

  Z = this->DoReadArrayVariant(
    vtkm::cont::Field::Association::ANY, fileStorageDataType, numPoints[2], 1);


  if (dim !=
      vtkm::Id3(static_cast<vtkm::Id>(numPoints[0]),
                static_cast<vtkm::Id>(numPoints[1]),
                static_cast<vtkm::Id>(numPoints[2])))
    throw vtkm::io::ErrorIO("DIMENSIONS not equal to number of points");

  vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                          vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                          vtkm::cont::ArrayHandle<vtkm::FloatDefault>>
    coords;

  // We need to store all coordinate arrays as FloatDefault.
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> Xc, Yc, Zc;
  // But the UnknownArrayHandle has type fileStorageDataType.
  // If the fileStorageDataType is the same as the storage type, no problem:
  if (fileStorageDataType == vtkm::io::internal::DataTypeName<vtkm::FloatDefault>::Name())
  {
    X.AsArrayHandle(Xc);
    Y.AsArrayHandle(Yc);
    Z.AsArrayHandle(Zc);
  }
  else
  {
    // Two cases if the data in the file differs from FloatDefault:
    if (fileStorageDataType == "float")
    {
      vtkm::cont::ArrayHandle<vtkm::Float32> Xcf, Ycf, Zcf;
      X.AsArrayHandle(Xcf);
      Y.AsArrayHandle(Ycf);
      Z.AsArrayHandle(Zcf);
      vtkm::cont::ArrayCopy(Xcf, Xc);
      vtkm::cont::ArrayCopy(Ycf, Yc);
      vtkm::cont::ArrayCopy(Zcf, Zc);
    }
    else
    {
      vtkm::cont::ArrayHandle<vtkm::Float64> Xcd, Ycd, Zcd;
      X.AsArrayHandle(Xcd);
      Y.AsArrayHandle(Ycd);
      Z.AsArrayHandle(Zcd);
      vtkm::cont::ArrayCopy(Xcd, Xc);
      vtkm::cont::ArrayCopy(Ycd, Yc);
      vtkm::cont::ArrayCopy(Zcd, Zc);
    }
  }
  // As a postscript to this somewhat branchy code, I thought that X.CopyTo(Xc) should be able to cast to the value_type of Xc.
  // But that would break the ability to make the cast lazy, and hence if you change it you induce performance bugs.

  coords = vtkm::cont::make_ArrayHandleCartesianProduct(Xc, Yc, Zc);
  vtkm::cont::CoordinateSystem coordSys("coordinates", coords);
  this->DataSet.AddCoordinateSystem(coordSys);
  this->DataSet.SetCellSet(internal::CreateCellSetStructured(dim));

  // Read points and cell attributes
  this->ReadAttributes();
}
}
} // namespace vtkm::io
