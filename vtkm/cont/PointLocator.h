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
#include <vtkm/cont/VirtualObjectHandle.h>
#include <vtkm/exec/PointLocator.h>

namespace vtkm
{
namespace cont
{

class PointLocator : public vtkm::cont::ExecutionObjectBase
{
public:
  PointLocator()
    : Modified(true)
  {
  }

  vtkm::cont::CoordinateSystem GetCoordinates() const { return this->Coords; }

  void SetCoordinates(const vtkm::cont::CoordinateSystem& coords)
  {
    this->Coords = coords;
    this->SetModified();
  }

  void Update()
  {
    if (this->Modified)
    {
      Build();
      this->Modified = false;
    }
  }

  VTKM_CONT const vtkm::exec::PointLocator* PrepareForExecution(
    vtkm::cont::DeviceAdapterId device) const
  {
    this->PrepareExecutionObject(this->ExecutionObjectHandle, device);
    return this->ExecutionObjectHandle.PrepareForExecution(device);
  }

protected:
  void SetModified() { this->Modified = true; }

  bool GetModified() const { return this->Modified; }

  virtual void Build() = 0;

  using ExecutionObjectHandleType = vtkm::cont::VirtualObjectHandle<vtkm::exec::PointLocator>;

  VTKM_CONT virtual void PrepareExecutionObject(ExecutionObjectHandleType& execObjHandle,
                                                vtkm::cont::DeviceAdapterId deviceId) const = 0;

private:
  vtkm::cont::CoordinateSystem Coords;
  bool Modified;

  mutable ExecutionObjectHandleType ExecutionObjectHandle;
};
}
}
#endif // vtk_m_cont_PointLocator_h
