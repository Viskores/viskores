//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/MapFieldMergeAverage.h>

#include <vtkm/cont/Logging.h>

#include <vtkm/worklet/AverageByKey.h>

#include <vtkm/filter/PolicyDefault.h>

namespace
{

struct DoMapFieldMerge
{
  template <typename InputArrayType>
  void operator()(const InputArrayType& input,
                  const vtkm::worklet::internal::KeysBase& keys,
                  vtkm::cont::UnknownArrayHandle& output) const
  {
    using BaseComponentType = typename InputArrayType::ValueType::ComponentType;

    vtkm::worklet::AverageByKey::Run(
      keys, input, output.ExtractArrayFromComponents<BaseComponentType>(vtkm::CopyFlag::Off));
  }
};

} // anonymous namespace

bool vtkm::filter::MapFieldMergeAverage(const vtkm::cont::Field& inputField,
                                        const vtkm::worklet::internal::KeysBase& keys,
                                        vtkm::cont::Field& outputField)
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  vtkm::cont::UnknownArrayHandle outputArray = inputField.GetData().NewInstanceBasic();
  outputArray.Allocate(keys.GetInputRange());

  try
  {
    inputField.GetData().CastAndCallWithExtractedArray(DoMapFieldMerge{}, keys, outputArray);
    outputField = vtkm::cont::Field(inputField.GetName(), inputField.GetAssociation(), outputArray);
    return true;
  }
  catch (...)
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Faild to map field " << inputField.GetName());
    return false;
  }
}

bool vtkm::filter::MapFieldMergeAverage(const vtkm::cont::Field& inputField,
                                        const vtkm::worklet::internal::KeysBase& keys,
                                        vtkm::cont::DataSet& outputData)
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  vtkm::cont::Field outputField;
  bool success = vtkm::filter::MapFieldMergeAverage(inputField, keys, outputField);
  if (success)
  {
    outputData.AddField(outputField);
  }
  return success;
}
