//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_PointLocatorBase_h
#define vtk_m_cont_internal_PointLocatorBase_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ExecutionObjectBase.h>

namespace vtkm
{
namespace cont
{

/// @brief Base class for all `PointLocator` classes.
///
/// `PointLocatorBase` subclasses must implement the pure virtual `Build()` method.
/// They also must provide a `PrepareForExecution()` method to satisfy the
/// `ExecutionObjectBase` superclass.
///
/// If a derived class changes its state in a way that invalidates its internal search
/// structure, it should call the protected `SetModified()` method. This will alert the
/// base class to rebuild the structure on the next call to `Update()`.
class VTKM_CONT_EXPORT PointLocatorBase : public vtkm::cont::ExecutionObjectBase
{
public:
  virtual ~PointLocatorBase() = default;

  /// @brief Specify the `CoordinateSystem` defining the location of the cells.
  ///
  /// This is often retrieved from the `vtkm::cont::DataSet::GetCoordinateSystem()` method,
  /// but it can be any array of size 3 `Vec`s.
  vtkm::cont::CoordinateSystem GetCoordinates() const { return this->Coords; }
  /// @copydoc GetCoordinates
  void SetCoordinates(const vtkm::cont::CoordinateSystem& coords)
  {
    this->Coords = coords;
    this->SetModified();
  }
  /// @copydoc GetCoordinates
  VTKM_CONT void SetCoordinates(const vtkm::cont::UnknownArrayHandle& coords)
  {
    this->SetCoordinates({ "coords", coords });
  }

  void Update() const;

protected:
  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }

  virtual void Build() = 0;

private:
  vtkm::cont::CoordinateSystem Coords;
  mutable bool Modified = true;
};

} // vtkm::cont
} // vtkm

#endif // vtk_m_cont_internal_PointLocatorBase_h
