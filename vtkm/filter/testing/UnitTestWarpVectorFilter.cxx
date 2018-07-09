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

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/WarpVector.h>

#include <random>
#include <vector>

namespace
{
const vtkm::Id dim = 5;
template <typename T>
vtkm::cont::DataSet MakeWarpVectorTestDataSet()
{
  using vecType = vtkm::Vec<T, 3>;
  vtkm::cont::DataSet dataSet;

  std::vector<vecType> coordinates;
  std::vector<vecType> vec1;
  for (vtkm::Id j = 0; j < dim; ++j)
  {
    T z = static_cast<T>(j) / static_cast<T>(dim - 1);
    for (vtkm::Id i = 0; i < dim; ++i)
    {
      T x = static_cast<T>(i) / static_cast<T>(dim - 1);
      T y = (x * x + z * z) / static_cast<T>(2.0);
      coordinates.push_back(vtkm::make_Vec(x, y, z));
      vec1.push_back(vtkm::make_Vec(x, y, y));
    }
  }

  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, vtkm::CopyFlag::On));

  vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "vec1", vec1);

  vecType vector = vtkm::make_Vec<T>(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(2.0));
  vtkm::cont::ArrayHandleConstant<vecType> vectorAH =
    vtkm::cont::make_ArrayHandleConstant(vector, dim * dim);
  vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "vec2", vectorAH);

  return dataSet;
}

class PolicyWarpVector : public vtkm::filter::PolicyBase<PolicyWarpVector>
{
public:
  using vecType = vtkm::Vec<vtkm::FloatDefault, 3>;
  struct TypeListTagWarpVectorTags
    : vtkm::ListTagBase<vtkm::cont::ArrayHandleConstant<vecType>::StorageTag,
                        vtkm::cont::ArrayHandle<vecType>::StorageTag>
  {
  };
  using FieldStorageList = TypeListTagWarpVectorTags;
};

void CheckResult(const vtkm::filter::WarpVector& filter, const vtkm::cont::DataSet& result)
{
  VTKM_TEST_ASSERT(result.HasField("warpvector", vtkm::cont::Field::Association::POINTS),
                   "Output filed WarpVector is missing");
  using vecType = vtkm::Vec<vtkm::FloatDefault, 3>;
  vtkm::cont::ArrayHandle<vecType> outputArray;
  result.GetField("warpvector", vtkm::cont::Field::Association::POINTS)
    .GetData()
    .CopyTo(outputArray);
  auto outPortal = outputArray.GetPortalConstControl();

  for (vtkm::Id j = 0; j < dim; ++j)
  {
    vtkm::FloatDefault z =
      static_cast<vtkm::FloatDefault>(j) / static_cast<vtkm::FloatDefault>(dim - 1);
    for (vtkm::Id i = 0; i < dim; ++i)
    {
      vtkm::FloatDefault x =
        static_cast<vtkm::FloatDefault>(i) / static_cast<vtkm::FloatDefault>(dim - 1);
      vtkm::FloatDefault y = (x * x + z * z) / static_cast<vtkm::FloatDefault>(2.0);
      vtkm::FloatDefault targetZ = filter.GetUseCoordinateSystemAsPrimaryField()
        ? z + static_cast<vtkm::FloatDefault>(2 * 2)
        : y + static_cast<vtkm::FloatDefault>(2 * 2);
      auto point = outPortal.Get(j * dim + i);
      VTKM_TEST_ASSERT(point[0] == x, "Wrong result of x value for warp vector");
      VTKM_TEST_ASSERT(point[1] == y, "Wrong result of y value for warp vector");
      VTKM_TEST_ASSERT(point[2] == targetZ, "Wrong result of z value for warp vector");
    }
  }
  VTKM_TEST_ASSERT(filter.GetVectorFieldName() == "vec2", "Vector field name is wrong");
}

void TestWarpVectorFilter()
{
  std::cout << "Testing WarpVector filter" << std::endl;
  vtkm::cont::DataSet ds = MakeWarpVectorTestDataSet<vtkm::FloatDefault>();
  vtkm::FloatDefault scale = 2;

  {
    std::cout << "   First field as coordinates" << std::endl;
    vtkm::filter::WarpVector filter(scale);
    filter.SetUseCoordinateSystemAsPrimaryField(true);
    filter.SetVectorField("vec2");
    vtkm::cont::DataSet result = filter.Execute(ds, PolicyWarpVector());
    CheckResult(filter, result);
  }

  {
    std::cout << "   First field as a vector" << std::endl;
    vtkm::filter::WarpVector filter(scale);
    filter.SetPrimaryField("vec1");
    filter.SetVectorField("vec2");
    vtkm::cont::DataSet result = filter.Execute(ds, PolicyWarpVector());
    CheckResult(filter, result);
  }
}
}

int UnitTestWarpVectorFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestWarpVectorFilter);
}
