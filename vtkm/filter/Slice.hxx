//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Slice_hxx
#define vtk_m_filter_Slice_hxx

#include <vtkm/cont/ArrayHandleTransform.h>

namespace vtkm
{
namespace filter
{

template <typename DerivedPolicy>
vtkm::cont::DataSet Slice::DoExecute(const vtkm::cont::DataSet& input,
                                     vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  const auto& coords = input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::DataSet result;
  auto impFuncEval =
    vtkm::ImplicitFunctionValueFunctor<vtkm::ImplicitFunctionGeneral>(this->Function);
  auto sliceScalars =
    vtkm::cont::make_ArrayHandleTransform(coords.GetDataAsMultiplexer(), impFuncEval);
  auto field = vtkm::cont::make_FieldPoint("sliceScalars", sliceScalars);

  this->ContourFilter.SetIsoValue(0.0);
  result =
    this->ContourFilter.DoExecute(input, sliceScalars, vtkm::filter::FieldMetadata(field), policy);

  return result;
}

}
} // vtkm::filter

#endif // vtk_m_filter_Slice_hxx
