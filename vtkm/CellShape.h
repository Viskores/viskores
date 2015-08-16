//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_CellShape_h
#define vtk_m_CellShape_h

namespace vtkm {

/// CellShapeId identifies the type of each cell. Currently these are designed
/// to match up with VTK cell types.
///
enum CellShapeId
{
  // Linear cells
  CELL_SHAPE_EMPTY              = 0,
  CELL_SHAPE_VERTEX             = 1,
  //CELL_SHAPE_POLY_VERTEX      = 2,
  CELL_SHAPE_LINE               = 3,
  //CELL_SHAPE_POLY_LINE        = 4,
  CELL_SHAPE_TRIANGLE           = 5,
  //CELL_SHAPE_TRIANGLE_STRIP   = 6,
  CELL_SHAPE_POLYGON            = 7,
  CELL_SHAPE_PIXEL              = 8,
  CELL_SHAPE_QUAD               = 9,
  CELL_SHAPE_TETRA              = 10,
  CELL_SHAPE_VOXEL              = 11,
  CELL_SHAPE_HEXAHEDRON         = 12,
  CELL_SHAPE_WEDGE              = 13,
  CELL_SHAPE_PYRAMID            = 14,

  NUMBER_OF_CELL_SHAPES
};


} // namespace vtkm

#endif //vtk_m_CellShape_h
