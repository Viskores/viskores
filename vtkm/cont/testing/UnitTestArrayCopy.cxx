//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleIndex.h>

#include <vtkm/TypeTraits.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename PortalType>
void TestValues(const PortalType& portal)
{
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE, "Wrong array size.");

  for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    VTKM_TEST_ASSERT(test_equal(portal.Get(index), index), "Got bad value.");
  }
}

template <typename ValueType>
void TryCopy()
{
  std::cout << "Trying type: " << vtkm::testing::TypeName<ValueType>::Name() << std::endl;

  vtkm::cont::ArrayHandleIndex input(ARRAY_SIZE);
  vtkm::cont::ArrayHandle<ValueType> output;

  vtkm::cont::ArrayCopy(input, output);

  TestValues(output.GetPortalConstControl());
}

void TestArrayCopy()
{
  TryCopy<vtkm::Id>();
  TryCopy<vtkm::IdComponent>();
  TryCopy<vtkm::Float32>();
}

} // anonymous namespace

int UnitTestArrayCopy(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestArrayCopy, argc, argv);
}
