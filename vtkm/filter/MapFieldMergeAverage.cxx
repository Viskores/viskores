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
  template <typename BaseComponentType>
  void operator()(BaseComponentType,
                  const vtkm::cont::UnknownArrayHandle& input,
                  const vtkm::worklet::internal::KeysBase& keys,
                  vtkm::cont::UnknownArrayHandle& output,
                  bool& called) const
  {
    if (!input.IsBaseComponentType<BaseComponentType>())
    {
      return;
    }

    output = input.NewInstanceBasic();
    output.Allocate(keys.GetInputRange());

    vtkm::IdComponent numComponents = input.GetNumberOfComponentsFlat();
    for (vtkm::IdComponent cIndex = 0; cIndex < numComponents; ++cIndex)
    {
      vtkm::worklet::AverageByKey::Run(
        keys,
        input.ExtractComponent<BaseComponentType>(cIndex, vtkm::CopyFlag::On),
        output.ExtractComponent<BaseComponentType>(cIndex, vtkm::CopyFlag::Off));
    }

    called = true;
  }
};

} // anonymous namespace

bool vtkm::filter::MapFieldMergeAverage(const vtkm::cont::Field& inputField,
                                        const vtkm::worklet::internal::KeysBase& keys,
                                        vtkm::cont::Field& outputField)
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  vtkm::cont::VariantArrayHandle outputArray;
  bool calledMap = false;
  vtkm::ListForEach(DoMapFieldMerge{},
                    vtkm::TypeListScalarAll{},
                    inputField.GetData(),
                    keys,
                    outputArray,
                    calledMap);
  if (calledMap)
  {
    outputField = vtkm::cont::Field(inputField.GetName(), inputField.GetAssociation(), outputArray);
  }
  else
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Faild to map field " << inputField.GetName());
  }
  return calledMap;
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
