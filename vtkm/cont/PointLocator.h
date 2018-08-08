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
#ifndef vtk_m_cont_PointLocator_h
#define vtk_m_cont_PointLocator_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/exec/PointLocator.h>

namespace vtkm
{
namespace cont
{

class PointLocator : public vtkm::cont::ExecutionObjectBase
{
public:
  PointLocator()
    : dirty(true)
  {
  }

  vtkm::cont::CoordinateSystem GetCoords() const { return coordinates; }

  void SetCoords(const vtkm::cont::CoordinateSystem& coords)
  {
    coordinates = coords;
    dirty = true;
  }

  virtual void Build() = 0;

  void Update()
  {
    if (dirty)
      Build();
    dirty = false;
  }

  template <typename DeviceAdapter>
  VTKM_CONT const vtkm::exec::PointLocator* PrepareForExecution(DeviceAdapter device) const
  {
    return PrepareForExecutionImp(device).PrepareForExecution(device);
  }

  //VTKM_CONT virtual const vtkm::exec::PointLocator*
  using HandleType = vtkm::cont::VirtualObjectHandle<vtkm::exec::PointLocator>;
  VTKM_CONT virtual const HandleType PrepareForExecutionImp(
    vtkm::cont::DeviceAdapterId deviceId) const = 0;

private:
  vtkm::cont::CoordinateSystem coordinates;

  bool dirty;
};
}
}
#endif // vtk_m_cont_PointLocator_h
