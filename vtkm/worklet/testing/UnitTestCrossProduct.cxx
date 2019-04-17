//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/CrossProduct.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <random>
#include <vtkm/cont/testing/Testing.h>

namespace
{
std::mt19937 randGenerator;

template <typename T>
void createVectors(std::vector<vtkm::Vec<T, 3>>& vecs1, std::vector<vtkm::Vec<T, 3>>& vecs2)
{
  // First, test the standard directions.
  // X x Y
  vecs1.push_back(vtkm::make_Vec(1, 0, 0));
  vecs2.push_back(vtkm::make_Vec(0, 1, 0));

  // Y x Z
  vecs1.push_back(vtkm::make_Vec(0, 1, 0));
  vecs2.push_back(vtkm::make_Vec(0, 0, 1));

  // Z x X
  vecs1.push_back(vtkm::make_Vec(0, 0, 1));
  vecs2.push_back(vtkm::make_Vec(1, 0, 0));

  // Y x X
  vecs1.push_back(vtkm::make_Vec(0, 1, 0));
  vecs2.push_back(vtkm::make_Vec(1, 0, 0));

  // Z x Y
  vecs1.push_back(vtkm::make_Vec(0, 0, 1));
  vecs2.push_back(vtkm::make_Vec(0, 1, 0));

  // X x Z
  vecs1.push_back(vtkm::make_Vec(1, 0, 0));
  vecs2.push_back(vtkm::make_Vec(0, 0, 1));

  //Test some other vector combinations
  std::uniform_real_distribution<vtkm::Float64> randomDist(-10.0, 10.0);
  randomDist(randGenerator);

  for (int i = 0; i < 100; i++)
  {
    vecs1.push_back(vtkm::make_Vec(
      randomDist(randGenerator), randomDist(randGenerator), randomDist(randGenerator)));
    vecs2.push_back(vtkm::make_Vec(
      randomDist(randGenerator), randomDist(randGenerator), randomDist(randGenerator)));
  }
}

template <typename T>
void TestCrossProduct()
{
  std::vector<vtkm::Vec<T, 3>> inputVecs1, inputVecs2;
  createVectors(inputVecs1, inputVecs2);

  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> inputArray1, inputArray2;
  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> outputArray;
  inputArray1 = vtkm::cont::make_ArrayHandle(inputVecs1);
  inputArray2 = vtkm::cont::make_ArrayHandle(inputVecs2);

  vtkm::worklet::CrossProduct crossProductWorklet;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::CrossProduct> dispatcherCrossProduct(
    crossProductWorklet);
  dispatcherCrossProduct.Invoke(inputArray1, inputArray2, outputArray);

  VTKM_TEST_ASSERT(outputArray.GetNumberOfValues() == inputArray1.GetNumberOfValues(),
                   "Wrong number of results for CrossProduct worklet");

  //Test the canonical cases.
  VTKM_TEST_ASSERT(
    test_equal(outputArray.GetPortalConstControl().Get(0), vtkm::make_Vec(0, 0, 1)) &&
      test_equal(outputArray.GetPortalConstControl().Get(1), vtkm::make_Vec(1, 0, 0)) &&
      test_equal(outputArray.GetPortalConstControl().Get(2), vtkm::make_Vec(0, 1, 0)) &&
      test_equal(outputArray.GetPortalConstControl().Get(3), vtkm::make_Vec(0, 0, -1)) &&
      test_equal(outputArray.GetPortalConstControl().Get(4), vtkm::make_Vec(-1, 0, 0)) &&
      test_equal(outputArray.GetPortalConstControl().Get(5), vtkm::make_Vec(0, -1, 0)),
    "Wrong result for CrossProduct worklet");

  for (vtkm::Id i = 0; i < inputArray1.GetNumberOfValues(); i++)
  {
    vtkm::Vec<T, 3> v1 = inputArray1.GetPortalConstControl().Get(i);
    vtkm::Vec<T, 3> v2 = inputArray2.GetPortalConstControl().Get(i);
    vtkm::Vec<T, 3> res = outputArray.GetPortalConstControl().Get(i);

    //Make sure result is orthogonal each input vector. Need to normalize to compare with zero.
    vtkm::Vec<T, 3> v1N(vtkm::Normal(v1)), v2N(vtkm::Normal(v1)), resN(vtkm::Normal(res));
    VTKM_TEST_ASSERT(test_equal(vtkm::Dot(resN, v1N), T(0.0)), "Wrong result for cross product");
    VTKM_TEST_ASSERT(test_equal(vtkm::Dot(resN, v2N), T(0.0)), "Wrong result for cross product");

    T sinAngle = vtkm::Magnitude(res) * vtkm::RMagnitude(v1) * vtkm::RMagnitude(v2);
    T cosAngle = vtkm::Dot(v1, v2) * vtkm::RMagnitude(v1) * vtkm::RMagnitude(v2);
    VTKM_TEST_ASSERT(test_equal(sinAngle * sinAngle + cosAngle * cosAngle, T(1.0)),
                     "Bad cross product length.");
  }
}

void TestCrossProductWorklets()
{
  std::cout << "Testing CrossProduct Worklet" << std::endl;
  TestCrossProduct<vtkm::Float32>();
  TestCrossProduct<vtkm::Float64>();
}
}

int UnitTestCrossProduct(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCrossProductWorklets, argc, argv);
}
