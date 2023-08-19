//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/ImplicitFunction.h>

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <array>

namespace
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
};

constexpr std::array<vtkm::Vec3f, 8> points_g = { { { 0, 0, 0 },
                                                    { 1, 0, 0 },
                                                    { 1, 0, 1 },
                                                    { 0, 0, 1 },
                                                    { 0, 1, 0 },
                                                    { 1, 1, 0 },
                                                    { 1, 1, 1 },
                                                    { 0, 1, 1 } } };

template <typename ImplicitFunctionType>
void EvaluateOnCoordinates(const ImplicitFunctionType& function,
                           vtkm::cont::ArrayHandle<vtkm::FloatDefault>& values,
                           vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& gradients)
{
  vtkm::cont::Invoker invoke;
  auto points = vtkm::cont::make_ArrayHandle(
    points_g.data(), static_cast<vtkm::Id>(points_g.size()), vtkm::CopyFlag::Off);
  invoke(EvaluateImplicitFunction{}, points, values, gradients, function);
}

template <typename ImplicitFunctorType>
void Try(ImplicitFunctorType& function,
         const std::array<vtkm::FloatDefault, 8>& expectedValues,
         const std::array<vtkm::Vec3f, 8>& expectedGradients)
{
  auto expectedValuesArray =
    vtkm::cont::make_ArrayHandle(expectedValues.data(), 8, vtkm::CopyFlag::Off);
  auto expectedGradientsArray =
    vtkm::cont::make_ArrayHandle(expectedGradients.data(), 8, vtkm::CopyFlag::Off);

  {
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
    EvaluateOnCoordinates(function, values, gradients);

    VTKM_TEST_ASSERT(test_equal_ArrayHandles(values, expectedValuesArray));
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(gradients, expectedGradientsArray));
  }

  {
    vtkm::ImplicitFunctionMultiplexer<ImplicitFunctorType> functionChoose(function);
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
    EvaluateOnCoordinates(functionChoose, values, gradients);

    VTKM_TEST_ASSERT(test_equal_ArrayHandles(values, expectedValuesArray));
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(gradients, expectedGradientsArray));
  }

  {
    vtkm::ImplicitFunctionGeneral functionChoose(function);
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> gradients;
    EvaluateOnCoordinates(functionChoose, values, gradients);

    VTKM_TEST_ASSERT(test_equal_ArrayHandles(values, expectedValuesArray));
    VTKM_TEST_ASSERT(test_equal_ArrayHandles(gradients, expectedGradientsArray));
  }
}

void TestBox()
{
  std::cout << "Testing vtkm::Box\n";

  std::cout << "  default box" << std::endl;
  vtkm::Box box;
  Try(box,
      { { -0.5f, 0.5f, 0.707107f, 0.5f, 0.5f, 0.707107f, 0.866025f, 0.707107f } },
      { { vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 0.707107f, 0.0f, 0.707107f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 1.0f, 0.0f },
          vtkm::Vec3f{ 0.707107f, 0.707107f, 0.0f },
          vtkm::Vec3f{ 0.57735f, 0.57735f, 0.57735f },
          vtkm::Vec3f{ 0.0f, 0.707107f, 0.707107f } } });

  std::cout << "  Specified min/max box" << std::endl;
  box.SetMinPoint({ 0.0f, -0.5f, -0.5f });
  box.SetMaxPoint({ 1.5f, 1.5f, 0.5f });
  Try(box,
      { { 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.5f } },
      { { vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } });

  std::cout << "  Specified bounds box" << std::endl;
  box.SetBounds({ vtkm::Range(0.0, 1.5), vtkm::Range(-0.5, 1.5), vtkm::Range(-0.5, 0.5) });
  Try(box,
      { { 0.0f, -0.5f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.5f } },
      { { vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } });
}

