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
#include <vtkm/filter/DotProduct.h>

#include <random>
#include <vector>

namespace
{
std::mt19937 randGenerator;

template <typename T>
void createVectors(std::size_t numPts,
                   int vecType,
                   std::vector<vtkm::Vec<T, 3>>& vecs1,
                   std::vector<vtkm::Vec<T, 3>>& vecs2)
{
  if (vecType == 0) // X x Y
  {
    vecs1.resize(numPts, vtkm::make_Vec(1, 0, 0));
    vecs2.resize(numPts, vtkm::make_Vec(0, 1, 0));
  }
  else if (vecType == 1) // Y x Z
  {
    vecs1.resize(numPts, vtkm::make_Vec(0, 1, 0));
    vecs2.resize(numPts, vtkm::make_Vec(0, 0, 1));
  }
  else if (vecType == 2) // Z x X
  {
    vecs1.resize(numPts, vtkm::make_Vec(0, 0, 1));
    vecs2.resize(numPts, vtkm::make_Vec(1, 0, 0));
  }
  else if (vecType == 3) // Y x X
  {
    vecs1.resize(numPts, vtkm::make_Vec(0, 1, 0));
    vecs2.resize(numPts, vtkm::make_Vec(1, 0, 0));
  }
  else if (vecType == 4) // Z x Y
  {
    vecs1.resize(numPts, vtkm::make_Vec(0, 0, 1));
    vecs2.resize(numPts, vtkm::make_Vec(0, 1, 0));
  }
  else if (vecType == 5) // X x Z
  {
    vecs1.resize(numPts, vtkm::make_Vec(1, 0, 0));
    vecs2.resize(numPts, vtkm::make_Vec(0, 0, 1));
  }
  else if (vecType == 6)
  {
    //Test some other vector combinations
    std::uniform_real_distribution<vtkm::Float64> randomDist(-10.0, 10.0);
    randomDist(randGenerator);

    vecs1.resize(numPts);
    vecs2.resize(numPts);
    for (std::size_t i = 0; i < numPts; i++)
    {
      vecs1[i] = vtkm::make_Vec(
        randomDist(randGenerator), randomDist(randGenerator), randomDist(randGenerator));
      vecs2[i] = vtkm::make_Vec(
        randomDist(randGenerator), randomDist(randGenerator), randomDist(randGenerator));
    }
  }
}

void CheckResult(const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& field1,
                 const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& field2,
                 const vtkm::cont::DataSet& result)
{
  VTKM_TEST_ASSERT(result.HasField("dotproduct", vtkm::cont::Field::Association::POINTS),
                   "Output field is missing.");

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outputArray;
  result.GetField("dotproduct", vtkm::cont::Field::Association::POINTS)
    .GetData()
    .CopyTo(outputArray);

  auto v1Portal = field1.GetPortalConstControl();
  auto v2Portal = field2.GetPortalConstControl();
  auto outPortal = outputArray.GetPortalConstControl();

  VTKM_TEST_ASSERT(outputArray.GetNumberOfValues() == field1.GetNumberOfValues(),
                   "Field sizes wrong");
  VTKM_TEST_ASSERT(outputArray.GetNumberOfValues() == field2.GetNumberOfValues(),
                   "Field sizes wrong");

  for (vtkm::Id j = 0; j < outputArray.GetNumberOfValues(); j++)
  {
    vtkm::Vec<vtkm::FloatDefault, 3> v1 = v1Portal.Get(j);
    vtkm::Vec<vtkm::FloatDefault, 3> v2 = v2Portal.Get(j);
    vtkm::FloatDefault res = outPortal.Get(j);

    VTKM_TEST_ASSERT(test_equal(vtkm::Dot(v1, v2), res), "Wrong result for dot product");
  }
}

void TestDotProduct()
{
  std::cout << "Testing DotProduct Filter" << std::endl;

  vtkm::cont::testing::MakeTestDataSet testDataSet;

  const int numCases = 7;
  for (int i = 0; i < numCases; i++)
  {
    std::cout << "Case " << i << std::endl;

    vtkm::cont::DataSet dataSet = testDataSet.Make3DUniformDataSet0();
    vtkm::Id nVerts = dataSet.GetCoordinateSystem(0).GetData().GetNumberOfValues();

    std::vector<vtkm::Vec<vtkm::FloatDefault, 3>> vecs1, vecs2;
    createVectors(static_cast<std::size_t>(nVerts), i, vecs1, vecs2);

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> field1, field2;
    field1 = vtkm::cont::make_ArrayHandle(vecs1);
    field2 = vtkm::cont::make_ArrayHandle(vecs2);

    vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "vec1", field1);
    vtkm::cont::DataSetFieldAdd::AddPointField(dataSet, "vec2", field2);
    dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("vecA", field1));
    dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("vecB", field2));

    {
      std::cout << "  Both vectors as normal fields" << std::endl;
      vtkm::filter::DotProduct filter;
      filter.SetPrimaryField("vec1");
      filter.SetSecondaryField("vec2");
      vtkm::cont::DataSet result = filter.Execute(dataSet);
      CheckResult(field1, field2, result);
    }

    {
      std::cout << "  First field as coordinates" << std::endl;
      vtkm::filter::DotProduct filter;
      filter.SetUseCoordinateSystemAsPrimaryField(true);
      filter.SetPrimaryCoordinateSystem(1);
      filter.SetSecondaryField("vec2");
      vtkm::cont::DataSet result = filter.Execute(dataSet);
      CheckResult(field1, field2, result);
    }

    {
      std::cout << "  Second field as coordinates" << std::endl;
      vtkm::filter::DotProduct filter;
      filter.SetPrimaryField("vec1");
      filter.SetUseCoordinateSystemAsSecondaryField(true);
      filter.SetSecondaryCoordinateSystem(2);
      vtkm::cont::DataSet result = filter.Execute(dataSet);
      CheckResult(field1, field2, result);
    }
  }
}
} // anonymous namespace

int UnitTestDotProductFilter(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestDotProduct);
}
