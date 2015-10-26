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
#ifndef vtk_m_io_reader_VTKPolyDataReader_h
#define vtk_m_io_reader_VTKPolyDataReader_h

#include <vtkm/io/reader/VTKDataSetReaderBase.h>

namespace vtkm {
namespace io {
namespace reader {

class VTKPolyDataReader : public VTKDataSetReaderBase
{
public:
  explicit VTKPolyDataReader(const char *fileName)
    : VTKDataSetReaderBase(fileName)
  { }

private:
  virtual void Read()
  {
    if (this->DataFile->Structure != internal::DATASET_POLYDATA)
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
    bool sameShape = true;
    while (!this->DataFile->Stream.eof())
    {
      vtkm::CellShapeIdEnum shape = vtkm::CELL_SHAPE_EMPTY;
      this->DataFile->Stream >> tag;
      if (tag == "VERTICES")
      {
        shape = vtkm::CELL_SHAPE_VERTEX;
      }
      else if (tag == "LINES")
      {
        shape = vtkm::CELL_SHAPE_LINE;
      }
      else if (tag == "POLYGONS")
      {
        shape = vtkm::CELL_SHAPE_POLYGON;
      }
      else if (tag == "TRIANGLE_STRIPS")
      {
        std::cerr << "Triangle strips are not supported. Skipping.";
      }
      else
      {
        this->DataFile->Stream.seekg(-static_cast<std::streamoff>(tag.length()),
                                     std::ios_base::cur);
        break;
      }

      std::size_t prevConnLength = connectivity.size();
      std::size_t prevNumIndicesLength = numIndices.size();
      sameShape = (prevNumIndicesLength == 0);
      this->ReadCells(connectivity, numIndices);

      std::size_t numNewCells = prevNumIndicesLength - numIndices.size();
      std::size_t newConnSize = prevConnLength - connectivity.size();
      switch (shape)
      {
      case vtkm::CELL_SHAPE_VERTEX:
        if (newConnSize != numNewCells)
        {
          throw vtkm::io::ErrorIO("POLY_VERTEX not supported");
        }
        shapes.insert(shapes.end(), numNewCells, vtkm::CELL_SHAPE_VERTEX);
        break;
      case vtkm::CELL_SHAPE_LINE:
        if (newConnSize != (numNewCells * 2))
        {
          throw vtkm::io::ErrorIO("POLY_LINE not supported");
        }
        shapes.insert(shapes.end(), numNewCells, vtkm::CELL_SHAPE_LINE);
        break;
      case vtkm::CELL_SHAPE_POLYGON:
        for (std::size_t i = prevNumIndicesLength; i < numIndices.size() - 1; ++i)
        {
          switch (numIndices[i])
          {
          case 3:
            shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
            break;
          case 4:
            shapes.push_back(vtkm::CELL_SHAPE_QUAD);
            break;
          default:
            sameShape = false;
            shapes.push_back(vtkm::CELL_SHAPE_POLYGON);
            break;
          }
          if (i > prevNumIndicesLength && sameShape && shapes[i - 1] != shapes[i])
          {
            sameShape = false;
          }
        }
        break;
      case vtkm::CELL_SHAPE_EMPTY: // TRIANGLE_STRIPS
        continue;
      default:
        assert(false);
      }
    }

    if (sameShape)
    {
      vtkm::cont::CellSetSingleType<> cs;
      switch(shapes[0])
      {
      case vtkm::CELL_SHAPE_VERTEX:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagVertex(),
                                             "cells");
        break;
      case vtkm::CELL_SHAPE_LINE:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagLine(),
                                             "cells");
        break;
      case vtkm::CELL_SHAPE_TRIANGLE:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagTriangle(),
                                             "cells");
        break;
      case vtkm::CELL_SHAPE_QUAD:
        cs = vtkm::cont::CellSetSingleType<>(vtkm::CellShapeTagQuad(),
                                             "cells");
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

#endif // vtk_m_io_reader_VTKPolyDataReader_h
