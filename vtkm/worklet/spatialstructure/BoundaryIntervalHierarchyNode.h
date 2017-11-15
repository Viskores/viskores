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

#ifndef vtk_m_worklet_spatialstructure_BoundaryIntervalHierarchyNode_h
#define vtk_m_worklet_spatialstructure_BoundaryIntervalHierarchyNode_h

#include <vtkm/Types.h>

namespace vtkm
{
namespace worklet
{
namespace spatialstructure
{

struct BoundaryIntervalHierarchyNode
{
  vtkm::IdComponent Dimension;
  vtkm::Id ChildIndex;
  union {
    struct
    {
      vtkm::Float64 LMax;
      vtkm::Float64 RMin;
    } Node;
    struct
    {
      vtkm::Id Start;
      vtkm::Id Size;
    } Leaf;
  };
}; // struct BoundaryIntervalHierarchyNode
}
}
} // namespace vtkm::worklet::spatialstructure

#endif // vtk_m_worklet_spatialstructure_BoundaryIntervalHierarchyNode_h
