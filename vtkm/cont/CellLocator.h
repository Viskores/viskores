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
#ifndef vtk_m_cont_CellLocator_h
#define vtk_m_cont_CellLocator_h

#include <vtkm/Types.h>
#include <vtkm/VirtualObjectBase.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{

namespace exec
{
// This will actually be used in the Execution Environment.
// As this object is returned by the PrepareForExecution on
// the CellLocator we need it to be covarient, and this acts
// like a base class.

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

namespace cont
{

class CellLocator : public vtkm::cont::ExecutionObjectBase
{

public:
  CellLocator()
    : Dirty(true)
  {
  }

  vtkm::cont::DynamicCellSet GetCellSet() const { return CellSet; }

  void SetCellSet(const vtkm::cont::DynamicCellSet& cellSet)
  {
    CellSet = cellSet;
    Dirty = true;
  }

  vtkm::cont::CoordinateSystem GetCoords() const { return Coords; }

  void SetCoords(const vtkm::cont::CoordinateSystem& coords)
  {
    Coords = coords;
    Dirty = true;
  }

  //This is going to need a TryExecute
  virtual void Build() = 0;

  void Update()
  {
    if (Dirty)
      Build();
    Dirty = false;
  }

  template <typename DeviceAdapter>
  VTKM_CONT const vtkm::exec::CellLocator* PrepareForExecution(DeviceAdapter) const
  {
    vtkm::cont::DeviceAdapterId deviceId = vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetId();
    return PrepareForExecutionImpl(deviceId);
  }

protected:
  VTKM_CONT virtual const vtkm::exec::CellLocator* PrepareForExecutionImpl(
    const vtkm::Int8 device) const = 0;

private:
  vtkm::cont::DynamicCellSet CellSet;
  vtkm::cont::CoordinateSystem Coords;
  bool Dirty;
};

} // namespace cont
} // namespace vtkm

#endif // vtk_m_cont_CellLocator_h
