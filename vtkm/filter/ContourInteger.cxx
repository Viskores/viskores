//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ContourExecute_cxx
#define vtk_m_filter_ContourExecute_cxx

#include <vtkm/filter/Contour.h>

namespace vtkm
{
namespace filter
{
template VTKM_FILTER_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);

template VTKM_FILTER_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::UInt8, vtkm::cont::StorageTagVirtual>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);

template VTKM_FILTER_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Int8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);

template VTKM_FILTER_EXPORT vtkm::cont::DataSet Contour::DoExecute(
  const vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Int8, vtkm::cont::StorageTagVirtual>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
}
}

#endif
