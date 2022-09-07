//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================
#ifndef vtk_m_filter_ComputeMoments_hxx
#define vtk_m_filter_ComputeMoments_hxx

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/filter/image_processing/ComputeMoments.h>
#include <vtkm/filter/image_processing/worklet/ComputeMoments.h>

namespace vtkm
{
namespace filter
{
namespace image_processing
{
using SupportedTypes = vtkm::List<vtkm::Float32,
                                  vtkm::Float64,
                                  vtkm::Vec<vtkm::Float32, 2>,
                                  vtkm::Vec<vtkm::Float64, 2>,
                                  vtkm::Vec<vtkm::Float32, 3>,
                                  vtkm::Vec<vtkm::Float64, 3>,
                                  vtkm::Vec<vtkm::Float32, 4>,
                                  vtkm::Vec<vtkm::Float64, 4>,
                                  vtkm::Vec<vtkm::Float32, 6>,
                                  vtkm::Vec<vtkm::Float64, 6>,
                                  vtkm::Vec<vtkm::Float32, 9>,
                                  vtkm::Vec<vtkm::Float64, 9>>;

VTKM_CONT ComputeMoments::ComputeMoments()
{
  this->SetOutputFieldName("moments_");
}

VTKM_CONT vtkm::cont::DataSet ComputeMoments::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsPointField())
  {
    throw vtkm::cont::ErrorBadValue("Active field for ComputeMoments must be a point field.");
  }

  vtkm::cont::DataSet output = this->CreateResult(input);
  auto worklet = vtkm::worklet::moments::ComputeMoments(this->Radius, this->Spacing);

  auto resolveType = [&](const auto& concrete) {
    worklet.Run(input.GetCellSet(), concrete, this->Order, output);
  };
  field.GetData().CastAndCallForTypesWithFloatFallback<SupportedTypes, VTKM_DEFAULT_STORAGE_LIST>(
    resolveType);

  return output;
}
} // namespace image_processing
} // namespace filter
} // namespace vtkm
#endif //vtk_m_filter_ComputeMoments_hxx
