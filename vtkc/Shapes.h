//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_c_Shapes_h
#define vtk_c_Shapes_h

#include <vtkc/internal/Config.h>

#include <cstdint>

namespace vtkc
{

enum ShapeId : IdShape
{
  // Linear cells
  EMPTY            = 0,
  VERTEX           = 1,
  //POLY_VERTEX      = 2,
  LINE             = 3,
  //POLY_LINE        = 4,
  TRIANGLE         = 5,
  //TRIANGLE_STRIP   = 6,
  POLYGON          = 7,
  PIXEL            = 8,
  QUAD             = 9,
  TETRA            = 10,
  VOXEL            = 11,
  HEXAHEDRON       = 12,
  WEDGE            = 13,
  PYRAMID          = 14,

  NUMBER_OF_CELL_SHAPES
};

class Cell
{
public:
  constexpr VTKC_EXEC Cell() : Shape(ShapeId::EMPTY), NumberOfPoints(0) {}
  constexpr VTKC_EXEC Cell(IdShape shape, IdComponent numberOfPoints)
    : Shape(shape), NumberOfPoints(numberOfPoints)
  {
  }

  constexpr VTKC_EXEC IdShape shape() const noexcept { return this->Shape; }
  constexpr VTKC_EXEC IdComponent numberOfPoints() const noexcept { return this->NumberOfPoints; }

protected:
  IdShape Shape;
  IdComponent NumberOfPoints;
};

/// \brief Check if a shape id is valid
/// \param[in]  shapeId  The id to check.
/// \return              true if id is a valid shape id.
///
constexpr inline VTKC_EXEC bool isValidShape(IdShape shapeId)
{
  return (shapeId >= ShapeId::EMPTY) && (shapeId < ShapeId::NUMBER_OF_CELL_SHAPES);
}

/// \brief Returns the dimensionality of a cell
/// \param[in]  shapeId  The shape id of the cell.
/// \return              The dimensionality of the cell, -1 for invalid shapes.
///
inline VTKC_EXEC int dimension(IdShape shapeId)
{
  switch (shapeId)
  {
    case VERTEX:
      return 0;
    case LINE:
      return 1;
    case TRIANGLE:
    case POLYGON:
    case PIXEL:
    case QUAD:
      return 2;
    case TETRA:
    case VOXEL:
    case HEXAHEDRON:
    case WEDGE:
    case PYRAMID:
      return 3;
    case EMPTY:
    default:
      return -1;
  }
}

/// \brief Returns the dimensionality of a cell
/// \param[in]  cell  The cell.
/// \return           The dimensionality of the cell, -1 for invalid shapes.
///
inline VTKC_EXEC int dimension(Cell cell)
{
  return dimension(cell.shape());
}

// forward declare cell tags
class Vertex;
class Line;
class Triangle;
class Polygon;
class Pixel;
class Quad;
class Tetra;
class Voxel;
class Hexahedron;
class Wedge;
class Pyramid;

} //namespace vtkc

#define vtkcGenericCellShapeMacroCase(cellId, cell, call)                                          \
  case cellId:                                                                                     \
  {                                                                                                \
    using CellTag = cell;                                                                          \
    call;                                                                                          \
  }                                                                                                \
  break

#define vtkcGenericCellShapeMacro(call)                                                            \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::VERTEX,     vtkc::Vertex,     call);                \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::LINE,       vtkc::Line,       call);                \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::TRIANGLE,   vtkc::Triangle,   call);                \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::POLYGON,    vtkc::Polygon,    call);                \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::PIXEL,      vtkc::Pixel,      call);                \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::QUAD,       vtkc::Quad,       call);                \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::TETRA,      vtkc::Tetra,      call);                \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::VOXEL,      vtkc::Voxel,      call);                \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::HEXAHEDRON, vtkc::Hexahedron, call);                \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::WEDGE,      vtkc::Wedge,      call);                \
  vtkcGenericCellShapeMacroCase(vtkc::ShapeId::PYRAMID,    vtkc::Pyramid,    call)

#endif //vtk_c_Shapes_h
