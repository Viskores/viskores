//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
////////////////////////// **** DO NOT EDIT THIS FILE!!! ****
// This file is automatically generated by ClipWithImplicitFunctionExternInstantiations.h.in
// clang-format off

#ifndef vtk_m_filter_ClipWithImplicitFunctionExternInstantiations_h
#define vtk_m_filter_ClipWithImplicitFunctionExternInstantiations_h

#include <vtkm/filter/ClipWithImplicitFunction.h>
#include <vtkm/filter/ClipWithImplicitFunction.hxx>
#include <vtkm/filter/Instantiations.h>

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/UnknownCellSet.h>

namespace vtkm
{
namespace filter
{

#ifndef vtkm_filter_ClipWithImplicitFunction_cxx

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_EXTRA_EXPORT bool ClipWithImplicitFunction::DoMapField(
  vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::UInt8>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_EXTRA_EXPORT bool ClipWithImplicitFunction::DoMapField(
  vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Int32>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_EXTRA_EXPORT bool ClipWithImplicitFunction::DoMapField(
  vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Int64>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_EXTRA_EXPORT bool ClipWithImplicitFunction::DoMapField(
  vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Float32>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_EXTRA_EXPORT bool ClipWithImplicitFunction::DoMapField(
  vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Float64>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_EXTRA_EXPORT bool ClipWithImplicitFunction::DoMapField(
  vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Vec3f_32>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END

VTKM_INSTANTIATION_BEGIN
extern template VTKM_FILTER_EXTRA_EXPORT bool ClipWithImplicitFunction::DoMapField(
  vtkm::cont::DataSet&,
  const vtkm::cont::ArrayHandle<vtkm::Vec3f_64>&,
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>);
VTKM_INSTANTIATION_END


#endif

}
}

#endif
