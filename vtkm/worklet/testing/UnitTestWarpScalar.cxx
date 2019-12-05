//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WarpScalar.h>

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetFieldAdd.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace
{
template <typename T>
vtkm::cont::DataSet MakeWarpScalarTestDataSet()
{
  vtkm::cont::DataSet dataSet;

  std::vector<vtkm::Vec<T, 3>> coordinates;
  std::vector<T> scaleFactor;
  const vtkm::Id dim = 5;
  for (vtkm::Id i = 0; i < dim; ++i)
  {
    T z = static_cast<T>(i);
    for (vtkm::Id j = 0; j < dim; ++j)
    {
      T x = static_cast<T>(j);
      T y = static_cast<T>(j + 1);
      coordinates.push_back(vtkm::make_Vec(x, y, z));
      scaleFactor.push_back(static_cast<T>(i * dim + j));
    }
  }

  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, vtkm::CopyFlag::On));
  vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "scalefactor", scaleFactor);
  return dataSet;
}
}

void TestWarpScalar()
{
  std::cout << "Testing WarpScalar Worklet" << std::endl;
  using vecType = vtkm::Vec3f;

  vtkm::cont::DataSet ds = MakeWarpScalarTestDataSet<vtkm::FloatDefault>();

  vtkm::FloatDefault scaleAmount = 2;
  vtkm::cont::ArrayHandle<vecType> result;

  vecType normal = vtkm::make_Vec<vtkm::FloatDefault>(static_cast<vtkm::FloatDefault>(0.0),
                                                      static_cast<vtkm::FloatDefault>(0.0),
                                                      static_cast<vtkm::FloatDefault>(1.0));
  auto coordinate = ds.GetCoordinateSystem().GetData();
  vtkm::Id nov = coordinate.GetNumberOfValues();
  vtkm::cont::ArrayHandleConstant<vecType> normalAH =
    vtkm::cont::make_ArrayHandleConstant(normal, nov);

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> scaleFactorArray;
  auto scaleFactor = ds.GetField("scalefactor").GetData().ResetTypes(vtkm::TypeListFieldScalar());
  scaleFactor.CopyTo(scaleFactorArray);
  auto sFAPortal = scaleFactorArray.GetPortalControl();

  vtkm::worklet::WarpScalar warpWorklet;
  warpWorklet.Run(ds.GetCoordinateSystem(), normalAH, scaleFactor, scaleAmount, result);
  auto resultPortal = result.GetPortalConstControl();

  for (vtkm::Id i = 0; i < nov; i++)
  {
    for (vtkm::Id j = 0; j < 3; j++)
    {
      vtkm::FloatDefault ans =
        coordinate.GetPortalConstControl().Get(i)[static_cast<vtkm::IdComponent>(j)] +
        scaleAmount * normal[static_cast<vtkm::IdComponent>(j)] * sFAPortal.Get(i);
      VTKM_TEST_ASSERT(test_equal(ans, resultPortal.Get(i)[static_cast<vtkm::IdComponent>(j)]),
                       " Wrong result for WarpVector worklet");
    }
  }
}

int UnitTestWarpScalar(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestWarpScalar, argc, argv);
}
