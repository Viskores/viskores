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

#ifndef vtkm_worklet_contourtree_link_component_case_table_2d_h
#define vtkm_worklet_contourtree_link_component_case_table_2d_h

#include <vtkm/worklet/contourtree/Mesh2D_DEM_Triangulation_Macros.h>

namespace vtkm {
namespace worklet {
namespace contourtree {

const vtkm::IdComponent neighbourOffsets[N_INCIDENT_EDGES][2] = {
    { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, -1 }, { -1, -1 }, { -1, 0 }
};

const vtkm::UInt8 linkComponentCaseTable[64] = {
  0, 1, 2, 2, 4, 5, 4, 4, 8, 9, 10, 10, 8, 9, 8, 8, 16, 17, 18, 18, 20, 21, 20, 20, 16,
  17, 18, 18, 16, 17, 16, 16, 32, 32, 34, 32, 36, 36, 36, 32, 40, 40, 42, 40, 40, 40, 40, 32, 32, 32,
  34, 32, 36, 36, 36, 32, 32, 32, 34, 32, 32, 32, 32, 32
};

}
}
}

#endif
