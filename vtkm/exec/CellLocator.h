//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_CellLocator_h
#define vtk_m_exec_CellLocator_h

#include <vtkm/Types.h>
#include <vtkm/VirtualObjectBase.h>
#include <vtkm/exec/FunctorBase.h>

namespace vtkm
{
namespace exec
{

class CellLocator : public vtkm::VirtualObjectBase
{
public:
  VTKM_EXEC
  virtual void FindCell(const vtkm::Vec<vtkm::FloatDefault, 3>& point,
                        vtkm::Id& cellId,
                        vtkm::Vec<vtkm::FloatDefault, 3>& parametric,
                        const vtkm::exec::FunctorBase& worklet) const = 0;
};

} // namespace exec
} // namespace vtkm

#endif // vtk_m_exec_CellLocator_h
