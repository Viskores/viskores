//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>

namespace
{

struct TestEqualArrayHandleType2
{
  template <typename T, typename FirstArrayType>
  void operator()(T,
                  const FirstArrayType& array1,
                  const vtkm::cont::UnknownArrayHandle& array2,
                  vtkm::IdComponent cIndex,
                  TestEqualResult& result,
                  bool& called) const
  {
    if (!array2.IsBaseComponentType<T>())
    {
      return;
    }

    result = test_equal_ArrayHandles(array1, array2.ExtractComponent<T>(cIndex));

    called = true;
  }
};

struct TestEqualArrayHandleType1
{
  template <typename T>
  void operator()(T,
                  const vtkm::cont::UnknownArrayHandle& array1,
                  const vtkm::cont::UnknownArrayHandle& array2,
                  TestEqualResult& result,
                  bool& called) const
  {
    if (!array1.IsBaseComponentType<T>())
    {
      return;
    }

    for (vtkm::IdComponent cIndex = 0; cIndex < array1.GetNumberOfComponentsFlat(); ++cIndex)
    {
      vtkm::ListForEach(TestEqualArrayHandleType2{},
                        vtkm::TypeListScalarAll{},
                        array1.ExtractComponent<T>(cIndex),
                        array2,
                        cIndex,
                        result,
                        called);
      if (!result)
      {
        break;
      }
    }
  }
};

} // anonymous namespace

TestEqualResult test_equal_ArrayHandles(const vtkm::cont::UnknownArrayHandle& array1,
                                        const vtkm::cont::UnknownArrayHandle& array2)
{
  TestEqualResult result;

  if (array1.GetNumberOfComponentsFlat() != array2.GetNumberOfComponentsFlat())
  {
    result.PushMessage("Arrays have different numbers of components.");
    return result;
  }

  bool called = false;

  vtkm::ListForEach(
    TestEqualArrayHandleType1{}, vtkm::TypeListScalarAll{}, array1, array2, result, called);

  if (!called)
  {
    result.PushMessage("Could not base component type for " + array1.GetBaseComponentTypeName() +
                       " or " + array2.GetBaseComponentTypeName());
  }
  return result;
}
