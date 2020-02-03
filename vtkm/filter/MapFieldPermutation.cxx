//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/MapFieldPermutation.h>

#include <vtkm/TypeList.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/Logging.h>

#include <vtkm/filter/PolicyDefault.h>

namespace
{

struct DoMapFieldPermutation
{
  bool CalledMap = false;

  template <typename T, typename S>
  void operator()(const vtkm::cont::ArrayHandle<T, S>& inputArray,
                  const vtkm::cont::ArrayHandle<vtkm::Id>& permutation,
                  vtkm::cont::VariantArrayHandle& output)
  {
    vtkm::cont::ArrayHandle<T> outputArray;
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandlePermutation(permutation, inputArray),
                          outputArray);
    output = vtkm::cont::VariantArrayHandle(outputArray);
    this->CalledMap = true;
  }
};

} // anonymous namespace

bool vtkm::filter::MapFieldPermutation(const vtkm::cont::Field& inputField,
                                       const vtkm::cont::ArrayHandle<vtkm::Id>& permutation,
                                       vtkm::cont::Field& outputField)
{
  vtkm::cont::VariantArrayHandle outputArray;
  DoMapFieldPermutation functor;
  inputField.GetData().ResetTypes<vtkm::TypeListAll>().CastAndCall(
    vtkm::filter::PolicyDefault::StorageList{}, functor, permutation, outputArray);
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

bool MapFieldPermutation(const vtkm::cont::Field& inputField,
                         const vtkm::cont::ArrayHandle<vtkm::Id>& permutation,
                         vtkm::cont::DataSet& outputData)
{
  vtkm::cont::Field outputField;
  bool success = vtkm::filter::MapFieldPermutation(inputField, permutation, outputField);
  if (success)
  {
    outputData.AddField(outputField);
  }
  return success;
}
