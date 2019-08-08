//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingImplicitFunction_h
#define vtk_m_cont_testing_TestingImplicitFunction_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterListTag.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/ImplicitFunctionHandle.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/internal/Configure.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <array>

namespace vtkm
{
namespace cont
{
namespace testing
{

namespace implicit_function_detail
{

class EvaluateImplicitFunction : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  EvaluateImplicitFunction(const vtkm::ImplicitFunction* function)
    : Function(function)
  {
  }

  template <typename VecType, typename ScalarType>
  VTKM_EXEC void operator()(const VecType& point, ScalarType& val, VecType& gradient) const
  {
    val = this->Function->Value(point);
    gradient = this->Function->Gradient(point);
  }

private:
  const vtkm::ImplicitFunction* Function;
};

template <typename DeviceAdapter>
void EvaluateOnCoordinates(vtkm::cont::CoordinateSystem points,
                           const vtkm::cont::ImplicitFunctionHandle& function,
                           vtkm::cont::ArrayHandle<vtkm::FloatDefault>& values,
                           vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& gradients,
                           DeviceAdapter device)
{
  using EvalDispatcher = vtkm::worklet::DispatcherMapField<EvaluateImplicitFunction>;

  EvaluateImplicitFunction eval(function.PrepareForExecution(device));
  EvalDispatcher dispatcher(eval);
  dispatcher.SetDevice(DeviceAdapter());
  dispatcher.Invoke(points, values, gradients);
}

template <typename ItemType, std::size_t N>
bool TestArrayEqual(const vtkm::cont::ArrayHandle<ItemType>& result,
                    const std::array<ItemType, N>& expected)
{
  bool success = false;
  auto portal = result.GetPortalConstControl();
  vtkm::Id count = portal.GetNumberOfValues();

  if (static_cast<std::size_t>(count) == N)
  {
    success = true;
    for (vtkm::Id i = 0; i < count; ++i)
    {
      if (!test_equal(portal.Get(i), expected[static_cast<std::size_t>(i)]))
      {
        success = false;
        break;
      }
    }
  }
  if (!success)
  {
    if (count == 0)
    {
      std::cout << "result: <empty>\n";
    }
    else
    {
      std::cout << "result: " << portal.Get(0);
      for (vtkm::Id i = 1; i < count; ++i)
      {
        std::cout << ", " << portal.Get(i);
      }
      std::cout << "\n";
      std::cout << "expected: " << expected[0];
      for (vtkm::Id i = 1; i < count; ++i)
      {
        std::cout << ", " << expected[static_cast<std::size_t>(i)];
      }
      std::cout << "\n";
    }
  }

  return success;
}

} // anonymous namespace

class TestingImplicitFunction
{
public:
  TestingImplicitFunction()
    : Input(vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet2())
  {
  }

  template <typename DeviceAdapter>
  void Run(DeviceAdapter device)
  {
    this->TestBox(device);
    this->TestCylinder(device);
    this->TestFrustum(device);
    this->TestPlane(device);
    this->TestSphere(device);
  }

private:
  template <typename DeviceAdapter>
  void TestBox(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Box on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0),
      vtkm::cont::make_ImplicitFunctionHandle(
        vtkm::Box({ 0.0f, -0.5f, -0.5f }, { 1.5f, 1.5f, 0.5f })),
      values,
      gradients,
      device);

