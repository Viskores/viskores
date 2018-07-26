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
#include <vtkm/filter/WarpScalar.h>

#include <random>
#include <vector>

namespace
{
const vtkm::Id dim = 5;
template <typename T>
vtkm::cont::DataSet MakeWarpScalarTestDataSet()
{
  using vecType = vtkm::Vec<T, 3>;
  vtkm::cont::DataSet dataSet;

  std::vector<vecType> coordinates;
  std::vector<vecType> vec1;
  std::vector<T> scalarFactor;
  for (vtkm::Id j = 0; j < dim; ++j)
  {
    T z = static_cast<T>(j) / static_cast<T>(dim - 1);
    for (vtkm::Id i = 0; i < dim; ++i)
    {
      T x = static_cast<T>(i) / static_cast<T>(dim - 1);
      T y = (x * x + z * z) / static_cast<T>(2.0);
      coordinates.push_back(vtkm::make_Vec(x, y, z));
      vec1.push_back(vtkm::make_Vec(x, y, y));
      scalarFactor.push_back(static_cast<T>(j * dim + i));
    }
  }

  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, vtkm::CopyFlag::On));

  vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "vec1", vec1);
  vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "scalarfactor", scalarFactor);

  vecType normal = vtkm::make_Vec<T>(static_cast<T>(0.0), static_cast<T>(0.0), static_cast<T>(1.0));
  vtkm::cont::ArrayHandleConstant<vecType> vectorAH =
    vtkm::cont::make_ArrayHandleConstant(normal, dim * dim);
  vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "normal", vectorAH);

  return dataSet;
}

class PolicyWarpScalar : public vtkm::filter::PolicyBase<PolicyWarpScalar>
{
public:
  using vecType = vtkm::Vec<vtkm::FloatDefault, 3>;
  struct TypeListTagWarpScalarTags
    : vtkm::ListTagBase<vtkm::cont::ArrayHandleConstant<vecType>::StorageTag,
                        vtkm::cont::ArrayHandle<vecType>::StorageTag,
                        vtkm::cont::ArrayHandle<vtkm::FloatDefault>::StorageTag>
  {
  };
  using FieldStorageList = TypeListTagWarpScalarTags;
};

void CheckResult(const vtkm::filter::WarpScalar& filter, const vtkm::cont::DataSet& result)
{
  VTKM_TEST_ASSERT(result.HasField("warpscalar", vtkm::cont::Field::Association::POINTS),
                   "Output filed warpscalar is missing");
  using vecType = vtkm::Vec<vtkm::FloatDefault, 3>;
  vtkm::cont::ArrayHandle<vecType> outputArray;
  result.GetField("warpscalar", vtkm::cont::Field::Association::POINTS)
    .GetData()
    .CopyTo(outputArray);
  auto outPortal = outputArray.GetPortalConstControl();

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> sfArray;
  result.GetField("scalarfactor", vtkm::cont::Field::Association::POINTS).GetData().CopyTo(sfArray);
  auto sfPortal = sfArray.GetPortalConstControl();

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
        ? z + static_cast<vtkm::FloatDefault>(2 * sfPortal.Get(j * dim + i))
        : y + static_cast<vtkm::FloatDefault>(2 * sfPortal.Get(j * dim + i));
      auto point = outPortal.Get(j * dim + i);
      VTKM_TEST_ASSERT(point[0] == x, "Wrong result of x value for warp scalar");
      VTKM_TEST_ASSERT(point[1] == y, "Wrong result of y value for warp scalar");
      VTKM_TEST_ASSERT(point[2] == targetZ, "Wrong result of z value for warp scalar");
    }
  }
}

void TestWarpScalarFilter()
{
  std::cout << "Testing WarpScalar filter" << std::endl;
  vtkm::cont::DataSet ds = MakeWarpScalarTestDataSet<vtkm::FloatDefault>();
  vtkm::FloatDefault scale = 2;

  {
    std::cout << "   First field as coordinates" << std::endl;
    vtkm::filter::WarpScalar filter(scale);
    filter.SetUseCoordinateSystemAsPrimaryField(true);
    filter.SetNormalField("normal");
    filter.SetScalarFactorField("scalarfactor");
    vtkm::cont::DataSet result = filter.Execute(ds, PolicyWarpScalar());
    CheckResult(filter, result);
  }

  {
    std::cout << "   First field as a vector" << std::endl;
    vtkm::filter::WarpScalar filter(scale);
    filter.SetPrimaryField("vec1");
    filter.SetNormalField("normal");
    filter.SetScalarFactorField("scalarfactor");
    vtkm::cont::DataSet result = filter.Execute(ds, PolicyWarpScalar());
    CheckResult(filter, result);
  }
}
}

int UnitTestWarpScalarFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestWarpScalarFilter);
}
