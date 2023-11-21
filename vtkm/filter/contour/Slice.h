//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_contour_Slice_h
#define vtk_m_filter_contour_Slice_h

#include <vtkm/filter/contour/Contour.h>
#include <vtkm/filter/contour/vtkm_filter_contour_export.h>

#include <vtkm/ImplicitFunction.h>

namespace vtkm
{
namespace filter
{
namespace contour
{

/// @brief Intersect a mesh with an implicit surface.
///
/// This filter accepts a `vtkm::ImplicitFunction` that defines the surface to
/// slice on. A `vtkm::Plane` is a common function to use that cuts the mesh
/// along a plane.
///
class VTKM_FILTER_CONTOUR_EXPORT Slice : public vtkm::filter::contour::Contour
{
public:
  /// @brief Set the implicit function that is used to perform the slicing.
  ///
  /// Only a limited number of implicit functions are supported. See
  /// `vtkm::ImplicitFunctionGeneral` for information on which ones.
  ///
  VTKM_CONT
  void SetImplicitFunction(const vtkm::ImplicitFunctionGeneral& func) { this->Function = func; }
  /// @brief Get the implicit function that us used to perform the slicing.
  VTKM_CONT
  const vtkm::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::ImplicitFunctionGeneral Function;
};
} // namespace contour
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_Slice_h
