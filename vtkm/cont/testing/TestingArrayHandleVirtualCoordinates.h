//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
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
  typedef void ControlSignature(FieldIn<FieldCommon> in, FieldOut<FieldCommon> out);
  typedef _2 ExecutionSignature(_1);

  template <typename T>
  VTKM_EXEC T operator()(const T& in) const
  {
    return in;
  }
};

} // anonymous namespace

template <typename DeviceAdapter>
class TestingArrayHandleVirtualCoordinates
{
private:
  using ArrayHandleRectilinearCoords =
    vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                            vtkm::cont::ArrayHandle<vtkm::FloatDefault>>;

  template <typename T, typename InStorageTag, typename OutStorageTag>
  static void TestVirtualAccess(const vtkm::cont::ArrayHandle<T, InStorageTag>& in,
                                vtkm::cont::ArrayHandle<T, OutStorageTag>& out)
  {
    vtkm::worklet::DispatcherMapField<CopyWorklet, DeviceAdapter>().Invoke(
      vtkm::cont::ArrayHandleVirtualCoordinates(in),
      vtkm::cont::ArrayHandleVirtualCoordinates(out));

    VTKM_TEST_ASSERT(test_equal_portals(in.GetPortalConstControl(), out.GetPortalConstControl()),
                     "Input and output portals don't match");
  }

  static void TestAll()
  {
    using PointType = vtkm::Vec<vtkm::FloatDefault, 3>;
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
    TestVirtualAccess(vtkm::cont::ArrayHandleUniformPointCoordinates(vtkm::Id3(4, 4, 4)), out);

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
  }

public:
  static int Run() { return vtkm::cont::testing::Testing::Run(TestAll); }
};
}
}
} // vtkm::cont::testing


#endif // vtk_m_cont_testing_TestingArrayHandleVirtualCoordinates_h
