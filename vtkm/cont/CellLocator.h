//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellLocator_h
#define vtk_m_cont_CellLocator_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ExecutionObjectBase.h>

#include <vtkm/exec/CellLocator.h>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT CellLocator : public vtkm::cont::ExecutionObjectBase
{

public:
  virtual ~CellLocator();

  const vtkm::cont::DynamicCellSet& GetCellSet() const { return this->CellSet; }

  void SetCellSet(const vtkm::cont::DynamicCellSet& cellSet)
  {
    this->CellSet = cellSet;
    this->SetModified();
  }

  const vtkm::cont::CoordinateSystem& GetCoordinates() const { return this->Coords; }

  void SetCoordinates(const vtkm::cont::CoordinateSystem& coords)
  {
    this->Coords = coords;
    this->SetModified();
  }

  void Update()
  {
    if (this->Modified)
    {
      this->Build();
      this->Modified = false;
    }
  }

  VTKM_CONT virtual const vtkm::exec::CellLocator* PrepareForExecution(
    vtkm::cont::DeviceAdapterId device) const = 0;

protected:
  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }

  //This is going to need a TryExecute
  VTKM_CONT virtual void Build() = 0;

private:
  vtkm::cont::DynamicCellSet CellSet;
  vtkm::cont::CoordinateSystem Coords;
  bool Modified = true;
};

} // namespace cont
} // namespace vtkm

#endif // vtk_m_cont_CellLocator_h
