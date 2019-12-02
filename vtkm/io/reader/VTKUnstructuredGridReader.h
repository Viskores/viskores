//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_reader_VTKUnstructuredGridReader_h
#define vtk_m_io_reader_VTKUnstructuredGridReader_h

#include <vtkm/io/reader/VTKDataSetReaderBase.h>

namespace vtkm
{
namespace io
{
namespace reader
{

VTKM_SILENCE_WEAK_VTABLE_WARNING_START

class VTKUnstructuredGridReader : public VTKDataSetReaderBase
{
public:
  explicit VTKUnstructuredGridReader(const char* fileName)
    : VTKDataSetReaderBase(fileName)
  {
  }

private:
  virtual void Read()
  {
    if (this->DataFile->Structure != vtkm::io::internal::DATASET_UNSTRUCTURED_GRID)
    {
      throw vtkm::io::ErrorIO("Incorrect DataSet type");
    }

    //We need to be able to handle VisIt files which dump Field data
    //at the top of a VTK file
    std::string tag;
    this->DataFile->Stream >> tag;
    if (tag == "FIELD")
    {
      this->ReadGlobalFields();
      this->DataFile->Stream >> tag;
    }

    // Read the points
    internal::parseAssert(tag == "POINTS");
    this->ReadPoints();

    vtkm::Id numPoints = this->DataSet.GetNumberOfPoints();

    // Read the cellset
    vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
    vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;

    this->DataFile->Stream >> tag;
    internal::parseAssert(tag == "CELLS");

    this->ReadCells(connectivity, numIndices);
    this->ReadShapes(shapes);

    vtkm::cont::ArrayHandle<vtkm::Id> permutation;
    vtkm::io::internal::FixupCellSet(connectivity, numIndices, shapes, permutation);
    this->SetCellsPermutation(permutation);

    //DRP
    if (false) //vtkm::io::internal::IsSingleShape(shapes))
    {
      vtkm::cont::CellSetSingleType<> cellSet;
      cellSet.Fill(numPoints,
                   shapes.GetPortalConstControl().Get(0),
                   numIndices.GetPortalConstControl().Get(0),
                   connectivity);
      this->DataSet.SetCellSet(cellSet);
    }
    else
    {
      auto offsets = vtkm::cont::ConvertNumIndicesToOffsets(numIndices);
      vtkm::cont::CellSetExplicit<> cellSet;
      cellSet.Fill(numPoints, shapes, connectivity, offsets);
      this->DataSet.SetCellSet(cellSet);
    }

    // Read points and cell attributes
    this->ReadAttributes();
  }
};

VTKM_SILENCE_WEAK_VTABLE_WARNING_END
}
}
} // namespace vtkm::io:reader

#endif // vtk_m_io_reader_VTKUnstructuredGridReader_h
