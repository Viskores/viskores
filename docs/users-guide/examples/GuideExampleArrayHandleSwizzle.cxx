//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleSwizzle.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template<typename ArrayHandleType>
void CheckArray(const ArrayHandleType array)
{
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == 3, "Permuted array has wrong size.");

  auto portal = array.ReadPortal();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == 3, "Permuted portal has wrong size.");

  VTKM_TEST_ASSERT(test_equal(portal.Get(0), vtkm::Vec3f_64(0.2, 0.0, 0.3)),
                   "Permuted array has wrong value.");
  VTKM_TEST_ASSERT(test_equal(portal.Get(1), vtkm::Vec3f_64(1.2, 1.0, 1.3)),
                   "Permuted array has wrong value.");
  VTKM_TEST_ASSERT(test_equal(portal.Get(2), vtkm::Vec3f_64(2.2, 2.0, 2.3)),
                   "Permuted array has wrong value.");
}

void Test()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleSwizzle
  ////
  using ValueArrayType = vtkm::cont::ArrayHandle<vtkm::Vec4f_64>;

  // Create array with values
  // [ (0.0, 0.1, 0.2, 0.3), (1.0, 1.1, 1.2, 1.3), (2.0, 2.1, 2.2, 2.3) ]
  ValueArrayType valueArray;
  valueArray.Allocate(3);
  auto valuePortal = valueArray.WritePortal();
  valuePortal.Set(0, vtkm::make_Vec(0.0, 0.1, 0.2, 0.3));
  valuePortal.Set(1, vtkm::make_Vec(1.0, 1.1, 1.2, 1.3));
  valuePortal.Set(2, vtkm::make_Vec(2.0, 2.1, 2.2, 2.3));

  // Use ArrayHandleSwizzle to make an array of Vec-3 with x,y,z,w swizzled to z,x,w
  // [ (0.2, 0.0, 0.3), (1.2, 1.0, 1.3), (2.2, 2.0, 2.3) ]
  vtkm::cont::ArrayHandleSwizzle<ValueArrayType, 3> swizzledArray(
    valueArray, vtkm::IdComponent3(2, 0, 3));
  ////
  //// END-EXAMPLE ArrayHandleSwizzle
  ////
  CheckArray(swizzledArray);

  CheckArray(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandleSwizzle
    ////
    vtkm::cont::make_ArrayHandleSwizzle(valueArray, 2, 0, 3)
    ////
    //// END-EXAMPLE MakeArrayHandleSwizzle
    ////
  );
}

} // anonymous namespace

int GuideExampleArrayHandleSwizzle(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
