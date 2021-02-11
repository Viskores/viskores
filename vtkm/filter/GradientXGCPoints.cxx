//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_Gradient_XGC_cxx
#define vtk_m_filter_Gradient_XGC_cxx

#include <vtkm/filter/Gradient.h>
#include <vtkm/filter/Gradient.hxx>

namespace vtkm
{
namespace filter
{
template VTKM_FILTER_GRADIENT_TEMPLATE_EXPORT vtkm::cont::DataSet Gradient::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Vec<float, 3>, vtkm::cont::StorageTagXGCCoordinates>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);

template VTKM_FILTER_GRADIENT_TEMPLATE_EXPORT vtkm::cont::DataSet Gradient::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Vec<double, 3>, vtkm::cont::StorageTagXGCCoordinates>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
}
}

#endif