    std::array<vtkm::FloatDefault, 8> expected = {
      { 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.5f }
    };
    std::array<vtkm::Vec<vtkm::FloatDefault, 3>, 8> expectedGradients = {
      { { -1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f },
        { -1.0f, 0.0f, 0.0f },
        { 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f },
        { 0.0f, 0.0f, 1.0f } }
    };
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expected),
                     "Result does not match expected values");
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(gradients, expectedGradients),
                     "Result does not match expected gradients values");
  }

  template <typename DeviceAdapter>
  void TestCylinder(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Cylinder on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    vtkm::Cylinder cylinder;
    cylinder.SetCenter({ 0.0f, 0.0f, 1.0f });
    cylinder.SetAxis({ 0.0f, 1.0f, 0.0f });
    cylinder.SetRadius(1.0f);

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0),
      vtkm::cont::ImplicitFunctionHandle(&cylinder, false),
      values,
      gradients,
      device);

    std::array<vtkm::FloatDefault, 8> expected = {
      { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f }
    };
    std::array<vtkm::Vec<vtkm::FloatDefault, 3>, 8> expectedGradients = {
      { { 0.0f, 0.0f, -2.0f },
        { 2.0f, 0.0f, -2.0f },
        { 2.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, -2.0f },
        { 2.0f, 0.0f, -2.0f },
        { 2.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f } }
    };
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expected),
                     "Result does not match expected values");
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(gradients, expectedGradients),
                     "Result does not match expected gradients values");
  }

  template <typename DeviceAdapter>
  void TestFrustum(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Frustum on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    vtkm::Vec3f points[8] = {
      { 0.0f, 0.0f, 0.0f }, // 0
      { 1.0f, 0.0f, 0.0f }, // 1
      { 1.0f, 0.0f, 1.0f }, // 2
      { 0.0f, 0.0f, 1.0f }, // 3
      { 0.5f, 1.5f, 0.5f }, // 4
      { 1.5f, 1.5f, 0.5f }, // 5
      { 1.5f, 1.5f, 1.5f }, // 6
      { 0.5f, 1.5f, 1.5f }  // 7
    };
    vtkm::Frustum frustum;
    frustum.CreateFromPoints(points);

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0),
      vtkm::cont::make_ImplicitFunctionHandle(frustum),
      values,
      gradients,
      device);

    std::array<vtkm::FloatDefault, 8> expected = {
      { 0.0f, 0.0f, 0.0f, 0.0f, 0.316228f, 0.316228f, -0.316228f, 0.316228f }
    };
    std::array<vtkm::Vec<vtkm::FloatDefault, 3>, 8> expectedGradients = {
      { { 0.0f, -1.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f },
        { 0.0f, 0.316228f, -0.948683f },
        { 0.0f, 0.316228f, -0.948683f },
        { 0.948683f, -0.316228f, 0.0f },
        { -0.948683f, 0.316228f, 0.0f } }
    };
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expected),
                     "Result does not match expected values");
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(gradients, expectedGradients),
                     "Result does not match expected gradients values");
  }

  template <typename DeviceAdapter>
  void TestPlane(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Plane on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    auto planeHandle = vtkm::cont::make_ImplicitFunctionHandle<vtkm::Plane>(
      vtkm::make_Vec(0.5f, 0.5f, 0.5f), vtkm::make_Vec(1.0f, 0.0f, 1.0f));
    auto plane = static_cast<vtkm::Plane*>(planeHandle.Get());

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0), planeHandle, values, gradients, device);
    std::array<vtkm::FloatDefault, 8> expected = {
      { -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f }
    };
    std::array<vtkm::Vec<vtkm::FloatDefault, 3>, 8> expectedGradients = {
      { { 1.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f },
        { 1.0f, 0.0f, 1.0f } }
    };
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expected),
                     "Result does not match expected values");
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(gradients, expectedGradients),
                     "Result does not match expected gradients values");

    plane->SetNormal({ -1.0f, 0.0f, -1.0f });
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0), planeHandle, values, gradients, device);
    expected = { { 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f } };
    expectedGradients = { { { -1.0f, 0.0f, -1.0f },
                            { -1.0f, 0.0f, -1.0f },
                            { -1.0f, 0.0f, -1.0f },
                            { -1.0f, 0.0f, -1.0f },
                            { -1.0f, 0.0f, -1.0f },
                            { -1.0f, 0.0f, -1.0f },
                            { -1.0f, 0.0f, -1.0f },
                            { -1.0f, 0.0f, -1.0f } } };
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expected),
                     "Result does not match expected values");
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(gradients, expectedGradients),
                     "Result does not match expected gradients values");
  }

  template <typename DeviceAdapter>
  void TestSphere(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Sphere on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0),
      vtkm::cont::make_ImplicitFunctionHandle<vtkm::Sphere>(vtkm::make_Vec(0.0f, 0.0f, 0.0f), 1.0f),
      values,
      gradients,
      device);

    std::array<vtkm::FloatDefault, 8> expected = {
      { -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f }
    };
    std::array<vtkm::Vec<vtkm::FloatDefault, 3>, 8> expectedGradients = {
      { { 0.0f, 0.0f, 0.0f },
        { 2.0f, 0.0f, 0.0f },
        { 2.0f, 0.0f, 2.0f },
        { 0.0f, 0.0f, 2.0f },
        { 0.0f, 2.0f, 0.0f },
        { 2.0f, 2.0f, 0.0f },
        { 2.0f, 2.0f, 2.0f },
        { 0.0f, 2.0f, 2.0f } }
    };
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expected),
                     "Result does not match expected values");
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(gradients, expectedGradients),
                     "Result does not match expected gradients values");
  }

  vtkm::cont::DataSet Input;
};
}
}
} // vtmk::cont::testing

#endif //vtk_m_cont_testing_TestingImplicitFunction_h
