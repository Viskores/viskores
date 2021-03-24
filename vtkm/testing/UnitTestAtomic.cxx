//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Atomic.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleBasic.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 100;

template <typename T>
struct AtomicTests
{
  vtkm::cont::Invoker Invoke;

  static constexpr vtkm::Id OVERLAP = sizeof(T) * CHAR_BIT;
  static constexpr vtkm::Id EXTENDED_SIZE = ARRAY_SIZE * OVERLAP;

  VTKM_EXEC_CONT static T TestValue(vtkm::Id index) { return ::TestValue(index, T{}); }

  struct ArrayToRawPointer : vtkm::cont::ExecutionObjectBase
  {
    vtkm::cont::ArrayHandleBasic<T> Array;
    VTKM_CONT ArrayToRawPointer(const vtkm::cont::ArrayHandleBasic<T>& array)
      : Array(array)
    {
    }

    VTKM_CONT T* PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                     vtkm::cont::Token& token) const
    {
      return reinterpret_cast<T*>(this->Array.GetBuffers()[0].WritePointerDevice(device, token));
    }
  };

  struct LoadFunctor : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VTKM_EXEC void operator()(vtkm::Id index, T* data) const
    {
      if (!test_equal(vtkm::AtomicLoad(data + index), TestValue(index)))
      {
        this->RaiseError("Bad AtomicLoad");
      }
    }
  };

  VTKM_CONT void TestLoad()
  {
    std::cout << "AtomicLoad" << std::endl;
    vtkm::cont::ArrayHandleBasic<T> array;
    array.Allocate(ARRAY_SIZE);
    SetPortal(array.WritePortal());

    this->Invoke(LoadFunctor{}, array, ArrayToRawPointer(array));
  }

  struct StoreFunctor : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VTKM_EXEC void operator()(vtkm::Id index, T* data) const
    {
      vtkm::AtomicStore(data + (index % ARRAY_SIZE), TestValue(index));
    }
  };

  VTKM_CONT void TestStore()
  {
    std::cout << "AtomicStore" << std::endl;
    vtkm::cont::ArrayHandleBasic<T> array;
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      StoreFunctor{}, vtkm::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    for (vtkm::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      bool foundExpected = false;
      T foundValue = portal.Get(arrayIndex);
      for (vtkm::Id overlapIndex = 0; overlapIndex < OVERLAP; ++overlapIndex)
      {
        if (test_equal(foundValue, TestValue(arrayIndex + (overlapIndex * ARRAY_SIZE))))
        {
          foundExpected = true;
          break;
        }
      }
      VTKM_TEST_ASSERT(
        foundExpected, "Wrong value (", foundValue, ") stored in index ", arrayIndex);
    }
  }

  struct AddFunctor : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VTKM_EXEC void operator()(vtkm::Id index, T* data) const
    {
      vtkm::AtomicAdd(data + (index % ARRAY_SIZE), 2);
      vtkm::AtomicAdd(data + (index % ARRAY_SIZE), -1);
    }
  };

  VTKM_CONT void TestAdd()
  {
    std::cout << "AtomicAdd" << std::endl;
    vtkm::cont::ArrayHandleBasic<T> array;
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleConstant<T>(0, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AddFunctor{}, vtkm::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(OVERLAP);
    for (vtkm::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VTKM_TEST_ASSERT(test_equal(foundValue, expectedValue), foundValue, " != ", expectedValue);
    }
  }

  struct AndFunctor : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VTKM_EXEC void operator()(vtkm::Id index, T* data) const
    {
      vtkm::Id arrayIndex = index % ARRAY_SIZE;
      vtkm::Id offsetIndex = index / ARRAY_SIZE;
      vtkm::AtomicAnd(data + arrayIndex, ~(T(0x1u) << offsetIndex));
    }
  };

  VTKM_CONT void TestAnd()
  {
    std::cout << "AtomicAnd" << std::endl;
    vtkm::cont::ArrayHandleBasic<T> array;
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleConstant<T>(T(-1), ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AndFunctor{}, vtkm::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    for (vtkm::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VTKM_TEST_ASSERT(test_equal(foundValue, 0), foundValue, " != 0");
    }
  }

  struct OrFunctor : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VTKM_EXEC void operator()(vtkm::Id index, T* data) const
    {
      vtkm::Id arrayIndex = index % ARRAY_SIZE;
      vtkm::Id offsetIndex = index / ARRAY_SIZE;
      vtkm::AtomicOr(data + arrayIndex, 0x1u << offsetIndex);
    }
  };

  VTKM_CONT void TestOr()
  {
    std::cout << "AtomicOr" << std::endl;
    vtkm::cont::ArrayHandleBasic<T> array;
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleConstant<T>(0, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AndFunctor{}, vtkm::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(-1);
    for (vtkm::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VTKM_TEST_ASSERT(test_equal(foundValue, 0), foundValue, " != ", expectedValue);
    }
  }

  struct XorFunctor : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VTKM_EXEC void operator()(vtkm::Id index, T* data) const
    {
      vtkm::Id arrayIndex = index % ARRAY_SIZE;
      vtkm::Id offsetIndex = index / ARRAY_SIZE;
      vtkm::AtomicXor(data + arrayIndex, 0x3u << offsetIndex);
    }
  };

  VTKM_CONT void TestXor()
  {
    std::cout << "AtomicXor" << std::endl;
    vtkm::cont::ArrayHandleBasic<T> array;
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleConstant<T>(0, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AndFunctor{}, vtkm::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(1);
    for (vtkm::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VTKM_TEST_ASSERT(test_equal(foundValue, 0), foundValue, " != ", expectedValue);
    }
  }

  struct NotFunctor : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VTKM_EXEC void operator()(vtkm::Id index, T* data) const
    {
      vtkm::Id arrayIndex = index % ARRAY_SIZE;
      vtkm::Id offsetIndex = index / ARRAY_SIZE;
      if (offsetIndex < arrayIndex)
      {
        vtkm::AtomicNot(data + arrayIndex);
      }
    }
  };

  VTKM_CONT void TestNot()
  {
    std::cout << "AtomicNot" << std::endl;
    vtkm::cont::ArrayHandleBasic<T> array;
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleConstant<T>(0xA, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AndFunctor{}, vtkm::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(0xA);
    for (vtkm::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VTKM_TEST_ASSERT(test_equal(foundValue, 0), foundValue, " != ", expectedValue);
      expectedValue = static_cast<T>(~expectedValue);
    }
  }

  struct CompareExchangeFunctor : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn ignored, ExecObject);
    using ExecutionSignature = void(WorkIndex, _2);

    VTKM_EXEC void operator()(vtkm::Id index, T* data) const
    {
      vtkm::Id arrayIndex = index % ARRAY_SIZE;
      bool success = false;
      for (T overlapIndex = 0; overlapIndex < static_cast<T>(OVERLAP); ++overlapIndex)
      {
        T expectedValue = overlapIndex;
        if (vtkm::AtomicCompareExchange(data + arrayIndex, &expectedValue, overlapIndex + 1))
        {
          success = true;
          break;
        }
      }

      if (!success)
      {
        this->RaiseError("No compare succeeded");
      }
    }
  };

  VTKM_CONT void TestCompareExchange()
  {
    std::cout << "AtomicCompareExchange" << std::endl;
    vtkm::cont::ArrayHandleBasic<T> array;
    vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandleConstant<T>(0, ARRAY_SIZE), array);
    array.Allocate(ARRAY_SIZE);

    this->Invoke(
      AddFunctor{}, vtkm::cont::ArrayHandleIndex(EXTENDED_SIZE), ArrayToRawPointer(array));

    auto portal = array.ReadPortal();
    T expectedValue = T(OVERLAP);
    for (vtkm::Id arrayIndex = 0; arrayIndex < ARRAY_SIZE; ++arrayIndex)
    {
      T foundValue = portal.Get(arrayIndex);
      VTKM_TEST_ASSERT(test_equal(foundValue, expectedValue), foundValue, " != ", expectedValue);
    }
  }

  VTKM_CONT void TestAll()
  {
    TestLoad();
    TestStore();
    TestAdd();
    TestAnd();
    TestOr();
    TestXor();
    TestNot();
    TestCompareExchange();
  }
};

struct TestFunctor
{
  template <typename T>
  VTKM_CONT void operator()(T) const
  {
    AtomicTests<T>().TestAll();
  }
};

void Run()
{
  VTKM_TEST_ASSERT(vtkm::ListHas<vtkm::AtomicTypesSupported, vtkm::AtomicTypePreferred>::value);

  vtkm::testing::Testing::TryTypes(TestFunctor{}, vtkm::AtomicTypesSupported{});
}

} // anonymous namespace

int UnitTestAtomic(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
