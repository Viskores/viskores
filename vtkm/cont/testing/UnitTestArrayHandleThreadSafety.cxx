//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/Token.h>

#include <vtkm/cont/serial/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/Testing.h>

#include <array>
#include <chrono>
#include <future>
#include <thread>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;
constexpr std::size_t NUM_THREADS = 20;

using ValueType = vtkm::FloatDefault;

template <typename Storage>
bool IncrementArray(vtkm::cont::ArrayHandle<ValueType, Storage> array)
{
  vtkm::cont::Token token;
  auto portal = array.PrepareForInPlace(vtkm::cont::DeviceAdapterTagSerial{}, token);
  if (portal.GetNumberOfValues() != ARRAY_SIZE)
  {
    std::cout << "!!!!! Wrong array size: " << portal.GetNumberOfValues() << std::endl;
    return false;
  }

  for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    ValueType value = portal.Get(index);
    ValueType base = TestValue(index, ValueType{});
    if ((value < base) || (value >= base + static_cast<ValueType>(NUM_THREADS)))
    {
      std::cout << "!!!!! Unexpected value in array: " << value << std::endl;
      return false;
    }
    portal.Set(index, value + 1);
  }

  return true;
}

template <typename Storage>
bool IncrementArrayOrdered(vtkm::cont::ArrayHandle<ValueType, Storage> array,
                           vtkm::cont::Token&& token_,
                           std::size_t threadNum)
{
  // Make sure the Token is moved to the proper scope.
  vtkm::cont::Token token = std::move(token_);

  // Sleep for a bit to make sure that threads at the end wait for threads before that
  // are sleeping.
  std::this_thread::sleep_for(
    std::chrono::milliseconds(10 * static_cast<long long>(NUM_THREADS - threadNum)));

  auto portal = array.PrepareForInPlace(vtkm::cont::DeviceAdapterTagSerial{}, token);
  if (portal.GetNumberOfValues() != ARRAY_SIZE)
  {
    std::cout << "!!!!! Wrong array size: " << portal.GetNumberOfValues() << std::endl;
    return false;
  }

  for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    ValueType value = portal.Get(index);
    ValueType base = TestValue(index, ValueType{});
    if (!test_equal(value, base + static_cast<ValueType>(threadNum)))
    {
      std::cout << "!!!!! Unexpected value in array: " << value << std::endl;
      std::cout << "!!!!! ArrayHandle access likely out of order." << std::endl;
      return false;
    }
    portal.Set(index, value + 1);
  }

  return true;
}

template <typename Storage>
bool CheckArray(vtkm::cont::ArrayHandle<ValueType, Storage> array)
{
  vtkm::cont::Token token;
  auto portal = array.PrepareForInput(vtkm::cont::DeviceAdapterTagSerial{}, token);
  if (portal.GetNumberOfValues() != ARRAY_SIZE)
  {
    std::cout << "!!!!! Wrong array size: " << portal.GetNumberOfValues() << std::endl;
    return false;
  }

  for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    ValueType value = portal.Get(index);
    ValueType expectedValue = TestValue(index, value) + static_cast<ValueType>(NUM_THREADS);
    if (!test_equal(value, expectedValue))
    {
      std::cout << "!!!!! Unexpected value in array: " << value << std::endl;
      return false;
    }
  }

  return true;
}

template <typename Storage>
bool DecrementArray(vtkm::cont::ArrayHandle<ValueType, Storage> array)
{
  vtkm::cont::Token token;
  auto portal = array.PrepareForInPlace(vtkm::cont::DeviceAdapterTagSerial{}, token);
  if (portal.GetNumberOfValues() != ARRAY_SIZE)
  {
    std::cout << "!!!!! Wrong array size: " << portal.GetNumberOfValues() << std::endl;
    return false;
  }

  for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    ValueType value = portal.Get(index);
    ValueType base = TestValue(index, value);
    if ((value <= base) || (value >= base + static_cast<ValueType>(NUM_THREADS) + 1))
    {
      std::cout << "!!!!! Unexpected value in array: " << value << std::endl;
      return false;
    }
    portal.Set(index, value - 1);
  }

  return true;
}

template <typename Storage>
void ThreadsIncrementToArray(vtkm::cont::ArrayHandle<ValueType, Storage> array)
{
  vtkm::cont::Token token;
  auto portal = array.PrepareForOutput(ARRAY_SIZE, vtkm::cont::DeviceAdapterTagSerial{}, token);

  std::cout << "  Starting write threads" << std::endl;
  std::array<decltype(std::async(std::launch::async, IncrementArray<Storage>, array)), NUM_THREADS>
    futures;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    futures[index] = std::async(std::launch::async, IncrementArray<Storage>, array);
  }

  std::cout << "  Filling array" << std::endl;
  for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
  {
    portal.Set(index, TestValue(index, ValueType{}));
  }

  std::cout << "  Releasing portal" << std::endl;
  token.DetachFromAll();

  std::cout << "  Wait for threads to complete" << std::endl;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    bool futureResult = futures[index].get();
    VTKM_TEST_ASSERT(futureResult, "Failure in IncrementArray");
  }
}

