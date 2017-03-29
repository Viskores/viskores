//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_testing_TestingImplicitFunction_h
#define vtk_m_cont_testing_TestingImplicitFunction_h

#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterListTag.h>
#include <vtkm/cont/ImplicitFunction.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/internal/Configure.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <array>


namespace vtkm {
namespace cont {
namespace testing {

namespace implicit_function_detail {

class EvaluateImplicitFunction : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<Vec3>, FieldOut<Scalar>);
  typedef void ExecutionSignature(_1, _2);

  EvaluateImplicitFunction(const vtkm::exec::ImplicitFunction &function)
    : Function(function)
  { }

  template<typename VecType, typename ScalarType>
  VTKM_EXEC
  void operator()(const VecType &point, ScalarType &val) const
  {
    val = this->Function.Value(point);
  }

private:
  vtkm::exec::ImplicitFunction Function;
};

template <typename DeviceAdapter>
void EvaluateOnCoordinates(vtkm::cont::CoordinateSystem points,
                           const vtkm::cont::ImplicitFunction &function,
                           vtkm::cont::ArrayHandle<vtkm::FloatDefault> &values,
                           DeviceAdapter device)
{
  typedef vtkm::worklet::DispatcherMapField<EvaluateImplicitFunction, DeviceAdapter>
    EvalDispatcher;

  EvaluateImplicitFunction eval(function.PrepareForExecution(device));
  EvalDispatcher(eval).Invoke(points, values);
}

template <std::size_t N>
bool TestArrayEqual(const vtkm::cont::ArrayHandle<vtkm::FloatDefault> &result,
                    const std::array<vtkm::FloatDefault, N> &expected)
{
  if (result.GetNumberOfValues() != N)
  {
    return false;
  }

  vtkm::cont::ArrayHandle<vtkm::FloatDefault>::PortalConstControl portal =
    result.GetPortalConstControl();
  for (std::size_t i = 0; i < N; ++i)
  {
    if (!test_equal(portal.Get(static_cast<vtkm::Id>(i)), expected[i]))
    {
      return false;
    }
  }
  return true;
}

} // anonymous namespace

class TestingImplicitFunction
{
public:
  TestingImplicitFunction()
    : Input(vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSet2())
  {
  }

  template<typename DeviceAdapter>
  void Run(DeviceAdapter device)
  {
    this->TestSphere(device);
    this->TestPlane(device);
    this->TestBox(device);
  }

private:
  template <typename DeviceAdapter>
  void TestSphere(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::cont::Sphere on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
              << "\n";

    vtkm::cont::Sphere sphere({0.0f, 0.0f, 0.0f}, 1.0f);
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0), sphere, values, device);

    std::array<vtkm::FloatDefault, 8> expected =
      { {-1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 2.0f, 1.0f} };
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expected),
                     "Result does not match expected values");
  }

  template <typename DeviceAdapter>
  void TestPlane(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::cont::Plane on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
              << "\n";

    vtkm::cont::Plane plane({0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 1.0f});
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;

    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0), plane, values, device);
    std::array<vtkm::FloatDefault, 8> expected1 =
      { {-1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f} };
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expected1),
                     "Result does not match expected values");

    plane.SetNormal({-1.0f, 0.0f, -1.0f});
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0), plane, values, device);
    std::array<vtkm::FloatDefault, 8> expected2 =
      { {1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f} };
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expected2),
                     "Result does not match expected values");
  }

  template <typename DeviceAdapter>
  void TestBox(DeviceAdapter device)
  {
    std::cout << "Testing vtkm::cont::Box on "
              << vtkm::cont::DeviceAdapterTraits<DeviceAdapter>::GetName()
              << "\n";

    vtkm::cont::Box box({0.0f, -0.5f, -0.5f}, {1.5f, 1.5f, 0.5f});
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
    implicit_function_detail::EvaluateOnCoordinates(
      this->Input.GetCoordinateSystem(0), box, values, device);

    std::array<vtkm::FloatDefault, 8> expected =
      { {0.0f, -0.5f, 0.5f, 0.5f, 0.0f, -0.5f, 0.5f, 0.5f} };
    VTKM_TEST_ASSERT(implicit_function_detail::TestArrayEqual(values, expected),
                     "Result does not match expected values");
  }

  vtkm::cont::DataSet Input;
};

}
}
} // vtmk::cont::testing

#endif //vtk_m_cont_testing_TestingImplicitFunction_h
