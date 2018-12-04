//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_Ghost_Cell_h
#define vtk_m_Ghost_Cell_h

namespace vtkm
{

enum struct CellClassification
{
  NORMAL = 0,         //Valid cell
  DUPLICATE = 1 << 0, //Ghost cell
  INVALID = 1 << 1,   //Cell is invalid
  UNUSED0 = 1 << 2,
  UNUSED1 = 1 << 3,
  UNUSED3 = 1 << 4,
  UNUSED4 = 1 << 5,
  UNUSED5 = 1 << 6,
};
}

#endif // vtk_m_Ghost_Cell_h
