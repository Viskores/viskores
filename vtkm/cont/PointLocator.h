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
#ifndef vtk_m_cont_PointLocator_h
#define vtk_m_cont_PointLocator_h

#include <vtkm/Types.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapter.h>
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
    : Dirty(true)
  {
  }

  vtkm::cont::CoordinateSystem GetCoordinates() const { return Coords; }

  void SetCoordinates(const vtkm::cont::CoordinateSystem& coords)
  {
    Coords = coords;
    Dirty = true;
  }

  void Update()
  {
    if (Dirty)
      Build();
    Dirty = false;
  }

  template <typename DeviceAdapter>
  VTKM_CONT const vtkm::exec::PointLocator* PrepareForExecution(DeviceAdapter)
  {
    vtkm::cont::DeviceAdapterId deviceId = vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetId();
    return PrepareForExecution(deviceId);
  }

protected:
  void SetDirty() { Dirty = true; }

  VTKM_CONT virtual void Build() = 0;

  VTKM_CONT virtual const vtkm::exec::PointLocator* PrepareForExecutionImpl(
    const vtkm::Int8 device) = 0;

private:
  vtkm::cont::CoordinateSystem Coords;
  bool Dirty;
};

} // namespace cont
} // namespace vtkm

#endif // vtk_m_cont_PointLocator_h
