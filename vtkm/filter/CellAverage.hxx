//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_CellAverage_hxx
#define vtk_m_filter_CellAverage_hxx

#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>

namespace vtkm
{
namespace filter
{
//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet CellAverage::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& inField,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  if (!fieldMetadata.IsPointField())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  vtkm::cont::DynamicCellSet cellSet = input.GetCellSet();

  //todo: we need to ask the policy what storage type we should be using
  //If the input is implicit, we should know what to fall back to
  vtkm::cont::ArrayHandle<T> outArray;

  this->Invoke(this->Worklet, vtkm::filter::ApplyPolicyCellSet(cellSet, policy), inField, outArray);

  std::string outputName = this->GetOutputFieldName();
  if (outputName.empty())
  {
    // Default name is name of input.
    outputName = fieldMetadata.GetName();
  }

  return CreateResultFieldCell(input, outArray, outputName);
}
}
} // namespace vtkm::filter

#endif
