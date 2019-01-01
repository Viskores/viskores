//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleVirtual.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/cont/serial/internal/ArrayManagerExecutionSerial.h>
#include <vtkm/cont/serial/internal/DeviceAdapterAlgorithmSerial.h>
#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/BinaryOperators.h>

#include <algorithm>

namespace UnitTestArrayHandleVirtualDetail
{

template <typename ValueType>
struct Test
{
  static constexpr vtkm::Id ARRAY_SIZE = 100;
  static constexpr vtkm::Id NUM_KEYS = 3;

  using ArrayHandle = vtkm::cont::ArrayHandle<ValueType>;
  using VirtHandle = vtkm::cont::ArrayHandleVirtual<ValueType>;
  using DeviceTag = vtkm::cont::DeviceAdapterTagSerial;
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>;

  void TestConstructors()
  {
    VirtHandle nullStorage;
    VTKM_TEST_ASSERT(nullStorage.GetStorage() == nullptr,
                     "storage should be empty when using ArrayHandleVirtual().");

    VirtHandle fromArrayHandle{ ArrayHandle{} };
    VTKM_TEST_ASSERT(fromArrayHandle.GetStorage() != nullptr,
                     "storage should be empty when using ArrayHandleVirtual().");
    VTKM_TEST_ASSERT(vtkm::cont::IsType<ArrayHandle>(fromArrayHandle),
                     "ArrayHandleVirtual should contain a ArrayHandle<ValueType>.");

    VirtHandle fromVirtHandle(fromArrayHandle);
    VTKM_TEST_ASSERT(fromVirtHandle.GetStorage() != nullptr,
                     "storage should be empty when using ArrayHandleVirtual().");
    VTKM_TEST_ASSERT(vtkm::cont::IsType<ArrayHandle>(fromVirtHandle),
                     "ArrayHandleVirtual should contain a ArrayHandle<ValueType>.");

    VirtHandle fromNullPtrHandle(nullStorage);
    VTKM_TEST_ASSERT(fromNullPtrHandle.GetStorage() == nullptr,
                     "storage should be empty when constructing from a ArrayHandleVirtual that has "
                     "nullptr storage.");
    VTKM_TEST_ASSERT((vtkm::cont::IsType<ArrayHandle>(fromNullPtrHandle) == false),
                     "ArrayHandleVirtual shouldn't match any type with nullptr storage.");
  }


  void TestMoveConstructors()
  {
    //test shared_ptr move constructor
    {
      vtkm::cont::ArrayHandleCounting<ValueType> countingHandle;
      using ST = typename decltype(countingHandle)::StorageTag;
      auto sharedPtr = std::make_shared<vtkm::cont::StorageAny<ValueType, ST>>(countingHandle);

      VirtHandle virt(std::move(sharedPtr));
      VTKM_TEST_ASSERT(
        vtkm::cont::IsType<decltype(countingHandle)>(virt),
        "ArrayHandleVirtual should be valid after move constructor shared_ptr<Storage>.");
    }

    //test unique_ptr move constructor
    {
      vtkm::cont::ArrayHandleCounting<ValueType> countingHandle;
      using ST = typename decltype(countingHandle)::StorageTag;
      auto uniquePtr = std::unique_ptr<vtkm::cont::StorageAny<ValueType, ST>>(
        new vtkm::cont::StorageAny<ValueType, ST>(countingHandle));

      VirtHandle virt(std::move(uniquePtr));
      VTKM_TEST_ASSERT(
        vtkm::cont::IsType<decltype(countingHandle)>(virt),
        "ArrayHandleVirtual should be valid after move constructor unique_ptr<Storage>.");
    }

    //test ArrayHandle move constructor
    {
      ArrayHandle handle;
      VirtHandle virt(std::move(handle));
      VTKM_TEST_ASSERT(
        vtkm::cont::IsType<ArrayHandle>(virt),
        "ArrayHandleVirtual should be valid after move constructor ArrayHandle<ValueType>.");
    }

    //test ArrayHandleVirtual move constructor
    {
      ArrayHandle handle;
      VirtHandle virt(std::move(handle));
      VirtHandle virt2(std::move(virt));
      VTKM_TEST_ASSERT(
        vtkm::cont::IsType<ArrayHandle>(virt2),
        "ArrayHandleVirtual should be valid after move constructor ArrayHandleVirtual<ValueType>.");
    }
  }

