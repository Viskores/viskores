//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingArrayHandleVirtualCoordinates_h
#define vtk_m_cont_testing_TestingArrayHandleVirtualCoordinates_h

#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/ArrayHandleVirtualCoordinates.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace cont
{
namespace testing
{

namespace
{

struct CopyWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn in, FieldOut out);
  using ExecutionSignature = _2(_1);

  template <typename T>
  VTKM_EXEC T operator()(const T& in) const
  {
    return in;
  }
};

// A dummy worklet
struct DoubleWorklet : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn in);
  typedef void ExecutionSignature(_1);
  using InputDomain = _1;

  template <typename T>
  VTKM_EXEC void operator()(T& in) const
  {
    in = in * 2;
  }
};

template <typename T, typename S>
inline void TestVirtualAccess(const vtkm::cont::ArrayHandle<T, S>& in,
                              vtkm::cont::ArrayHandle<T>& out)
{
  vtkm::worklet::DispatcherMapField<CopyWorklet>().Invoke(
    vtkm::cont::ArrayHandleVirtualCoordinates(in), vtkm::cont::ArrayHandleVirtualCoordinates(out));

  VTKM_TEST_ASSERT(test_equal_portals(in.GetPortalConstControl(), out.GetPortalConstControl()),
                   "Input and output portals don't match");
}


} // anonymous namespace

template <typename DeviceAdapter>
class TestingArrayHandleVirtualCoordinates
{
private:
  using ArrayHandleRectilinearCoords =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>;



  static void TestAll()
  {
    using PointType = vtkm::Vec3f;
    static constexpr vtkm::Id length = 64;

    vtkm::cont::ArrayHandle<PointType> out;

    std::cout << "Testing basic ArrayHandle as input\n";
    vtkm::cont::ArrayHandle<PointType> a1;
    a1.Allocate(length);
    for (vtkm::Id i = 0; i < length; ++i)
    {
      a1.GetPortalControl().Set(i, TestValue(i, PointType()));
    }
    TestVirtualAccess(a1, out);

    std::cout << "Testing ArrayHandleUniformPointCoordinates as input\n";
    auto t = vtkm::cont::ArrayHandleUniformPointCoordinates(vtkm::Id3(4, 4, 4));
    TestVirtualAccess(t, out);

    std::cout << "Testing ArrayHandleCartesianProduct as input\n";
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> c1, c2, c3;
    c1.Allocate(length);
    c2.Allocate(length);
    c3.Allocate(length);
    for (vtkm::Id i = 0; i < length; ++i)
    {
      auto p = a1.GetPortalConstControl().Get(i);
      c1.GetPortalControl().Set(i, p[0]);
      c2.GetPortalControl().Set(i, p[1]);
      c3.GetPortalControl().Set(i, p[2]);
    }
    TestVirtualAccess(vtkm::cont::make_ArrayHandleCartesianProduct(c1, c2, c3), out);

    std::cout << "Testing resources releasing on ArrayHandleVirtualCoordinates\n";
    vtkm::cont::ArrayHandleVirtualCoordinates virtualC =
      vtkm::cont::ArrayHandleVirtualCoordinates(a1);
    vtkm::worklet::DispatcherMapField<DoubleWorklet>().Invoke(a1);
    virtualC.ReleaseResourcesExecution();
    VTKM_TEST_ASSERT(a1.GetNumberOfValues() == length,
                     "ReleaseResourcesExecution"
                     " should not change the number of values on the Arrayhandle");
    VTKM_TEST_ASSERT(
      virtualC.GetNumberOfValues() == length,
      "ReleaseResources"
      " should set the number of values on the ArrayHandleVirtualCoordinates to be 0");
    virtualC.ReleaseResources();
    VTKM_TEST_ASSERT(a1.GetNumberOfValues() == 0,
                     "ReleaseResources"
                     " should set the number of values on the Arrayhandle to be 0");
  }

public:
  static int Run(int argc, char* argv[])
  {
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapter());
    return vtkm::cont::testing::Testing::Run(TestAll, argc, argv);
  }
};
}
}
} // vtkm::cont::testing


#endif // vtk_m_cont_testing_TestingArrayHandleVirtualCoordinates_h