void TestCylinder()
{
  std::cout << "Testing vtkm::Cylinder\n";

  std::cout << "  Default cylinder" << std::endl;
  vtkm::Cylinder cylinder;
  Try(cylinder,
      { { -0.25f, 0.75f, 1.75f, 0.75f, -0.25f, 0.75f, 1.75f, 0.75f } },
      { { vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 2.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 2.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f } } });

  std::cout << "  Translated, scaled cylinder" << std::endl;
  cylinder.SetCenter({ 0.0f, 0.0f, 1.0f });
  cylinder.SetAxis({ 0.0f, 1.0f, 0.0f });
  cylinder.SetRadius(1.0f);
  Try(cylinder,
      { { 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f } },
      { { vtkm::Vec3f{ 0.0f, 0.0f, -2.0f },
          vtkm::Vec3f{ 2.0f, 0.0f, -2.0f },
          vtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, -2.0f },
          vtkm::Vec3f{ 2.0f, 0.0f, -2.0f },
          vtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 0.0f } } });

  std::cout << "  Non-unit axis" << std::endl;
  cylinder.SetCenter({ 0.0f, 0.0f, 0.0f });
  cylinder.SetAxis({ 1.0f, 1.0f, 0.0f });
  cylinder.SetRadius(1.0f);
  Try(cylinder,
      { { -1.0f, -0.5f, 0.5f, 0.0f, -0.5f, -1.0f, 0.0f, 0.5f } },
      { { vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 1.0f, -1.0f, 0.0f },
          vtkm::Vec3f{ 1.0f, -1.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ -1.0f, 1.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ -1.0f, 1.0f, 2.0f } } });
}

void TestFrustum()
{
  std::cout << "Testing vtkm::Frustum\n";

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
  Try(frustum,
      { { 0.0f, 0.353553f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f } },
      { { vtkm::Vec3f{ 0.0f, -1.0f, 0.0f },
          vtkm::Vec3f{ 0.707107f, -0.707107f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 1.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 1.0f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } });

  std::cout << "  With 6 planes" << std::endl;
  vtkm::Vec3f planePoints[6] = {
    { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f },  { -0.5f, 0.0f, 0.0f },
    { 0.5f, 0.0f, 0.0f }, { 0.0f, 0.0f, -0.5f }, { 0.0f, 0.0f, 0.5f }
  };
  vtkm::Vec3f planeNormals[6] = { { 0.0f, -1.0f, 0.0f }, { 0.707107f, 0.707107f, 0.0f },
                                  { -1.0f, 0.0f, 0.0f }, { 0.707107f, -0.707107f, 0.0f },
                                  { 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f } };
  frustum.SetPlanes(planePoints, planeNormals);
  Try(frustum,
      { { 0.0f, 0.353553f, 0.5f, 0.5f, -0.5f, 0.0f, 0.5f, 0.5f } },
      { { vtkm::Vec3f{ 0.0f, -1.0f, 0.0f },
          vtkm::Vec3f{ 0.707107f, -0.707107f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ -1.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 0.707107f, 0.707107f, 0.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } });
}

void TestPlane()
{
  std::cout << "Testing vtkm::Plane\n";

  std::cout << "  Default plane" << std::endl;
  vtkm::Plane plane;
  Try(plane,
      { { 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f } },
      { { vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 1.0f } } });

  std::cout << "  Normal of length 2" << std::endl;
  plane.SetOrigin({ 1.0f, 1.0f, 1.0f });
  plane.SetNormal({ 0.0f, 0.0f, 2.0f });
  Try(plane,
      { { -2.0f, -2.0f, 0.0f, 0.0f, -2.0f, -2.0f, 0.0f, 0.0f } },
      { { vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f } } });

  std::cout << "  Oblique plane" << std::endl;
  plane.SetOrigin({ 0.5f, 0.5f, 0.5f });
  plane.SetNormal({ 1.0f, 0.0f, 1.0f });
  Try(plane,
      { { -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f } },
      { { vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 1.0f },
          vtkm::Vec3f{ 1.0f, 0.0f, 1.0f } } });

  std::cout << "  Another oblique plane" << std::endl;
  plane.SetNormal({ -1.0f, 0.0f, -1.0f });
  Try(plane,
      { { 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f } },
      { { vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
          vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
          vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
          vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
          vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
          vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
          vtkm::Vec3f{ -1.0f, 0.0f, -1.0f },
          vtkm::Vec3f{ -1.0f, 0.0f, -1.0f } } });
}

