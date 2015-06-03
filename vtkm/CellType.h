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
#ifndef vtk_m_CellType_h
#define vtk_m_CellType_h

namespace vtkm {

/// CellType stores the type of each cell.  Currently these are designed to
/// match up with VTK cell types.
///
enum CellType
{
  // Linear cells
  VTKM_EMPTY_CELL         = 0,
  VTKM_VERTEX             = 1,
  //VTKM_POLY_VERTEX      = 2,
  VTKM_LINE               = 3,
  //VTKM_POLY_LINE        = 4,
  VTKM_TRIANGLE           = 5,
  //VTKM_TRIANGLE_STRIP   = 6,
  VTKM_POLYGON            = 7,
  VTKM_PIXEL              = 8,
  VTKM_QUAD               = 9,
  VTKM_TETRA              = 10,
  VTKM_VOXEL              = 11,
  VTKM_HEXAHEDRON         = 12,
  VTKM_WEDGE              = 13,
  VTKM_PYRAMID            = 14,

  VTKM_NUMBER_OF_CELL_TYPES
};


} // namespace vtkm

#endif //vtk_m_CellType_h