template <typename Storage>
void ThreadsCheckArray(vtkm::cont::ArrayHandle<ValueType, Storage> array)
{
  std::cout << "  Check array in control environment" << std::endl;
  auto portal = array.ReadPortal();
  VTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);

  std::cout << "  Starting threads to check" << std::endl;
  std::array<decltype(std::async(std::launch::async, CheckArray<Storage>, array)), NUM_THREADS>
    futures;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    futures[index] = std::async(std::launch::async, CheckArray<Storage>, array);
  }

  std::cout << "  Wait for threads to complete" << std::endl;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    bool futureResult = futures[index].get();
    VTKM_TEST_ASSERT(futureResult, "Failure in CheckArray");
  }
}

template <typename Storage>
void ThreadsDecrementArray(vtkm::cont::ArrayHandle<ValueType, Storage> array)
{
  std::cout << "  Starting threads to decrement" << std::endl;
  std::array<decltype(std::async(std::launch::async, DecrementArray<Storage>, array)), NUM_THREADS>
    futures;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    futures[index] = std::async(std::launch::async, DecrementArray<Storage>, array);
  }

  std::cout << "  Wait for threads to complete" << std::endl;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    bool futureResult = futures[index].get();
    VTKM_TEST_ASSERT(futureResult, "Failure in DecrementArray");
  }

  CheckPortal(array.ReadPortal());
}

template <typename Storage>
void ThreadsIncrementToArrayOrdered(vtkm::cont::ArrayHandle<ValueType, Storage> array)
{
  VTKM_TEST_ASSERT(array.GetNumberOfValues() == ARRAY_SIZE);
  SetPortal(array.WritePortal());

  std::cout << "  Starting ordered write threads" << std::endl;
  std::array<decltype(std::async(std::launch::async, IncrementArray<Storage>, array)), NUM_THREADS>
    futures;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    vtkm::cont::Token token;
    array.Enqueue(token);
    futures[index] = std::async(
      std::launch::async, IncrementArrayOrdered<Storage>, array, std::move(token), index);
  }

  std::cout << "  Wait for threads to complete" << std::endl;
  for (std::size_t index = 0; index < NUM_THREADS; ++index)
  {
    bool futureResult = futures[index].get();
    VTKM_TEST_ASSERT(futureResult, "Failure in IncrementArray");
  }
}

template <typename Storage>
void InvalidateControlPortal(vtkm::cont::ArrayHandle<ValueType, Storage> array)
{
  std::cout << "  Starting invalidate control portal" << std::endl;
  auto writePortal = array.WritePortal();

  {
    // PrepareForInPlace should invalidate the local control portal. It should work, but
    // further use of writePortal will be invalid.
    vtkm::cont::Token token;
    array.PrepareForInPlace(vtkm::cont::DeviceAdapterTagSerial{}, token);

#if 0
    std::cout << "    This should result in an error in the log and possibly a crash." << std::endl;
    // Unfortunately, there is no way to check this other than visual inspection or outright crash
    writePortal.Get(0);
#endif
  }

  auto readPortal = array.ReadPortal();

  {
    vtkm::cont::Token token;
    array.PrepareForInPlace(vtkm::cont::DeviceAdapterTagSerial{}, token);

#if 0
    std::cout << "    This should result in an error in the log and possibly a crash." << std::endl;
    // Unfortunately, there is no way to check this other than visual inspection or outright crash
    readPortal.Get(0);
#endif
  }
}

template <typename Storage>
void DoThreadSafetyTest(vtkm::cont::ArrayHandle<ValueType, Storage> array)
{
  ThreadsIncrementToArray(array);
  ThreadsCheckArray(array);
  ThreadsDecrementArray(array);
  ThreadsIncrementToArrayOrdered(array);
  InvalidateControlPortal(array);
}

void DoTest()
{
  std::cout << "Basic array handle." << std::endl;
  vtkm::cont::ArrayHandle<ValueType> basicArray;
  DoThreadSafetyTest(basicArray);

  std::cout << "Fancy array handle." << std::endl;
  vtkm::cont::ArrayHandle<ValueType> valueArray;
  valueArray.Allocate(ARRAY_SIZE);
  auto fancyArray =
    vtkm::cont::make_ArrayHandlePermutation(vtkm::cont::ArrayHandleIndex(ARRAY_SIZE), valueArray);
  DoThreadSafetyTest(fancyArray);
}

} // anonymous namespace

int UnitTestArrayHandleThreadSafety(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
