//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#define VTKM_STORAGE VTKM_STORAGE_ERROR

#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageImplicit.h>

#include <vtkm/cont/internal/IteratorFromArrayPortal.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

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
  vtkm::Id GetNumberOfValues() const { return 1; }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id vtkmNotUsed(index)) const { return Temp; }
};

const vtkm::Id ARRAY_SIZE = 1;

template <typename T>
struct TemplatedTests
{
  using StorageTagType = vtkm::cont::StorageTagImplicit<TestImplicitStorage<T>>;
  using StorageType = vtkm::cont::internal::Storage<T, StorageTagType>;

  using ValueType = typename StorageType::ValueType;
  using PortalType = typename StorageType::PortalType;
  using IteratorType = typename PortalType::IteratorType;

  void BasicAllocation()
  {
    StorageType arrayStorage;

    // The implicit portal defined for this test always returns 1 for the
    // number of values. We should get that.
    VTKM_TEST_ASSERT(arrayStorage.GetNumberOfValues() == 1,
                     "Implicit Storage GetNumberOfValues returned wrong size.");

    try
    {
      arrayStorage.Allocate(ARRAY_SIZE);
      VTKM_TEST_ASSERT(false == true, "Implicit Storage Allocate method didn't throw error.");
    }
    catch (vtkm::cont::ErrorBadValue&)
    {
    }

    try
    {
      arrayStorage.Shrink(ARRAY_SIZE);
      VTKM_TEST_ASSERT(true == false,
                       "Array shrink do a larger size was possible. This can't be allowed.");
    }
    catch (vtkm::cont::ErrorBadValue&)
    {
    }

    //verify that calling ReleaseResources doesn't throw an exception
    arrayStorage.ReleaseResources();
  }

  void BasicAccess()
  {
    TestImplicitStorage<T> portal;
    vtkm::cont::ArrayHandle<T, StorageTagType> implictHandle(portal);
    VTKM_TEST_ASSERT(implictHandle.GetNumberOfValues() == 1, "handle should have size 1");
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

int UnitTestStorageImplicit(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestStorageBasic);
}
