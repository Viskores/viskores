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
#include <vtkm/cont/DeviceAdapterList.h>
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
      std::cout << "result:   " << portal.Get(0);
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
  void Try(vtkm::cont::ImplicitFunctionHandle& function,
           const std::array<vtkm::FloatDefault, 8>& expectedValues,
           const std::array<vtkm::Vec3f, 8>& expectedGradients,
           DeviceAdapter device)
  {
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0), function, values, gradients, device);

    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expectedValues),
                     "Result does not match expected values");
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(gradients, expectedGradients),
                     "Result does not match expected gradients values");
  }

  template <typename DeviceAdapter>
  void TestBox(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Box on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  default box" << std::endl;
    vtkm::Box box;
    vtkm::cont::ImplicitFunctionHandle boxHandle(&box, false);
    this->Try(boxHandle,
              { { -0.5f, 0.5f, 0.707107f, 0.5f, 0.5f, 0.707107f, 0.866025f, 0.707107f } },
              { { vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 0.707107f, 0.0f, 0.707107f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 1.0f, 0.0f },
                  vtkm::Vec3f{ 0.707107f, 0.707107f, 0.0f },
                  vtkm::Vec3f{ 0.57735f, 0.57735f, 0.57735f },
                  vtkm::Vec3f{ 0.0f, 0.707107f, 0.707107f } } },
              device);

    std::cout << "  Specified min/max box" << std::endl;
    box.SetMinPoint({ 0.0f, -0.5f, -0.5f });
    box.SetMaxPoint({ 1.5f, 1.5f, 0.5f });
    this->Try(boxHandle,
              { { 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.5f } },
              { { vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } },
              device);

    std::cout << "  Specified bounds box" << std::endl;
    box.SetBounds({ vtkm::Range(0.0, 1.5), vtkm::Range(-0.5, 1.5), vtkm::Range(-0.5, 0.5) });
    this->Try(boxHandle,
              { { 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.5f } },
              { { vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } },
              device);
  }

  template <typename DeviceAdapter>
  void TestCylinder(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Cylinder on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  Default cylinder" << std::endl;
    vtkm::Cylinder cylinder;
    vtkm::cont::ImplicitFunctionHandle cylinderHandle(&cylinder, false);
    this->Try(cylinderHandle,
              { { -0.25f, 0.75f, 1.75f, 0.75f, -0.25f, 0.75f, 1.75f, 0.75f } },
              { { vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 2.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 2.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f } } },
              device);

    std::cout << "  Translated, scaled cylinder" << std::endl;
    cylinder.SetCenter({ 0.0f, 0.0f, 1.0f });
    cylinder.SetAxis({ 0.0f, 1.0f, 0.0f });
    cylinder.SetRadius(1.0f);
    this->Try(cylinderHandle,
              { { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f } },
              { { vtkm::Vec3f{ 0.0f, 0.0f, -2.0f },
                  vtkm::Vec3f{ 2.0f, 0.0f, -2.0f },
                  vtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, -2.0f },
                  vtkm::Vec3f{ 2.0f, 0.0f, -2.0f },
                  vtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 0.0f } } },
              device);

    std::cout << "  Non-unit axis" << std::endl;
    cylinder.SetCenter({ 0.0f, 0.0f, 0.0f });
    cylinder.SetAxis({ 1.0f, 1.0f, 0.0f });
    cylinder.SetRadius(1.0f);
    this->Try(cylinderHandle,
              { { -1.0f, -0.5f, 0.5f, 0.0f, -0.5f, -1.0f, 0.0f, 0.5f } },
              { { vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 1.0f, -1.0f, 0.0f },
                  vtkm::Vec3f{ 1.0f, -1.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ -1.0f, 1.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ -1.0f, 1.0f, 2.0f } } },
              device);
  }

  template <typename DeviceAdapter>
  void TestFrustum(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Frustum on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  With corner points" << std::endl;
    vtkm::Vec3f cornerPoints[8] = {
      { -0.5f, 0.0f, -0.5f }, // 0
      { -0.5f, 0.0f, 0.5f },  // 1
      { 0.5f, 0.0f, 0.5f },   // 2
      { 0.5f, 0.0f, -0.5f },  // 3
      { -0.5f, 1.0f, -0.5f }, // 4
      { -0.5f, 1.0f, 0.5f },  // 5
      { 1.5f, 1.0f, 0.5f },   // 6
      { 1.5f, 1.0f, -0.5f }   // 7
    };
    vtkm::cont::ImplicitFunctionHandle frustumHandle =
      vtkm::cont::make_ImplicitFunctionHandle<vtkm::Frustum>(cornerPoints);
    vtkm::Frustum* frustum = static_cast<vtkm::Frustum*>(frustumHandle.Get());
    this->Try(frustumHandle,
              { { 0.0f, 0.353553f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f } },
              { { vtkm::Vec3f{ 0.0f, -1.0f, 0.0f },
                  vtkm::Vec3f{ 0.707107f, -0.707107f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 1.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 1.0f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } },
              device);

    std::cout << "  With 6 planes" << std::endl;
    vtkm::Vec3f planePoints[6] = { { 0.0f, 0.0f, 0.0f },  { 1.0f, 1.0f, 0.0f },
                                   { -0.5f, 0.0f, 0.0f }, { 0.5f, 0.0f, 0.0f },
                                   { 0.0f, 0.0f, -0.5f }, { 0.0f, 0.0f, 0.5f } };
    vtkm::Vec3f planeNormals[6] = { { 0.0f, -1.0f, 0.0f }, { 0.707107f, 0.707107f, 0.0f },
                                    { -1.0f, 0.0f, 0.0f }, { 0.707107f, -0.707107f, 0.0f },
                                    { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } };
    frustum->SetPlanes(planePoints, planeNormals);
    this->Try(frustumHandle,
              { { 0.0f, 0.353553f, 0.5f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f } },
              { { vtkm::Vec3f{ 0.0f, -1.0f, 0.0f },
                  vtkm::Vec3f{ 0.707107f, -0.707107f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 0.707107f, 0.707107f, 0.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } },
              device);
  }

  template <typename DeviceAdapter>
  void TestPlane(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Plane on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  Default plane" << std::endl;
    vtkm::cont::ImplicitFunctionHandle planeHandle =
      vtkm::cont::make_ImplicitFunctionHandle(vtkm::Plane());
    vtkm::Plane* plane = static_cast<vtkm::Plane*>(planeHandle.Get());
    this->Try(planeHandle,
              { { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f } },
              { { vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } },
              device);

    std::cout << "  Normal of length 2" << std::endl;
    plane->SetOrigin({ 1.0f, 1.0f, 1.0f });
    plane->SetNormal({ 0.0f, 0.0f, 2.0f });
    this->Try(planeHandle,
              { { -2.0f, -2.0f, 0.0f, 0.0f, -2.0f, -2.0f, 0.0f, 0.0f } },
              { { vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f } } },
              device);

    std::cout << "  Oblique plane" << std::endl;
    plane->SetOrigin({ 0.5f, 0.5f, 0.5f });
    plane->SetNormal({ 1.0f, 0.0f, 1.0f });
    this->Try(planeHandle,
              { { -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f } },
              { { vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
                  vtkm::Vec3f{ 1.0f, 0.0f, 1.0f } } },
              device);

    std::cout << "  Another oblique plane" << std::endl;
    plane->SetNormal({ -1.0f, 0.0f, -1.0f });
    this->Try(planeHandle,
              { { 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f } },
              { { vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
                  vtkm::Vec3f{ -1.0f, 0.0f, -1.0f } } },
              device);
  }

  template <typename DeviceAdapter>
  void TestSphere(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Sphere on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  Default sphere" << std::endl;
    vtkm::Sphere sphere;
    vtkm::cont::ImplicitFunctionHandle sphereHandle(&sphere, false);
    this->Try(sphereHandle,
              { { -0.25f, 0.75f, 1.75f, 0.75f, 0.75f, 1.75f, 2.75f, 1.75f } },
              { { vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ 2.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 2.0f, 0.0f },
                  vtkm::Vec3f{ 2.0f, 2.0f, 0.0f },
                  vtkm::Vec3f{ 2.0f, 2.0f, 2.0f },
                  vtkm::Vec3f{ 0.0f, 2.0f, 2.0f } } },
              device);

    std::cout << "  Shifted and scaled sphere" << std::endl;
    sphere.SetCenter({ 1.0f, 1.0f, 1.0f });
    sphere.SetRadius(1.0f);
    this->Try(sphereHandle,
              { { 2.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f } },
              { { vtkm::Vec3f{ -2.0f, -2.0f, -2.0f },
                  vtkm::Vec3f{ 0.0f, -2.0f, -2.0f },
                  vtkm::Vec3f{ 0.0f, -2.0f, 0.0f },
                  vtkm::Vec3f{ -2.0f, -2.0f, 0.0f },
                  vtkm::Vec3f{ -2.0f, 0.0f, -2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, -2.0f },
                  vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
                  vtkm::Vec3f{ -2.0f, 0.0f, 0.0f } } },
              device);
  }

  vtkm::cont::DataSet Input;
};
}
}
} // vtmk::cont::testing

#endif //vtk_m_cont_testing_TestingImplicitFunction_h
