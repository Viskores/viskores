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
  bool CalledMap = false;

  template <typename T, typename S>
  void operator()(const vtkm::cont::ArrayHandle<T, S>& inputArray,
                  const vtkm::worklet::internal::KeysBase& keys,
                  vtkm::cont::VariantArrayHandle& output)
  {
    vtkm::cont::ArrayHandle<T> outputArray = vtkm::worklet::AverageByKey::Run(keys, inputArray);
    output = vtkm::cont::VariantArrayHandle(outputArray);
    this->CalledMap = true;
  }
};

} // anonymous namespace

bool vtkm::filter::MapFieldMergeAverage(const vtkm::cont::Field& inputField,
                                        const vtkm::worklet::internal::KeysBase& keys,
                                        vtkm::cont::Field& outputField)
{
  vtkm::cont::VariantArrayHandle outputArray;
  DoMapFieldMerge functor;
  inputField.GetData().ResetTypes<vtkm::TypeListAll>().CastAndCall(
    vtkm::filter::PolicyDefault::StorageList{}, functor, keys, outputArray);
  if (functor.CalledMap)
  {
    outputField = vtkm::cont::Field(inputField.GetName(), inputField.GetAssociation(), outputArray);
  }
  else
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Warn, "Faild to map field " << inputField.GetName());
  }
  return functor.CalledMap;
}

bool vtkm::filter::MapFieldMergeAverage(const vtkm::cont::Field& inputField,
                                        const vtkm::worklet::internal::KeysBase& keys,
                                        vtkm::cont::DataSet& outputData)
{
  vtkm::cont::Field outputField;
  bool success = vtkm::filter::MapFieldMergeAverage(inputField, keys, outputField);
  if (success)
  {
    outputData.AddField(outputField);
  }
  return success;
}
