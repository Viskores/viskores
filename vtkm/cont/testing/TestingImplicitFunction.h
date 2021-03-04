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

#include <vtkm/ImplicitFunction.h>

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterList.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
#include <vtkm/cont/ImplicitFunctionHandle.h>
#endif //!VTKM_NO_DEPRECATED_VIRTUAL

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
  using ControlSignature = void(FieldIn, FieldOut, FieldOut, ExecObject);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename VecType, typename ScalarType, typename FunctionType>
  VTKM_EXEC void operator()(const VecType& point,
                            ScalarType& val,
                            VecType& gradient,
                            const FunctionType& function) const
  {
    val = function.Value(point);
    gradient = function.Gradient(point);
  }

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
  VTKM_DEPRECATED_SUPPRESS_BEGIN
  template <typename VecType, typename ScalarType, typename FunctionType>
  VTKM_EXEC void operator()(const VecType& point,
                            ScalarType& val,
                            VecType& gradient,
                            const FunctionType* function) const
  {
    val = function->Value(point);
    gradient = function->Gradient(point);
  }
  VTKM_DEPRECATED_SUPPRESS_END
#endif //!VTKM_NO_DEPRECATED_VIRTUAL
};

template <typename ImplicitFunctionType>
void EvaluateOnCoordinates(vtkm::cont::CoordinateSystem points,
                           const ImplicitFunctionType& function,
                           vtkm::cont::ArrayHandle<vtkm::FloatDefault>& values,
                           vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& gradients,
                           vtkm::cont::DeviceAdapterId device)
{
  vtkm::cont::Invoker invoke{ device };
  invoke(EvaluateImplicitFunction{}, points.GetDataAsMultiplexer(), values, gradients, function);
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
  template <typename ImplicitFunctorType>
  void Try(ImplicitFunctorType& function,
           const std::array<vtkm::FloatDefault, 8>& expectedValues,
           const std::array<vtkm::Vec3f, 8>& expectedGradients,
           vtkm::cont::DeviceAdapterId device)
  {
    auto expectedValuesArray =
      vtkm::cont::make_ArrayHandle(expectedValues.data(), 8, vtkm::CopyFlag::Off);
    auto expectedGradientsArray =
      vtkm::cont::make_ArrayHandle(expectedGradients.data(), 8, vtkm::CopyFlag::Off);

    {
      vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
      vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
      implicit_function_detail::EvaluateOnCoordinates(
        this->Input.GetCoordinateSystem(0), function, values, gradients, device);

      VTKM_TEST_ASSERT(test_equal_ArrayHandles(values, expectedValuesArray));
      VTKM_TEST_ASSERT(test_equal_ArrayHandles(gradients, expectedGradientsArray));
    }

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
    VTKM_DEPRECATED_SUPPRESS_BEGIN
    {
      vtkm::cont::ImplicitFunctionHandle functionHandle(&function, false);
      vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
      vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
      implicit_function_detail::EvaluateOnCoordinates(
        this->Input.GetCoordinateSystem(0), functionHandle, values, gradients, device);

      VTKM_TEST_ASSERT(test_equal_ArrayHandles(values, expectedValuesArray));
      VTKM_TEST_ASSERT(test_equal_ArrayHandles(gradients, expectedGradientsArray));
    }
    VTKM_DEPRECATED_SUPPRESS_END
#endif //!VTKM_NO_DEPRECATED_VIRTUAL

    {
      vtkm::ImplicitFunctionMultiplexer<ImplicitFunctorType> functionChoose(function);
      vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
      vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
      implicit_function_detail::EvaluateOnCoordinates(
        this->Input.GetCoordinateSystem(0), functionChoose, values, gradients, device);

      VTKM_TEST_ASSERT(test_equal_ArrayHandles(values, expectedValuesArray));
      VTKM_TEST_ASSERT(test_equal_ArrayHandles(gradients, expectedGradientsArray));
    }

    {
      vtkm::ImplicitFunctionGeneral functionChoose(function);
      vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
      vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
      implicit_function_detail::EvaluateOnCoordinates(
        this->Input.GetCoordinateSystem(0), functionChoose, values, gradients, device);

      VTKM_TEST_ASSERT(test_equal_ArrayHandles(values, expectedValuesArray));
      VTKM_TEST_ASSERT(test_equal_ArrayHandles(gradients, expectedGradientsArray));
    }
  }

  template <typename DeviceAdapter>
  void TestBox(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::Box on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName() << "\n";

    std::cout << "  default box" << std::endl;
    vtkm::Box box;
    this->Try(box,
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
    this->Try(box,
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
    this->Try(box,
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
    this->Try(cylinder,
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
    this->Try(cylinder,
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
    this->Try(cylinder,
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
    vtkm::Frustum frustum{ cornerPoints };
    this->Try(frustum,
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
    frustum.SetPlanes(planePoints, planeNormals);
    this->Try(frustum,
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
    vtkm::Plane plane;
    this->Try(plane,
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
    plane.SetOrigin({ 1.0f, 1.0f, 1.0f });
    plane.SetNormal({ 0.0f, 0.0f, 2.0f });
    this->Try(plane,
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
    plane.SetOrigin({ 0.5f, 0.5f, 0.5f });
    plane.SetNormal({ 1.0f, 0.0f, 1.0f });
    this->Try(plane,
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
    plane.SetNormal({ -1.0f, 0.0f, -1.0f });
    this->Try(plane,
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
    this->Try(sphere,
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
    this->Try(sphere,
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
