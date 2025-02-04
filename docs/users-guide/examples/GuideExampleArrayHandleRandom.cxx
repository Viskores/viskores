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
#include <viskores/cont/ArrayHandleRandomStandardNormal.h>
#include <viskores/cont/ArrayHandleRandomUniformBits.h>
#include <viskores/cont/ArrayHandleRandomUniformReal.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

void Test()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleRandomUniformBits
  ////
  // Create an array containing a sequence of random bits seeded
  // by std::random_device.
  viskores::cont::ArrayHandleRandomUniformBits randomArray(50);
  // Create an array containing a sequence of random bits with
  // a user supplied seed.
  viskores::cont::ArrayHandleRandomUniformBits randomArraySeeded(50, { 123 });
  ////
  //// END-EXAMPLE ArrayHandleRandomUniformBits
  ////

  ////
  //// BEGIN-EXAMPLE ArrayHandleRandomUniformBitsFunctional
  ////
  // ArrayHandleRandomUniformBits is functional, it returns
  // the same value for the same entry is accessed.
  auto r0 = randomArray.ReadPortal().Get(5);
  auto r1 = randomArray.ReadPortal().Get(5);
  assert(r0 == r1);
  ////
  //// END-EXAMPLE ArrayHandleRandomUniformBitsFunctional
  ////
  // In case assert is an empty expression.
  VISKORES_TEST_ASSERT(r0 == r1);

  ////
  //// BEGIN-EXAMPLE ArrayHandleRandomUniformBitsIteration
  ////
  // Create a new insance of ArrayHandleRandomUniformBits
  // for each set of random bits.
  viskores::cont::ArrayHandleRandomUniformBits randomArray0(50, { 0 });
  viskores::cont::ArrayHandleRandomUniformBits randomArray1(50, { 1 });
  assert(randomArray0.ReadPortal().Get(5) != randomArray1.ReadPortal().Get(5));
  ////
  //// END-EXAMPLE ArrayHandleRandomUniformBitsIteration
  ////
  // In case assert is an empty expression.
  VISKORES_TEST_ASSERT(randomArray0.ReadPortal().Get(5) != randomArray1.ReadPortal().Get(5));

  {
    ////
    //// BEGIN-EXAMPLE ArrayHandleRandomUniformReal
    ////
    constexpr viskores::Id NumPoints = 50;
    auto randomPointsInBox = viskores::cont::make_ArrayHandleCompositeVector(
      viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>(NumPoints),
      viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>(NumPoints),
      viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault>(NumPoints));
    ////
    //// END-EXAMPLE ArrayHandleRandomUniformReal
    ////

    VISKORES_TEST_ASSERT(randomPointsInBox.GetNumberOfValues() == NumPoints);
    auto portal = randomPointsInBox.ReadPortal();
    for (viskores::Id idx = 0; idx < NumPoints; ++idx)
    {
      viskores::Vec3f value = portal.Get(idx);
      VISKORES_TEST_ASSERT((value[0] >= 0) && (value[0] <= 1));
      VISKORES_TEST_ASSERT((value[1] >= 0) && (value[1] <= 1));
      VISKORES_TEST_ASSERT((value[2] >= 0) && (value[2] <= 1));
    }
  }

  {
    ////
    //// BEGIN-EXAMPLE ArrayHandleRandomStandardNormal
    ////
    constexpr viskores::Id NumPoints = 50;
    auto randomPointsInGaussian = viskores::cont::make_ArrayHandleCompositeVector(
      viskores::cont::ArrayHandleRandomStandardNormal<viskores::FloatDefault>(NumPoints),
      viskores::cont::ArrayHandleRandomStandardNormal<viskores::FloatDefault>(NumPoints),
      viskores::cont::ArrayHandleRandomStandardNormal<viskores::FloatDefault>(NumPoints));
    ////
    //// END-EXAMPLE ArrayHandleRandomStandardNormal
    ////

    VISKORES_TEST_ASSERT(randomPointsInGaussian.GetNumberOfValues() == NumPoints);
  }
}

} // anonymous namespace

int GuideExampleArrayHandleRandom(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
