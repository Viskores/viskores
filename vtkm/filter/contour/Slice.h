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
class VTKM_FILTER_CONTOUR_EXPORT Slice : public vtkm::filter::contour::Contour
{
public:
  /// Set/Get the implicit function that is used to perform the slicing.
  ///
  VTKM_CONT
  void SetImplicitFunction(const vtkm::ImplicitFunctionGeneral& func) { this->Function = func; }
  VTKM_CONT
  const vtkm::ImplicitFunctionGeneral& GetImplicitFunction() const { return this->Function; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::ImplicitFunctionGeneral Function;
};
} // namespace contour
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::contour::Slice.") Slice
  : public vtkm::filter::contour::Slice
{
  using contour::Slice::Slice;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_contour_Slice_h
