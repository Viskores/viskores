//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_contour_ClipWithImplicitFunction_h
#define vtk_m_filter_contour_ClipWithImplicitFunction_h

#include <vtkm/ImplicitFunction.h>

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/contour/vtkm_filter_contour_export.h>

namespace vtkm
{
namespace filter
{
namespace contour
{
/// \brief Clip a dataset using an implicit function
///
/// Clip a dataset using a given implicit function value, such as vtkm::Sphere
/// or vtkm::Frustum.
/// The resulting geometry will not be water tight.
class VTKM_FILTER_CONTOUR_EXPORT ClipWithImplicitFunction : public vtkm::filter::NewFilterField
{
public:
  void SetImplicitFunction(const vtkm::ImplicitFunctionGeneral& func) { this->Function = func; }

  void SetInvertClip(bool invert) { this->Invert = invert; }

  const vtkm::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

private:
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::ImplicitFunctionGeneral Function;
  bool Invert = false;
};
} // namespace contour
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::contour::ClipWithImplicitFunction.")
  ClipWithImplicitFunction : public vtkm::filter::contour::ClipWithImplicitFunction
{
  using contour::ClipWithImplicitFunction::ClipWithImplicitFunction;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_ClipWithImplicitFunction_h