void TestSphere()
{
  std::cout << "Testing vtkm::Sphere\n";

  std::cout << "  Default sphere" << std::endl;
  vtkm::Sphere sphere;
  Try(sphere,
      { { -0.25f, 0.75f, 1.75f, 0.75f, 0.75f, 1.75f, 2.75f, 1.75f } },
      { { vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 2.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ 2.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 2.0f, 0.0f },
          vtkm::Vec3f{ 2.0f, 2.0f, 0.0f },
          vtkm::Vec3f{ 2.0f, 2.0f, 2.0f },
          vtkm::Vec3f{ 0.0f, 2.0f, 2.0f } } });

  std::cout << "  Shifted and scaled sphere" << std::endl;
  sphere.SetCenter({ 1.0f, 1.0f, 1.0f });
  sphere.SetRadius(1.0f);
  Try(sphere,
      { { 2.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f } },
      { { vtkm::Vec3f{ -2.0f, -2.0f, -2.0f },
          vtkm::Vec3f{ 0.0f, -2.0f, -2.0f },
          vtkm::Vec3f{ 0.0f, -2.0f, 0.0f },
          vtkm::Vec3f{ -2.0f, -2.0f, 0.0f },
          vtkm::Vec3f{ -2.0f, 0.0f, -2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, -2.0f },
          vtkm::Vec3f{ 0.0f, 0.0f, 0.0f },
          vtkm::Vec3f{ -2.0f, 0.0f, 0.0f } } });
}

void TestMultiPlane()
{
  std::cout << "Testing vtkm::MultiPlane\n";
  std::cout << "  3 axis aligned planes intersected at (1, 1, 1)" << std::endl;
  vtkm::MultiPlane<3> TriplePlane;
  //insert xy plane
  TriplePlane.AddPlane(vtkm::Vec3f{ 1.0f, 1.0f, 0.0f }, vtkm::Vec3f{ 0.0f, 0.0f, 1.0f });
  //insert yz plane
  TriplePlane.AddPlane(vtkm::Vec3f{ 0.0f, 1.0f, 1.0f }, vtkm::Vec3f{ 1.0f, 0.0f, 0.0f });
  //insert xz plane
  TriplePlane.AddPlane(vtkm::Vec3f{ 1.0f, 0.0f, 1.0f }, vtkm::Vec3f{ 0.0f, 1.0f, 0.0f });
  Try(TriplePlane,
      { { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f } },
      { {
        vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
        vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
        vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
        vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
        vtkm::Vec3f{ 0.0f, 1.0f, 0.0f },
        vtkm::Vec3f{ 1.0f, 0.0f, 0.0f },
        vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
        vtkm::Vec3f{ 0.0f, 0.0f, 1.0f },
      } });
  std::cout << "  test MultiPlane copy" << std::endl;
  vtkm::MultiPlane<4> QuadPlane1(TriplePlane);
  vtkm::MultiPlane<4> QuadPlane2 = TriplePlane;
  for (int i = 0; i < 3; i++)
  {
    VTKM_TEST_ASSERT(QuadPlane1.GetPlane(i).GetOrigin() == TriplePlane.GetPlane(i).GetOrigin());
    VTKM_TEST_ASSERT(QuadPlane1.GetPlane(i).GetNormal() == TriplePlane.GetPlane(i).GetNormal());
    VTKM_TEST_ASSERT(QuadPlane2.GetPlane(i).GetOrigin() == TriplePlane.GetPlane(i).GetOrigin());
    VTKM_TEST_ASSERT(QuadPlane1.GetPlane(i).GetNormal() == TriplePlane.GetPlane(i).GetNormal());
  }
}

void Run()
{
  TestBox();
  TestCylinder();
  TestFrustum();
  TestPlane();
  TestSphere();
  TestMultiPlane();
}

} // anonymous namespace

int UnitTestImplicitFunction(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
