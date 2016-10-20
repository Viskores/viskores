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
#ifndef vtk_m_io_reader_VTKUnstructuredGridReader_h
#define vtk_m_io_reader_VTKUnstructuredGridReader_h

#include <vtkm/io/reader/VTKDataSetReaderBase.h>

namespace vtkm {
namespace io {
namespace reader {

class VTKUnstructuredGridReader : public VTKDataSetReaderBase
{
public:
  explicit VTKUnstructuredGridReader(const char *fileName)
    : VTKDataSetReaderBase(fileName)
  { }

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
    internal::parseAssert(tag == "POINTS" || tag == "FIELD");

    if(tag == "FIELD")
    {
      std::string name;
      this->ReadFields(name);
      this->DataFile->Stream >> tag;
      internal::parseAssert(tag == "POINTS");
    }

    // Read the points
    this->ReadPoints();

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

    if (vtkm::io::internal::IsSingleShape(shapes))
    {
      vtkm::cont::CellSetSingleType<> cs;
      switch(shapes.GetPortalConstControl().Get(0))
      {
      vtkmGenericCellShapeMacro((cs = vtkm::cont::CellSetSingleType<>(CellShapeTag(), 0, "cells")));
      default:
        break;
      }
      cs.Fill(connectivity);
      this->DataSet.AddCellSet(cs);
    }
    else
    {
      vtkm::cont::CellSetExplicit<> cs(0, "cells");
      cs.Fill(shapes, numIndices, connectivity);
      this->DataSet.AddCellSet(cs);
    }

    // Read points and cell attributes
    this->ReadAttributes();
  }
};

}
}
} // namespace vtkm::io:reader

#endif // vtk_m_io_reader_VTKUnstructuredGridReader_h
