//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_PointLocator_h
#define vtk_m_cont_PointLocator_h

#include <vtkm/Deprecated.h>
#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/VirtualObjectHandle.h>
#include <vtkm/exec/PointLocator.h>

#ifdef VTKM_NO_DEPRECATED_VIRTUAL
#error "PointLocator with virtual methods is removed. Do not include PointLocator.h"
#endif

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT VTKM_DEPRECATED(1.6, "PointLocator with virtual methods no longer supported")
  PointLocator : public vtkm::cont::ExecutionObjectBase
{
  VTKM_DEPRECATED_SUPPRESS_BEGIN

public:
  virtual ~PointLocator();

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

  VTKM_CONT virtual const vtkm::exec::PointLocator* PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const = 0;

  VTKM_CONT
  VTKM_DEPRECATED(1.6, "PrepareForExecution now requires a vtkm::cont::Token object")
  const vtkm::exec::PointLocator* PrepareForExecution(vtkm::cont::DeviceAdapterId device) const
  {
    vtkm::cont::Token token;
    return this->PrepareForExecution(device, token);
  }

protected:
  void SetModified() { this->Modified = true; }

  bool GetModified() const { return this->Modified; }

  virtual void Build() = 0;

private:
  vtkm::cont::CoordinateSystem Coords;
  bool Modified;
};
VTKM_DEPRECATED_SUPPRESS_END

} // namespace vtkm::cont
} // namespace vtkm

#endif // vtk_m_cont_PointLocator_h
