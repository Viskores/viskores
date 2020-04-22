//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/internal/ArrayPortalBasic.h>
#include <vtkm/internal/ArrayPortalHelpers.h>

#include <vtkm/StaticAssert.h>

#include <vtkm/testing/Testing.h>

#include <array>

namespace
{

static constexpr vtkm::Id ARRAY_SIZE = 10;

struct TypeTest
{
  template <typename T>
  void operator()(T) const
  {
    std::cout << "Creating data" << std::endl;
    std::array<T, ARRAY_SIZE> array;
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      array[static_cast<std::size_t>(index)] = TestValue(index, T{});
    }

    std::cout << "Construct read portal" << std::endl;
    using ReadPortalType = vtkm::internal::ArrayPortalBasicRead<T>;
    VTKM_STATIC_ASSERT((vtkm::internal::PortalSupportsGets<ReadPortalType>::value));
    VTKM_STATIC_ASSERT((!vtkm::internal::PortalSupportsSets<ReadPortalType>::value));
    VTKM_STATIC_ASSERT((vtkm::internal::PortalSupportsIterators<ReadPortalType>::value));

    ReadPortalType readPortal(array.data(), ARRAY_SIZE);
    VTKM_TEST_ASSERT(readPortal.GetNumberOfValues() == ARRAY_SIZE);
    VTKM_TEST_ASSERT(readPortal.GetArray() == array.data());
    VTKM_TEST_ASSERT(readPortal.GetIteratorBegin() == array.data());
    VTKM_TEST_ASSERT(readPortal.GetIteratorEnd() == array.data() + ARRAY_SIZE);

    std::cout << "Check initial read data" << std::endl;
    CheckPortal(readPortal);

    std::cout << "Construct write portal" << std::endl;
    using WritePortalType = vtkm::internal::ArrayPortalBasicWrite<T>;
    VTKM_STATIC_ASSERT((vtkm::internal::PortalSupportsGets<WritePortalType>::value));
    VTKM_STATIC_ASSERT((vtkm::internal::PortalSupportsSets<WritePortalType>::value));
    VTKM_STATIC_ASSERT((vtkm::internal::PortalSupportsIterators<WritePortalType>::value));

    WritePortalType writePortal(array.data(), ARRAY_SIZE);
    VTKM_TEST_ASSERT(writePortal.GetNumberOfValues() == ARRAY_SIZE);
    VTKM_TEST_ASSERT(writePortal.GetArray() == array.data());
    VTKM_TEST_ASSERT(writePortal.GetIteratorBegin() == array.data());
    VTKM_TEST_ASSERT(writePortal.GetIteratorEnd() == array.data() + ARRAY_SIZE);

    std::cout << "Check initial write data" << std::endl;
    CheckPortal(writePortal);

    std::cout << "Write new data" << std::endl;
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      writePortal.Set(index, TestValue(index + 10, T{}));
    }

    std::cout << "Check data written to array." << std::endl;
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      VTKM_TEST_ASSERT(
        test_equal(array[static_cast<std::size_t>(index)], TestValue(index + 10, T{})));
    }
  }
};

void Run()
{
  vtkm::testing::Testing::TryTypes(TypeTest{});
}

} // anonymous namespace

int UnitTestArrayPortalBasic(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(Run, argc, argv);
}
