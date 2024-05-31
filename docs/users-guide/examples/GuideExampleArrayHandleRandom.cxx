//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleRandomStandardNormal.h>
#include <vtkm/cont/ArrayHandleRandomUniformBits.h>
#include <vtkm/cont/ArrayHandleRandomUniformReal.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

void Test()
{
  ////
  //// BEGIN-EXAMPLE ArrayHandleRandomUniformBits
  ////
  // Create an array containing a sequence of random bits seeded
  // by std::random_device.
  vtkm::cont::ArrayHandleRandomUniformBits randomArray(50);
  // Create an array containing a sequence of random bits with
  // a user supplied seed.
  vtkm::cont::ArrayHandleRandomUniformBits randomArraySeeded(50, { 123 });
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
  VTKM_TEST_ASSERT(r0 == r1);

  ////
  //// BEGIN-EXAMPLE ArrayHandleRandomUniformBitsIteration
  ////
  // Create a new insance of ArrayHandleRandomUniformBits
  // for each set of random bits.
  vtkm::cont::ArrayHandleRandomUniformBits randomArray0(50, { 0 });
  vtkm::cont::ArrayHandleRandomUniformBits randomArray1(50, { 1 });
  assert(randomArray0.ReadPortal().Get(5) != randomArray1.ReadPortal().Get(5));
  ////
  //// END-EXAMPLE ArrayHandleRandomUniformBitsIteration
  ////
  // In case assert is an empty expression.
  VTKM_TEST_ASSERT(randomArray0.ReadPortal().Get(5) != randomArray1.ReadPortal().Get(5));

  {
    ////
    //// BEGIN-EXAMPLE ArrayHandleRandomUniformReal
    ////
    constexpr vtkm::Id NumPoints = 50;
    auto randomPointsInBox = vtkm::cont::make_ArrayHandleCompositeVector(
      vtkm::cont::ArrayHandleRandomUniformReal<vtkm::FloatDefault>(NumPoints),
      vtkm::cont::ArrayHandleRandomUniformReal<vtkm::FloatDefault>(NumPoints),
      vtkm::cont::ArrayHandleRandomUniformReal<vtkm::FloatDefault>(NumPoints));
    ////
    //// END-EXAMPLE ArrayHandleRandomUniformReal
    ////

    VTKM_TEST_ASSERT(randomPointsInBox.GetNumberOfValues() == NumPoints);
    auto portal = randomPointsInBox.ReadPortal();
    for (vtkm::Id idx = 0; idx < NumPoints; ++idx)
    {
      vtkm::Vec3f value = portal.Get(idx);
      VTKM_TEST_ASSERT((value[0] >= 0) && (value[0] <= 1));
      VTKM_TEST_ASSERT((value[1] >= 0) && (value[1] <= 1));
      VTKM_TEST_ASSERT((value[2] >= 0) && (value[2] <= 1));
    }
  }

  {
    ////
    //// BEGIN-EXAMPLE ArrayHandleRandomStandardNormal
    ////
    constexpr vtkm::Id NumPoints = 50;
    auto randomPointsInGaussian = vtkm::cont::make_ArrayHandleCompositeVector(
      vtkm::cont::ArrayHandleRandomStandardNormal<vtkm::FloatDefault>(NumPoints),
      vtkm::cont::ArrayHandleRandomStandardNormal<vtkm::FloatDefault>(NumPoints),
      vtkm::cont::ArrayHandleRandomStandardNormal<vtkm::FloatDefault>(NumPoints));
    ////
    //// END-EXAMPLE ArrayHandleRandomStandardNormal
    ////

    VTKM_TEST_ASSERT(randomPointsInGaussian.GetNumberOfValues() == NumPoints);
  }
}

} // anonymous namespace

int GuideExampleArrayHandleRandom(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
