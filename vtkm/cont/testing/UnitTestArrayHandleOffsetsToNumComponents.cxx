//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandleOffsetsToNumComponents.h>

#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSetExplicit.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 20;

template <typename OffsetsArray, typename ExpectedNumComponents>
void TestOffsetsToNumComponents(const OffsetsArray& offsetsArray,
                                const ExpectedNumComponents& expectedNumComponents)
{
  VTKM_TEST_ASSERT(offsetsArray.GetNumberOfValues() ==
                   expectedNumComponents.GetNumberOfValues() + 1);

  auto numComponents = vtkm::cont::make_ArrayHandleOffsetsToNumComponents(offsetsArray);
  VTKM_TEST_ASSERT(numComponents.GetNumberOfValues() == expectedNumComponents.GetNumberOfValues());
  VTKM_TEST_ASSERT(
    test_equal_portals(numComponents.ReadPortal(), expectedNumComponents.ReadPortal()));
}

void TryNormalOffsets()
{
  std::cout << "Normal offset array." << std::endl;

  vtkm::cont::ArrayHandle<vtkm::IdComponent> numComponents;
  numComponents.Allocate(ARRAY_SIZE);
  auto numComponentsPortal = numComponents.WritePortal();
  for (vtkm::IdComponent i = 0; i < ARRAY_SIZE; ++i)
  {
    numComponentsPortal.Set(i, i % 5);
  }

  auto offsets = vtkm::cont::ConvertNumIndicesToOffsets(numComponents);
  TestOffsetsToNumComponents(offsets, numComponents);
}

void TryFancyOffsets()
{
  std::cout << "Fancy offset array." << std::endl;
  vtkm::cont::ArrayHandleCounting<vtkm::Id> offsets(0, 3, ARRAY_SIZE + 1);
  TestOffsetsToNumComponents(offsets,
                             vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(3, ARRAY_SIZE));
}

void Run()
{
  TryNormalOffsets();
  TryFancyOffsets();
}

} // anonymous namespace

int UnitTestArrayHandleOffsetsToNumComponents(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
