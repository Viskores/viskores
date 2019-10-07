//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_CellMeasures_hxx
#define vtk_m_filter_CellMeasures_hxx

#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename IntegrationType>
inline VTKM_CONT CellMeasures<IntegrationType>::CellMeasures()
  : vtkm::filter::FilterCell<CellMeasures<IntegrationType>>()
{
  this->SetUseCoordinateSystemAsField(true);
}

//-----------------------------------------------------------------------------
template <typename IntegrationType>
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet CellMeasures<IntegrationType>::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& points,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (fieldMeta.IsPointField() == false)
  {
    throw vtkm::cont::ErrorFilterExecution("CellMeasures expects point field input.");
  }

  const auto& cellset = input.GetCellSet();
  vtkm::cont::ArrayHandle<T> outArray;

  this->Invoke(vtkm::worklet::CellMeasure<IntegrationType>{},
               vtkm::filter::ApplyPolicyCellSet(cellset, policy),
               points,
               outArray);

  std::string outputName = this->GetCellMeasureName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = "measure";
  }
  return CreateResultFieldCell(input, outArray, outputName);
}
}
} // namespace vtkm::filter

#endif