  void TestAssignmentOps()
  {
    //test assignment operator from ArrayHandleVirtual
    {
      VirtHandle virt;
      virt = VirtHandle{ ArrayHandle{} };
      VTKM_TEST_ASSERT(vtkm::cont::IsType<ArrayHandle>(virt),
                       "ArrayHandleVirtual should be valid after assignment op from AHV.");
    }

    //test assignment operator from ArrayHandle
    {
      VirtHandle virt = vtkm::cont::ArrayHandleCounting<ValueType>{};
      virt = ArrayHandle{};
      VTKM_TEST_ASSERT(vtkm::cont::IsType<ArrayHandle>(virt),
                       "ArrayHandleVirtual should be valid after assignment op from AH.");
    }

    //test move assignment operator from ArrayHandleVirtual
    {
      VirtHandle temp{ ArrayHandle{} };
      VirtHandle virt;
      virt = std::move(temp);
      VTKM_TEST_ASSERT(vtkm::cont::IsType<ArrayHandle>(virt),
                       "ArrayHandleVirtual should be valid after move assignment op from AHV.");
    }

    //test move assignment operator from ArrayHandle
    {
      vtkm::cont::ArrayHandleCounting<ValueType> temp;
      VirtHandle virt;
      virt = std::move(temp);
      VTKM_TEST_ASSERT(vtkm::cont::IsType<decltype(temp)>(virt),
                       "ArrayHandleVirtual should be valid after move assignment op from AH.");
    }
  }

  void TestPrepareForExecution()
  {
    vtkm::cont::ArrayHandle<ValueType> handle;
    handle.Allocate(50);

    VirtHandle virt(std::move(handle));

    try
    {
      virt.PrepareForInput(DeviceTag());
      virt.PrepareForInPlace(DeviceTag());
      virt.PrepareForOutput(ARRAY_SIZE, DeviceTag());
    }
    catch (vtkm::cont::ErrorBadValue&)
    {
      // un-expected failure.
      VTKM_TEST_FAIL(
        "Unexpected error when using Prepare* on an ArrayHandleVirtual with StorageAny.");
    }
  }


  void TestIsType()
  {
    vtkm::cont::ArrayHandle<ValueType> handle;
    VirtHandle virt(std::move(handle));

    VTKM_TEST_ASSERT(vtkm::cont::IsType<decltype(virt)>(virt),
                     "virt should by same type as decltype(virt)");
    VTKM_TEST_ASSERT(vtkm::cont::IsType<decltype(handle)>(virt),
                     "virt should by same type as decltype(handle)");

    vtkm::cont::ArrayHandle<vtkm::Vec<ValueType, 3>> vecHandle;
    VTKM_TEST_ASSERT(!vtkm::cont::IsType<decltype(vecHandle)>(virt),
                     "virt shouldn't by same type as decltype(vecHandle)");
  }

  void TestCast()
  {

    vtkm::cont::ArrayHandle<ValueType> handle;
    VirtHandle virt(handle);

    auto c1 = vtkm::cont::Cast<decltype(virt)>(virt);
    VTKM_TEST_ASSERT(c1 == virt, "virt should cast to VirtHandle");

    auto c2 = vtkm::cont::Cast<decltype(handle)>(virt);
    VTKM_TEST_ASSERT(c2 == handle, "virt should cast to HandleType");

    using VecHandle = vtkm::cont::ArrayHandle<vtkm::Vec<ValueType, 3>>;
    try
    {
      auto c3 = vtkm::cont::Cast<VecHandle>(virt);
      VTKM_TEST_FAIL("Cast of T to Vec<T,3> should have failed");
    }
    catch (vtkm::cont::ErrorBadType&)
    {
    }
  }

  void operator()()
  {
    TestConstructors();
    TestMoveConstructors();
    TestAssignmentOps();
    TestPrepareForExecution();
    TestIsType();
    TestCast();
  }
};

void TestArrayHandleVirtual()
{
  Test<vtkm::UInt8>()();
  Test<vtkm::Int16>()();
  Test<vtkm::Int32>()();
  Test<vtkm::Int64>()();
  Test<vtkm::Float32>()();
  Test<vtkm::Float64>()();
}

} // end namespace UnitTestArrayHandleVirtualDetail

int UnitTestArrayHandleVirtual(int, char* [])
{
  using namespace UnitTestArrayHandleVirtualDetail;
  return vtkm::cont::testing::Testing::Run(TestArrayHandleVirtual);
}
