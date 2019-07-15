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
#include <vtkm/cont/RuntimeDeviceTracker.h>

#include <vtkm/cont/testing/Testing.h>

namespace vtkm
{
namespace cont
{
namespace testing
{

template <typename DeviceAdapter>
class TestingArrayHandleMultiplexer
{
  static constexpr vtkm::Id ARRAY_SIZE = 10;

  template <typename T>
  struct TestValueFunctor
  {
    T operator()(vtkm::Id index) const { return TestValue(index, T()); }
  };

  template <typename... Ts0, typename... Ts1>
  static void CheckArray(const vtkm::cont::ArrayHandleMultiplexer<Ts0...>& multiplexerArray,
                         const vtkm::cont::ArrayHandle<Ts1...>& expectedArray)
  {
    using T = typename std::remove_reference<decltype(multiplexerArray)>::type::ValueType;

    vtkm::cont::printSummary_ArrayHandle(multiplexerArray, std::cout);
    VTKM_TEST_ASSERT(test_equal_portals(multiplexerArray.GetPortalConstControl(),
                                        expectedArray.GetPortalConstControl()),
                     "Multiplexer array gave wrong result in control environment");

    vtkm::cont::ArrayHandle<T> copy;
    vtkm::cont::ArrayCopy(multiplexerArray, copy);
    VTKM_TEST_ASSERT(
      test_equal_portals(copy.GetPortalConstControl(), expectedArray.GetPortalConstControl()),
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

    multiplexer = array1;
    CheckArray(multiplexer, array1);

    multiplexer = array2;
    CheckArray(multiplexer, array2);

    multiplexer = array3;
    CheckArray(multiplexer, array3);
  }

  static void DefaultScalar()
  {
    std::cout << std::endl << "--- Default list for scalars" << std::endl;

    using ValueType = vtkm::FloatDefault;

    vtkm::cont::ArrayHandleMultiplexer<ValueType> multiplexer;

    std::cout << "Basic array type." << std::endl;
    vtkm::cont::ArrayHandle<ValueType> baseArray;
    baseArray.Allocate(ARRAY_SIZE);
    SetPortal(baseArray.GetPortalControl());
    multiplexer = baseArray;
    CheckArray(multiplexer, baseArray);

    std::cout << "Cast array type." << std::endl;
    vtkm::cont::ArrayHandle<vtkm::UInt8> castArray;
    castArray.Allocate(ARRAY_SIZE);
    SetPortal(castArray.GetPortalControl());
    multiplexer = vtkm::cont::make_ArrayHandleCast<ValueType>(castArray);
    CheckArray(multiplexer, castArray);
  }

  static void DefaultVec3()
  {
    std::cout << std::endl << "--- Default list for Vec3" << std::endl;

    using ValueType = vtkm::Vec<vtkm::FloatDefault, 3>;

    vtkm::cont::ArrayHandleMultiplexer<ValueType> multiplexer;

    std::cout << "Basic array type." << std::endl;
    vtkm::cont::ArrayHandle<ValueType> baseArray;
    baseArray.Allocate(ARRAY_SIZE);
    SetPortal(baseArray.GetPortalControl());
    multiplexer = baseArray;
    CheckArray(multiplexer, baseArray);

    std::cout << "Cast array type." << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>> castArray;
    castArray.Allocate(ARRAY_SIZE);
    SetPortal(castArray.GetPortalControl());
    multiplexer = vtkm::cont::make_ArrayHandleCast<ValueType>(castArray);
    CheckArray(multiplexer, castArray);

    std::cout << "Uniform point coordinates" << std::endl;
    vtkm::cont::ArrayHandleUniformPointCoordinates uniformCoords(vtkm::Id3(3));
    multiplexer = uniformCoords;
    CheckArray(multiplexer, uniformCoords);
  }

  static void TestAll()
  {
    BasicSwitch();
    DefaultScalar();
    DefaultVec3();
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
