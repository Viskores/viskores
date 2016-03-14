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
#ifndef vtk_m_testing_TestingImplicitFunctions_h
#define vtk_m_testing_TestingImplicitFunctions_h

#include <vtkm/ImplicitFunctions.h>
#include <vtkm/Math.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

VTKM_THIRDPARTY_PRE_INCLUDE
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>
VTKM_THIRDPARTY_POST_INCLUDE


namespace vtkm {
namespace testing {

namespace {

inline vtkm::FloatDefault GetRandomValue()
{
  static boost::random::mt19937 gen;
  boost::random::uniform_real_distribution<vtkm::FloatDefault> dist(-7.0, 7.0);
  return dist(gen);
}

}

template<typename DeviceAdapter>
struct TestingImplicitFunctions
{
  typedef vtkm::Vec<vtkm::FloatDefault, 3> FVec3;

  template<typename ImplicitFunction>
  class EvaluateImplicitFunction : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<Vec3>, FieldOut<Scalar>);
    typedef void ExecutionSignature(_1, _2);

    EvaluateImplicitFunction(const ImplicitFunction &function)
      : Function(function)
    { }

    template<typename VecType, typename ScalarType>
    VTKM_EXEC_EXPORT
    void operator()(const VecType &point, ScalarType &val) const
    {
      val = this->Function.Value(point);
    }

  private:
    ImplicitFunction Function;
  };


  static void TestSphereValue()
  {
    typedef EvaluateImplicitFunction<vtkm::Sphere> EvalWorklet;
    typedef vtkm::worklet::DispatcherMapField<EvalWorklet, DeviceAdapter>
        EvalDispatcher;

    FVec3 center(GetRandomValue(), GetRandomValue(), GetRandomValue());
    FloatDefault radius = vtkm::Abs(GetRandomValue());
    vtkm::Sphere sphere(center, radius);

    FloatDefault r = radius;
    FVec3 cube[8] = { center + FVec3(0, 0, 0),
                      center + FVec3(r, 0, 0),
                      center + FVec3(0, r, 0),
                      center + FVec3(r, r, 0),
                      center + FVec3(0, 0, r),
                      center + FVec3(r, 0, r),
                      center + FVec3(0, r, r),
                      center + FVec3(r, r, r)
                    };
    vtkm::cont::ArrayHandle<FVec3> points = vtkm::cont::make_ArrayHandle(cube, 8);

    EvalWorklet eval(sphere);
    vtkm::cont::ArrayHandle<FloatDefault> values;
    EvalDispatcher(eval).Invoke(points, values);

    vtkm::cont::ArrayHandle<FloatDefault>::PortalConstControl portal =
        values.GetPortalConstControl();

    bool success = (portal.Get(0) == -(r*r)) &&
                   test_equal(portal.Get(1), FloatDefault(0.0) ) &&
                   test_equal(portal.Get(2), FloatDefault(0.0) ) &&
                   test_equal(portal.Get(4), FloatDefault(0.0) ) &&
                   (portal.Get(3) > 0.0) &&
                   (portal.Get(5) > 0.0) &&
                   (portal.Get(6) > 0.0) &&
                   (portal.Get(7) > 0.0);

    VTKM_TEST_ASSERT(success, "Sphere: did not get expected results.");
  }

  static void TestPlaneValue()
  {
    typedef EvaluateImplicitFunction<vtkm::Plane> EvalWorklet;
    typedef vtkm::worklet::DispatcherMapField<EvalWorklet, DeviceAdapter>
        EvalDispatcher;

    FVec3 origin(GetRandomValue(), GetRandomValue(), GetRandomValue());
    FVec3 normal(GetRandomValue(), GetRandomValue(), GetRandomValue());
    vtkm::Plane plane(origin, normal);

    FloatDefault t[] = { -2.0, -1.0, 0.0, 1.0, 2.0 };
    vtkm::cont::ArrayHandle<FVec3> points;
    points.Allocate(5);
    for (int i = 0; i < 5; ++i)
    {
      points.GetPortalControl().Set(i, origin + (t[i] * normal));
    }

    EvalWorklet eval(plane);
    vtkm::cont::ArrayHandle<FloatDefault> values;
    EvalDispatcher(eval).Invoke(points, values);

    vtkm::cont::ArrayHandle<FloatDefault>::PortalConstControl portal =
        values.GetPortalConstControl();
    bool success = (portal.Get(0) < 0.0) &&
                   (portal.Get(1) < 0.0) &&
                   test_equal(portal.Get(2), FloatDefault(0.0) ) &&
                   (portal.Get(3) > 0.0) &&
                   (portal.Get(4) > 0.0);

    VTKM_TEST_ASSERT(success, "Plane: did not get expected results.");
  }

  static void Run()
  {
    for (int i = 0; i < 50; ++i)
    {
      TestSphereValue();
    }
    for (int i = 0; i < 50; ++i)
    {
      TestPlaneValue();
    }
  }
};

}
} // namespace vtkm::testing

#endif // vtk_m_testing_TestingImplicitFunctions_h
