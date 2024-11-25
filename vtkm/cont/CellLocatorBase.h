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

#include <vtkm/Deprecated.h>
#include <vtkm/Types.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/UnknownCellSet.h>

namespace vtkm
{
namespace cont
{

/// \brief Base class for all `CellLocator` classes.
///
/// `CellLocatorBase` uses the curiously recurring template pattern (CRTP). Subclasses
/// must provide their own type for the template parameter. Subclasses must implement
/// `Build()` and `PrepareForExecution()` methods.
///
/// If a derived class changes its state in a way that invalidates its internal search
/// structure, it should call the protected `SetModified()` method. This will alert the
/// base class to rebuild the structure on the next call to `Update()`.
class VTKM_CONT_EXPORT CellLocatorBase : public vtkm::cont::ExecutionObjectBase
{
  vtkm::cont::UnknownCellSet CellSet;
  vtkm::cont::CoordinateSystem Coords;
  mutable bool Modified = true;

public:
  virtual ~CellLocatorBase() = default;

  /// @brief Specify the `CellSet` defining the structure of the cells being searched.
  ///
  /// This is typically retrieved from the `vtkm::cont::DataSet::GetCellSet()` method.
  VTKM_CONT const vtkm::cont::UnknownCellSet& GetCellSet() const { return this->CellSet; }
  /// @copydoc GetCellSet
  VTKM_CONT void SetCellSet(const vtkm::cont::UnknownCellSet& cellSet)
  {
    this->CellSet = cellSet;
    this->SetModified();
  }

  /// @brief Specify the `CoordinateSystem` defining the location of the cells.
  ///
  /// This is typically retrieved from the `vtkm::cont::DataSet::GetCoordinateSystem()` method.
  VTKM_CONT const vtkm::cont::CoordinateSystem& GetCoordinates() const { return this->Coords; }
  /// @copydoc GetCoordinates
  VTKM_CONT void SetCoordinates(const vtkm::cont::CoordinateSystem& coords)
  {
    this->Coords = coords;
    this->SetModified();
  }

  /// @brief Build the search structure used to look up cells.
  ///
  /// This method must be called after the cells and coordiantes are specified with
  /// `SetCellSet()` and `SetCoordinates()`, respectively.
  /// The method must also be called before it is used with a worklet.
  /// Before building the search structure `Update()` checks to see if the structure is
  /// already built and up to date. If so, the method quickly returns.
  /// Thus, it is good practice to call `Update()` before each use in a worklet.
  ///
  /// Although `Update()` is called from the control environment, it lauches jobs in the
  /// execution environment to quickly build the search structure.
  VTKM_CONT void Update() const;

protected:
  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }

  virtual void Build() = 0;
};

}
} // vtkm::cont::internal

#endif //vtk_m_cont_internal_CellLocatorBase_h
