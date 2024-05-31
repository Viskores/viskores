//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleExtractComponent.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template<typename ArrayHandleType>
void CheckArray(const ArrayHandleType array)
{
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == 3, "Permuted array has wrong size.");

  auto portal = array.ReadPortal();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == 3, "Permuted portal has wrong size.");

  VTKM_TEST_ASSERT(test_equal(portal.Get(0), 0.2), "Permuted array has wrong value.");
  VTKM_TEST_ASSERT(test_equal(portal.Get(1), 1.2), "Permuted array has wrong value.");
  VTKM_TEST_ASSERT(test_equal(portal.Get(2), 2.2), "Permuted array has wrong value.");
}

void Test()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleExtractComponent
  ////
  using ValueArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f_64>;

  // Create array with values [ (0.0, 0.1, 0.2), (1.0, 1.1, 1.2), (2.0, 2.1, 2.2) ]
  ValueArrayType valueArray;
  valueArray.Allocate(3);
  auto valuePortal = valueArray.WritePortal();
  valuePortal.Set(0, vtkm::make_Vec(0.0, 0.1, 0.2));
  valuePortal.Set(1, vtkm::make_Vec(1.0, 1.1, 1.2));
  valuePortal.Set(2, vtkm::make_Vec(2.0, 2.1, 2.2));

  // Use ArrayHandleExtractComponent to make an array = [1.3, 2.3, 3.3].
  vtkm::cont::ArrayHandleExtractComponent<ValueArrayType> extractedComponentArray(
    valueArray, 2);
  ////
  //// END-EXAMPLE ArrayHandleExtractComponent
  ////
  CheckArray(extractedComponentArray);

  CheckArray(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandleExtractComponent
    ////
    vtkm::cont::make_ArrayHandleExtractComponent(valueArray, 2)
    ////
    //// END-EXAMPLE MakeArrayHandleExtractComponent
    ////
  );
}

} // anonymous namespace

int GuideExampleArrayHandleExtractComponent(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
