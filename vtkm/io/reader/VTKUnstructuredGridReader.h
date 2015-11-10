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
    if (this->DataFile->Structure != internal::DATASET_UNSTRUCTURED_GRID)
    {
      throw vtkm::io::ErrorIO("Incorrect DataSet type");
    }

    std::string tag;

    // Read the points
    this->ReadPoints();

    // Read the cellset
    std::vector<vtkm::Id> connectivity;
    std::vector<vtkm::IdComponent> numIndices;
    std::vector<vtkm::UInt8> shapes;

    this->DataFile->Stream >> tag;
    internal::parseAssert(tag == "CELLS");
    this->ReadCells(connectivity, numIndices);
    this->ReadShapes(shapes);

    bool sameShape = true;
    for (std::size_t i = 1; i < shapes.size(); ++i)
    {
      if (shapes[i] != shapes[i - 1])
      {
        sameShape = false;
        break;
      }
    }

    if (sameShape)
    {
      vtkm::cont::CellSetSingleType<> cs;
      switch(shapes[0])
      {
      case vtkm::CELL_SHAPE_VERTEX:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagVertex(), "cells");
        break;
      case vtkm::CELL_SHAPE_LINE:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagLine(), "cells");
        break;
      case vtkm::CELL_SHAPE_TRIANGLE:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagTriangle(), "cells");
        break;
      case vtkm::CELL_SHAPE_QUAD:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagQuad(), "cells");
        break;
      case vtkm::CELL_SHAPE_TETRA:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagTetra(), "cells");
        break;
      case vtkm::CELL_SHAPE_HEXAHEDRON:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagHexahedron(), "cells");
        break;
      case vtkm::CELL_SHAPE_WEDGE:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagWedge(), "cells");
        break;
      case vtkm::CELL_SHAPE_PYRAMID:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagPyramid(), "cells");
        break;
      default:
        assert(false);
      }

      cs.FillViaCopy(connectivity);
      this->DataSet.AddCellSet(cs);
    }
    else
    {
      vtkm::cont::CellSetExplicit<> cs(0, "cells");
      cs.FillViaCopy(shapes, numIndices, connectivity);
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
