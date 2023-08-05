//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/field_transform/Warp.h>

#include <vector>

namespace
{

constexpr vtkm::Id DIM = 5;

template <typename T>
vtkm::cont::DataSet MakeWarpTestDataSet()
{
  using vecType = vtkm::Vec<T, 3>;
  vtkm::cont::DataSet dataSet;

  std::vector<vecType> coordinates;
  std::vector<vecType> vec1;
  std::vector<T> scalarFactor;
  std::vector<vecType> vec2;
  for (vtkm::Id j = 0; j < DIM; ++j)
  {
    T z = static_cast<T>(j) / static_cast<T>(DIM - 1);
    for (vtkm::Id i = 0; i < DIM; ++i)
    {
      T x = static_cast<T>(i) / static_cast<T>(DIM - 1);
      T y = (x * x + z * z) / static_cast<T>(2.0);
      coordinates.push_back(vtkm::make_Vec(x, y, z));
      vec1.push_back(vtkm::make_Vec(x, y, y));
      scalarFactor.push_back(static_cast<T>(j * DIM + i));
      vec2.push_back(vtkm::make_Vec<T>(0, 0, static_cast<T>(j * DIM + i)));
    }
  }

  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, vtkm::CopyFlag::On));

  dataSet.AddPointField("vec1", vec1);
  dataSet.AddPointField("scalarfactor", scalarFactor);
  dataSet.AddPointField("vec2", vec2);

  vecType normal = vtkm::make_Vec<T>(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0));
  vtkm::cont::ArrayHandleConstant<vecType> vectorAH =
    vtkm::cont::make_ArrayHandleConstant(normal, DIM * DIM);
  dataSet.AddPointField("normal", vectorAH);

  return dataSet;
}

void CheckResult(const vtkm::filter::field_transform::Warp& filter,
                 const vtkm::cont::DataSet& result)
{
  VTKM_TEST_ASSERT(result.HasPointField(filter.GetOutputFieldName()));
  using vecType = vtkm::Vec3f;
  vtkm::cont::ArrayHandle<vecType> outputArray;
  result.GetPointField(filter.GetOutputFieldName()).GetData().AsArrayHandle(outputArray);
  auto outPortal = outputArray.ReadPortal();

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> sfArray;
  result.GetPointField("scalarfactor").GetData().AsArrayHandle(sfArray);
  auto sfPortal = sfArray.ReadPortal();

  for (vtkm::Id j = 0; j < DIM; ++j)
  {
    vtkm::FloatDefault z =
      static_cast<vtkm::FloatDefault>(j) / static_cast<vtkm::FloatDefault>(DIM - 1);
    for (vtkm::Id i = 0; i < DIM; ++i)
    {
      vtkm::FloatDefault x =
        static_cast<vtkm::FloatDefault>(i) / static_cast<vtkm::FloatDefault>(DIM - 1);
      vtkm::FloatDefault y = (x * x + z * z) / static_cast<vtkm::FloatDefault>(2.0);
      vtkm::FloatDefault targetZ = filter.GetUseCoordinateSystemAsField()
        ? z + static_cast<vtkm::FloatDefault>(2 * sfPortal.Get(j * DIM + i))
        : y + static_cast<vtkm::FloatDefault>(2 * sfPortal.Get(j * DIM + i));
      auto point = outPortal.Get(j * DIM + i);
      VTKM_TEST_ASSERT(test_equal(point[0], x), "Wrong result of x value for warp scalar");
      VTKM_TEST_ASSERT(test_equal(point[1], y), "Wrong result of y value for warp scalar");
      VTKM_TEST_ASSERT(test_equal(point[2], targetZ), "Wrong result of z value for warp scalar");
    }
  }
}

void TestWarpFilter()
{
  std::cout << "Testing Warp filter" << std::endl;
  vtkm::cont::DataSet ds = MakeWarpTestDataSet<vtkm::FloatDefault>();
  vtkm::FloatDefault scale = 2;

  {
    std::cout << "   First field as coordinates" << std::endl;
    vtkm::filter::field_transform::Warp filter;
    filter.SetScaleFactor(scale);
    filter.SetUseCoordinateSystemAsField(true);
    filter.SetDirectionField("normal");
    filter.SetScaleField("scalarfactor");
    vtkm::cont::DataSet result = filter.Execute(ds);
    CheckResult(filter, result);
  }

  {
    std::cout << "   First field as a vector" << std::endl;
    vtkm::filter::field_transform::Warp filter;
    filter.SetScaleFactor(scale);
    filter.SetActiveField("vec1");
    filter.SetDirectionField("normal");
    filter.SetScaleField("scalarfactor");
    vtkm::cont::DataSet result = filter.Execute(ds);
    CheckResult(filter, result);
  }

  {
    std::cout << "   Constant direction (warp scalar)" << std::endl;
    vtkm::filter::field_transform::Warp filter;
    filter.SetScaleFactor(scale);
    filter.SetUseCoordinateSystemAsField(true);
    filter.SetConstantDirection({ 0.0, 0.0, 1.0 });
    filter.SetScaleField("scalarfactor");
    vtkm::cont::DataSet result = filter.Execute(ds);
    CheckResult(filter, result);
  }

  {
    std::cout << "   Constant scale (warp vector)" << std::endl;
    vtkm::filter::field_transform::Warp filter;
    filter.SetScaleFactor(scale);
    filter.SetActiveField("vec1");
    filter.SetDirectionField("vec2");
    vtkm::cont::DataSet result = filter.Execute(ds);
    CheckResult(filter, result);
  }
}
}

int UnitTestWarpFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestWarpFilter, argc, argv);
}
