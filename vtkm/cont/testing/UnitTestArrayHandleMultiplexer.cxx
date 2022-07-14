//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingArrayHandleMultiplexer_h
#define vtk_m_cont_testing_TestingArrayHandleMultiplexer_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleMultiplexer.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vtkm/BinaryOperators.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template <typename T>
struct TestValueFunctor
{
  VTKM_EXEC_CONT T operator()(vtkm::Id index) const { return TestValue(index, T()); }
};

constexpr vtkm::Id ARRAY_SIZE = 10;

template <typename... Ts0, typename... Ts1>
static void CheckArray(const vtkm::cont::ArrayHandleMultiplexer<Ts0...>& multiplexerArray,
                       const vtkm::cont::ArrayHandle<Ts1...>& expectedArray)
{
  using T = typename std::remove_reference<decltype(multiplexerArray)>::type::ValueType;

  vtkm::cont::printSummary_ArrayHandle(multiplexerArray, std::cout);
  VTKM_TEST_ASSERT(test_equal_portals(multiplexerArray.ReadPortal(), expectedArray.ReadPortal()),
                   "Multiplexer array gave wrong result in control environment");

  vtkm::cont::ArrayHandle<T> copy;
  vtkm::cont::Algorithm::Copy(multiplexerArray, copy);
  VTKM_TEST_ASSERT(test_equal_portals(copy.ReadPortal(), expectedArray.ReadPortal()),
                   "Multiplexer did not copy correctly in execution environment");
}

void BasicSwitch()
{
  std::cout << "\n--- Basic switch" << std::endl;

  using ValueType = vtkm::FloatDefault;

  using ArrayType1 = vtkm::cont::ArrayHandleConstant<ValueType>;
  ArrayType1 array1(TestValue(0, vtkm::FloatDefault{}), ARRAY_SIZE);

  using ArrayType2 = vtkm::cont::ArrayHandleCounting<ValueType>;
  ArrayType2 array2(TestValue(1, vtkm::FloatDefault{}), 1.0f, ARRAY_SIZE);

  auto array3 = vtkm::cont::make_ArrayHandleImplicit(TestValueFunctor<ValueType>{}, ARRAY_SIZE);
  using ArrayType3 = decltype(array3);

  vtkm::cont::ArrayHandleMultiplexer<ArrayType1, ArrayType2, ArrayType3> multiplexer;

  std::cout << "Check array1" << std::endl;
  multiplexer = array1;
  CheckArray(multiplexer, array1);

  std::cout << "Check array2" << std::endl;
  multiplexer = array2;
  CheckArray(multiplexer, array2);

  std::cout << "Check array3" << std::endl;
  multiplexer = array3;
  CheckArray(multiplexer, array3);
}

void Reduce()
{
  // Regression test for an issue with compiling ArrayHandleMultiplexer with the thrust reduce
  // algorithm on CUDA. Most likely related to:
  // https://github.com/thrust/thrust/issues/928
  // https://github.com/thrust/thrust/issues/1044
  std::cout << "\n--- Reduce" << std::endl;

  using ValueType = vtkm::Vec3f;
  using MultiplexerType = vtkm::cont::ArrayHandleMultiplexer<
    vtkm::cont::ArrayHandleConstant<ValueType>,
    vtkm::cont::ArrayHandleCounting<ValueType>,
    vtkm::cont::ArrayHandle<ValueType>,
    vtkm::cont::ArrayHandleUniformPointCoordinates,
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>>;

  MultiplexerType multiplexer =
    vtkm::cont::ArrayHandleCounting<ValueType>(vtkm::Vec3f(1), vtkm::Vec3f(1), ARRAY_SIZE);

  {
    std::cout << "Basic Reduce" << std::endl;
    ValueType result = vtkm::cont::Algorithm::Reduce(multiplexer, ValueType(0.0));
    VTKM_TEST_ASSERT(test_equal(result, ValueType(0.5 * (ARRAY_SIZE * (ARRAY_SIZE + 1)))));
  }

  {
    std::cout << "Reduce with custom operator" << std::endl;
    vtkm::Vec<ValueType, 2> initial(ValueType(10000), ValueType(0));
    vtkm::Vec<ValueType, 2> result =
      vtkm::cont::Algorithm::Reduce(multiplexer, initial, vtkm::MinAndMax<ValueType>{});
    VTKM_TEST_ASSERT(test_equal(result[0], ValueType(1)));
    VTKM_TEST_ASSERT(test_equal(result[1], ValueType(static_cast<vtkm::FloatDefault>(ARRAY_SIZE))));
  }
}

void Fill()
{
  std::cout << "\n--- Fill" << std::endl;

  using ValueType = vtkm::Vec3f;
  using MultiplexerType = vtkm::cont::ArrayHandleMultiplexer<
    vtkm::cont::ArrayHandleConstant<ValueType>,
    vtkm::cont::ArrayHandleCounting<ValueType>,
    vtkm::cont::ArrayHandle<ValueType>,
    vtkm::cont::ArrayHandleUniformPointCoordinates,
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>>;

  const ValueType testValue1 = TestValue(1, ValueType{});
  const ValueType testValue2 = TestValue(2, ValueType{});

  MultiplexerType multiplexer = vtkm::cont::ArrayHandle<ValueType>{};

  multiplexer.AllocateAndFill(ARRAY_SIZE, testValue1);
  {
    auto portal = multiplexer.ReadPortal();
    VTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE);
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      VTKM_TEST_ASSERT(portal.Get(index) == testValue1);
    }
  }

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> array1;
  array1.Allocate(ARRAY_SIZE);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> array2;
  array2.Allocate(ARRAY_SIZE);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> array3;
  array3.Allocate(ARRAY_SIZE);
  multiplexer = vtkm::cont::make_ArrayHandleCartesianProduct(array1, array2, array3);

  multiplexer.Fill(testValue2);
  {
    auto portal1 = array1.ReadPortal();
    auto portal2 = array2.ReadPortal();
    auto portal3 = array3.ReadPortal();
    for (vtkm::Id index = 0; index < ARRAY_SIZE; ++index)
    {
      VTKM_TEST_ASSERT(portal1.Get(index) == testValue2[0]);
      VTKM_TEST_ASSERT(portal2.Get(index) == testValue2[1]);
      VTKM_TEST_ASSERT(portal3.Get(index) == testValue2[2]);
    }
  }
}

void TestAll()
{
  BasicSwitch();
  Reduce();
  Fill();
}

} // anonymous namespace

int UnitTestArrayHandleMultiplexer(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestAll, argc, argv);
}

#endif //vtk_m_cont_testing_TestingArrayHandleMultiplexer_h
