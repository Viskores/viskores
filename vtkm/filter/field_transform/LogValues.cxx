//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/field_transform/LogValues.h>
#include <vtkm/filter/field_transform/worklet/LogValues.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
VTKM_CONT vtkm::cont::DataSet LogValues::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> logField;
  auto resolveType = [&](const auto& concrete) {
    switch (this->BaseValue)
    {
      case LogBase::E:
      {
        this->Invoke(vtkm::worklet::detail::LogFunWorklet<vtkm::Log>{ this->GetMinValue() },
                     concrete,
                     logField);
        break;
      }
      case LogBase::TWO:
      {
        this->Invoke(vtkm::worklet::detail::LogFunWorklet<vtkm::Log2>{ this->GetMinValue() },
                     concrete,
                     logField);
        break;
      }
      case LogBase::TEN:
      {
        this->Invoke(vtkm::worklet::detail::LogFunWorklet<vtkm::Log10>{ this->GetMinValue() },
                     concrete,
                     logField);
        break;
      }
      default:
      {
        throw vtkm::cont::ErrorFilterExecution("Unsupported base value.");
      }
    }
  };
  const auto& inField = this->GetFieldFromDataSet(inDataSet);
  this->CastAndCallScalarField(inField, resolveType);

  return this->CreateResultField(
    inDataSet, this->GetOutputFieldName(), inField.GetAssociation(), logField);
}
} // namespace field_transform
} // namespace vtkm::filter
} // namespace vtkm
