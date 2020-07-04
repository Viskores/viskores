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

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleImplicit.h>
#include <vtkm/cont/ArrayHandleMultiplexer.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/BinaryOperators.h>

#include <vtkm/cont/testing/Testing.h>

namespace vtkm
{
namespace cont
{
namespace testing
{

template <typename T>
struct TestValueFunctor
{
  VTKM_EXEC_CONT T operator()(vtkm::Id index) const { return TestValue(index, T()); }
};

template <typename DeviceAdapter>
class TestingArrayHandleMultiplexer
{
  static constexpr vtkm::Id ARRAY_SIZE = 10;

  template <typename... Ts0, typename... Ts1>
  static void CheckArray(const vtkm::cont::ArrayHandleMultiplexer<Ts0...>& multiplexerArray,
                         const vtkm::cont::ArrayHandle<Ts1...>& expectedArray)
  {
    using T = typename std::remove_reference<decltype(multiplexerArray)>::type::ValueType;

    vtkm::cont::printSummary_ArrayHandle(multiplexerArray, std::cout);
    VTKM_TEST_ASSERT(test_equal_portals(multiplexerArray.ReadPortal(), expectedArray.ReadPortal()),
                     "Multiplexer array gave wrong result in control environment");

    vtkm::cont::ArrayHandle<T> copy;
    vtkm::cont::ArrayCopy(multiplexerArray, copy);
    VTKM_TEST_ASSERT(test_equal_portals(copy.ReadPortal(), expectedArray.ReadPortal()),
                     "Multiplexer did not copy correctly in execution environment");
  }

  static void BasicSwitch()
  {
    std::cout << std::endl << "--- Basic switch" << std::endl;

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

  static void Reduce()
  {
    // Regression test for an issue with compiling ArrayHandleMultiplexer with the thrust reduce
    // algorithm on CUDA. Most likely related to:
    // https://github.com/thrust/thrust/issues/928
    // https://github.com/thrust/thrust/issues/1044
    std::cout << std::endl << "--- Reduce" << std::endl;

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

    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    {
      std::cout << "Basic Reduce" << std::endl;
      ValueType result = Algorithm::Reduce(multiplexer, ValueType(0.0));
      VTKM_TEST_ASSERT(test_equal(result, ValueType(0.5 * (ARRAY_SIZE * (ARRAY_SIZE + 1)))));
    }

    {
      std::cout << "Reduce with custom operator" << std::endl;
      vtkm::Vec<ValueType, 2> initial(ValueType(10000), ValueType(0));
      vtkm::Vec<ValueType, 2> result =
        Algorithm::Reduce(multiplexer, initial, vtkm::MinAndMax<ValueType>{});
      VTKM_TEST_ASSERT(test_equal(result[0], ValueType(1)));
      VTKM_TEST_ASSERT(
        test_equal(result[1], ValueType(static_cast<vtkm::FloatDefault>(ARRAY_SIZE))));
    }
  }

  static void TestAll()
  {
    BasicSwitch();
    Reduce();
  }

public:
  static int Run(int argc, char* argv[])
  {
    vtkm::cont::ScopedRuntimeDeviceTracker device(DeviceAdapter{});
    return vtkm::cont::testing::Testing::Run(TestAll, argc, argv);
  }
};
}
}
} // namespace vtkm::cont::testing

#endif //vtk_m_cont_testing_TestingArrayHandleMultiplexer_h
