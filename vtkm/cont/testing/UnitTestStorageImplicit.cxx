//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageImplicit.h>

#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

#include <vtkm/cont/testing/Testing.h>

#if defined(VTKM_STORAGE)
#undef VTKM_STORAGE
#endif

#define VTKM_STORAGE VTKM_STORAGE_ERROR

namespace
{

const vtkm::Id ARRAY_SIZE = 10;

template <typename T>
struct TestImplicitStorage
{
  using ValueType = T;
  ValueType Temp;

  VTKM_EXEC_CONT
  TestImplicitStorage()
    : Temp(1)
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return ARRAY_SIZE; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id vtkmNotUsed(index)) const { return Temp; }
};

template <typename T>
struct TemplatedTests
{
  using StorageTagType = vtkm::cont::StorageTagImplicit<TestImplicitStorage<T>>;
  using StorageType = vtkm::cont::internal::Storage<T, StorageTagType>;

  using ValueType = typename StorageType::ValueType;
  using PortalType = typename StorageType::PortalType;

  void BasicAllocation()
  {
    StorageType arrayStorage;

    // The implicit portal defined for this test always returns ARRAY_SIZE for the
    // number of values. We should get that.
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE,
                     "Implicit Storage GetNumberOfValues returned wrong size.");

    // Make sure you can allocate and shrink to any value <= the reported portal size.
    arrayStorage.Allocate(ARRAY_SIZE / 2);
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE / 2,
                     "Cannot re-Allocate array to half size.");

    arrayStorage.Allocate(0);
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == 0, "Cannot re-Allocate array to zero.");

    arrayStorage.Allocate(ARRAY_SIZE);
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE,
                     "Cannot re-Allocate array to original size.");

    arrayStorage.Shrink(ARRAY_SIZE / 2);
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE / 2,
                     "Cannot Shrink array to half size.");

    arrayStorage.Shrink(0);
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == 0, "Cannot Shrink array to zero.");

    arrayStorage.Shrink(ARRAY_SIZE);
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == ARRAY_SIZE,
                     "Cannot Shrink array to original size.");

    //verify that calling ReleaseResources doesn't throw an exception
    arrayStorage.ReleaseResources();

    //verify that you can allocate after releasing resources.
    arrayStorage.Allocate(ARRAY_SIZE);
  }

  void BasicAccess()
  {
    TestImplicitStorage<T> portal;
    vtkm::cont::ArrayHandle<T, StorageTagType> implictHandle(portal);
    VTKM_TEST_ASSERT(implictHandle.GetNumberOfValues() == ARRAY_SIZE, "handle has wrong size");
    VTKM_TEST_ASSERT(implictHandle.GetPortalConstControl().Get(0) == T(1),
                     "portals first values should be 1");
  }

  void operator()()
  {
    BasicAllocation();
    BasicAccess();
  }
};

struct TestFunctor
{
  template <typename T>
  void operator()(T) const
  {
    TemplatedTests<T> tests;
    tests();
  }
};

void TestStorageBasic()
{
  vtkm::testing::Testing::TryTypes(TestFunctor());
}

} // Anonymous namespace

int UnitTestStorageImplicit(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStorageBasic, argc, argv);
}
