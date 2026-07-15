//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ArrayHandleCompositeVector.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

template<typename ArrayHandleType>
void CheckArray(ArrayHandleType array)
{
  viskores::cont::printSummary_ArrayHandle(array, std::cout);
  std::cout << std::endl;
  typename ArrayHandleType::ReadPortalType portal = array.ReadPortal();

  // [(0,3,2,0), (1,1,7,0), (2,4,1,0), (3,1,8,0), (4,5,2,0)].
  VISKORES_TEST_ASSERT(test_equal(portal.Get(0), viskores::make_Vec(0, 3, 2, 0)),
                       "Bad value in array.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(1), viskores::make_Vec(1, 1, 7, 0)),
                       "Bad value in array.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(2), viskores::make_Vec(2, 4, 1, 0)),
                       "Bad value in array.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(3), viskores::make_Vec(3, 1, 8, 0)),
                       "Bad value in array.");
}

void ArrayHandleCompositeVectorBasic()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleCompositeVectorBasic
  ////
  // Create an array with [0, 1, 2, 3, 4]
  using ArrayType1 = viskores::cont::ArrayHandleIndex;
  ArrayType1 array1(5);

  // Create an array with [3, 1, 4, 1, 5]
  using ArrayType2 = viskores::cont::ArrayHandle<viskores::Id>;
  ArrayType2 array2 = viskores::cont::make_ArrayHandle<viskores::Id>({ 3, 1, 4, 1, 5 });

  // Create an array with [2, 7, 1, 8, 2]
  using ArrayType3 = viskores::cont::ArrayHandle<viskores::Id>;
  ArrayType3 array3 = viskores::cont::make_ArrayHandle<viskores::Id>({ 2, 7, 1, 8, 2 });

  // Create an array with [0, 0, 0, 0]
  using ArrayType4 = viskores::cont::ArrayHandleConstant<viskores::Id>;
  ArrayType4 array4(0, 5);

  // Use ArrayhandleCompositeVector to create the array
  // [(0,3,2,0), (1,1,7,0), (2,4,1,0), (3,1,8,0), (4,5,2,0)].
  using CompositeArrayType = viskores::cont::
    ArrayHandleCompositeVector<ArrayType1, ArrayType2, ArrayType3, ArrayType4>;
  CompositeArrayType compositeArray(array1, array2, array3, array4);
  ////
  //// END-EXAMPLE ArrayHandleCompositeVectorBasic
  ////
  CheckArray(compositeArray);

  CheckArray(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandleCompositeVector
    ////
    viskores::cont::make_ArrayHandleCompositeVector(array1, array2, array3, array4)
    ////
    //// END-EXAMPLE MakeArrayHandleCompositeVector
    ////
  );
}

void Test()
{
  ArrayHandleCompositeVectorBasic();
}

} // anonymous namespace

int GuideExampleArrayHandleCompositeVector(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
