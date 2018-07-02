//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_PointLocator_h
#define vtk_m_exec_PointLocator_h

#include <vtkm/VirtualObjectBase.h>

namespace vtkm
{
namespace exec
{

class PointLocator : public vtkm::VirtualObjectBase
{
public:
  VTKM_EXEC virtual void FindNearestNeighbor(const vtkm::Vec<vtkm::FloatDefault, 3>& queryPoint,
                                             vtkm::Id& pointId,
                                             vtkm::FloatDefault& distanceSquared) const = 0;
};
}
}
#endif // vtk_m_exec_PointLocator_h
