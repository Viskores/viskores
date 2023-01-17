//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_CellLocatorBase_h
#define vtk_m_cont_internal_CellLocatorBase_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/UnknownCellSet.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

/// \brief Base class for all `CellLocator` classes.
///
/// `CellLocatorBase` uses the curiously recurring template pattern (CRTP). Subclasses
/// must provide their own type for the template parameter. Subclasses must implement
/// `Update` and `PrepareForExecution` methods.
///
template <typename Derived>
class VTKM_ALWAYS_EXPORT CellLocatorBase : public vtkm::cont::ExecutionObjectBase
{
  vtkm::cont::UnknownCellSet CellSet;
  vtkm::cont::CoordinateSystem Coords;
  mutable bool Modified = true;

public:
  const vtkm::cont::UnknownCellSet& GetCellSet() const { return this->CellSet; }

  void SetCellSet(const vtkm::cont::UnknownCellSet& cellSet)
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

  void Update() const
  {
    if (this->Modified)
    {
      static_cast<Derived*>(const_cast<CellLocatorBase*>(this))->Build();
      this->Modified = false;
    }
  }

protected:
  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }
};

}
}
} // vtkm::cont::internal

#endif //vtk_m_cont_internal_CellLocatorBase_h
