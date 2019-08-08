//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

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
  VTKM_ASSERT(fieldMeta.IsPointField());
  const auto& cellset = input.GetCellSet(this->GetActiveCellSetIndex());
  vtkm::cont::ArrayHandle<T> outArray;

  this->Invoke(vtkm::worklet::CellMeasure<IntegrationType>{},
               vtkm::filter::ApplyPolicy(cellset, policy),
               points,
               outArray);

  vtkm::cont::DataSet result;
  std::string outputName = this->GetCellMeasureName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = "measure";
  }
  result = CreateResult(input, outArray, outputName, vtkm::cont::Field::Association::CELL_SET);

  return result;
}
}
} // namespace vtkm::filter
