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

#include <viskores/cont/ArrayHandlePermutation.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

template<typename ArrayHandleType>
void CheckArray1(const ArrayHandleType array)
{
  VISKORES_TEST_ASSERT(array.GetNumberOfValues() == 3, "Permuted array has wrong size.");

  typename ArrayHandleType::ReadPortalType portal = array.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == 3,
                       "Permuted portal has wrong size.");

  VISKORES_TEST_ASSERT(test_equal(portal.Get(0), 0.3),
                       "Permuted array has wrong value.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(1), 0.0),
                       "Permuted array has wrong value.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(2), 0.1),
                       "Permuted array has wrong value.");
}

template<typename ArrayHandleType>
void CheckArray2(const ArrayHandleType array)
{
  VISKORES_TEST_ASSERT(array.GetNumberOfValues() == 5, "Permuted array has wrong size.");

  typename ArrayHandleType::ReadPortalType portal = array.ReadPortal();
  VISKORES_TEST_ASSERT(portal.GetNumberOfValues() == 5,
                       "Permuted portal has wrong size.");

  VISKORES_TEST_ASSERT(test_equal(portal.Get(0), 0.1),
                       "Permuted array has wrong value.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(1), 0.2),
                       "Permuted array has wrong value.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(2), 0.2),
                       "Permuted array has wrong value.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(3), 0.3),
                       "Permuted array has wrong value.");
  VISKORES_TEST_ASSERT(test_equal(portal.Get(4), 0.0),
                       "Permuted array has wrong value.");
}

void Test()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandlePermutation
  ////
  using IdArrayType = viskores::cont::ArrayHandle<viskores::Id>;
  using IdPortalType = IdArrayType::WritePortalType;

  using ValueArrayType = viskores::cont::ArrayHandle<viskores::Float64>;
  using ValuePortalType = ValueArrayType::WritePortalType;

  // Create array with values [0.0, 0.1, 0.2, 0.3]
  ValueArrayType valueArray;
  valueArray.Allocate(4);
  ValuePortalType valuePortal = valueArray.WritePortal();
  valuePortal.Set(0, 0.0);
  valuePortal.Set(1, 0.1);
  valuePortal.Set(2, 0.2);
  valuePortal.Set(3, 0.3);

  // Use ArrayHandlePermutation to make an array = [0.3, 0.0, 0.1].
  IdArrayType idArray1;
  idArray1.Allocate(3);
  IdPortalType idPortal1 = idArray1.WritePortal();
  idPortal1.Set(0, 3);
  idPortal1.Set(1, 0);
  idPortal1.Set(2, 1);
  viskores::cont::ArrayHandlePermutation<IdArrayType, ValueArrayType> permutedArray1(
    idArray1, valueArray);
  //// PAUSE-EXAMPLE
  CheckArray1(permutedArray1);
  //// RESUME-EXAMPLE

  // Use ArrayHandlePermutation to make an array = [0.1, 0.2, 0.2, 0.3, 0.0]
  IdArrayType idArray2;
  idArray2.Allocate(5);
  IdPortalType idPortal2 = idArray2.WritePortal();
  idPortal2.Set(0, 1);
  idPortal2.Set(1, 2);
  idPortal2.Set(2, 2);
  idPortal2.Set(3, 3);
  idPortal2.Set(4, 0);
  viskores::cont::ArrayHandlePermutation<IdArrayType, ValueArrayType> permutedArray2(
    idArray2, valueArray);
  //// PAUSE-EXAMPLE
  CheckArray2(permutedArray2);
  //// RESUME-EXAMPLE
  ////
  //// END-EXAMPLE ArrayHandlePermutation
  ////

  IdArrayType idArray = idArray2;
  CheckArray2(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandlePermutation
    ////
    viskores::cont::make_ArrayHandlePermutation(idArray, valueArray)
    ////
    //// END-EXAMPLE MakeArrayHandlePermutation
    ////
  );
}

} // anonymous namespace

int GuideExampleArrayHandlePermutation(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
