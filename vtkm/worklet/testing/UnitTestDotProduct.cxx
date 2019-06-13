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
#include <vtkm/worklet/DotProduct.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template <typename T>
T normalizedVector(T v)
{
  T vN = vtkm::Normal(v);
  return vN;
}

template <typename T>
void createVectors(std::vector<vtkm::Vec<T, 3>>& vecs1,
                   std::vector<vtkm::Vec<T, 3>>& vecs2,
                   std::vector<T>& result)
{
  vecs1.push_back(normalizedVector(vtkm::make_Vec(T(1), T(0), T(0))));
  vecs2.push_back(normalizedVector(vtkm::make_Vec(T(1), T(0), T(0))));
  result.push_back(1);

  vecs1.push_back(normalizedVector(vtkm::make_Vec(T(1), T(0), T(0))));
  vecs2.push_back(normalizedVector(vtkm::make_Vec(T(-1), T(0), T(0))));
  result.push_back(-1);

  vecs1.push_back(normalizedVector(vtkm::make_Vec(T(1), T(0), T(0))));
  vecs2.push_back(normalizedVector(vtkm::make_Vec(T(0), T(1), T(0))));
  result.push_back(0);

  vecs1.push_back(normalizedVector(vtkm::make_Vec(T(1), T(0), T(0))));
  vecs2.push_back(normalizedVector(vtkm::make_Vec(T(0), T(-1), T(0))));
  result.push_back(0);

  vecs1.push_back(normalizedVector(vtkm::make_Vec(T(1), T(0), T(0))));
  vecs2.push_back(normalizedVector(vtkm::make_Vec(T(1), T(1), T(0))));
  result.push_back(T(1.0 / vtkm::Sqrt(2.0)));

  vecs1.push_back(normalizedVector(vtkm::make_Vec(T(1), T(1), T(0))));
  vecs2.push_back(normalizedVector(vtkm::make_Vec(T(1), T(0), T(0))));
  result.push_back(T(1.0 / vtkm::Sqrt(2.0)));

  vecs1.push_back(normalizedVector(vtkm::make_Vec(T(-1), T(0), T(0))));
  vecs2.push_back(normalizedVector(vtkm::make_Vec(T(1), T(1), T(0))));
  result.push_back(-T(1.0 / vtkm::Sqrt(2.0)));

  vecs1.push_back(normalizedVector(vtkm::make_Vec(T(0), T(1), T(0))));
  vecs2.push_back(normalizedVector(vtkm::make_Vec(T(1), T(1), T(0))));
  result.push_back(T(1.0 / vtkm::Sqrt(2.0)));
}

template <typename T>
void TestDotProduct()
{
  std::vector<vtkm::Vec<T, 3>> inputVecs1, inputVecs2;
  std::vector<T> answer;
  createVectors(inputVecs1, inputVecs2, answer);

  vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> inputArray1, inputArray2;
  vtkm::cont::ArrayHandle<T> outputArray;
  inputArray1 = vtkm::cont::make_ArrayHandle(inputVecs1);
  inputArray2 = vtkm::cont::make_ArrayHandle(inputVecs2);

  vtkm::worklet::DotProduct dotProductWorklet;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::DotProduct> dispatcherDotProduct(
    dotProductWorklet);
  dispatcherDotProduct.Invoke(inputArray1, inputArray2, outputArray);

  VTKM_TEST_ASSERT(outputArray.GetNumberOfValues() == inputArray1.GetNumberOfValues(),
                   "Wrong number of results for DotProduct worklet");

  for (vtkm::Id i = 0; i < inputArray1.GetNumberOfValues(); i++)
  {
    vtkm::Vec<T, 3> v1 = inputArray1.GetPortalConstControl().Get(i);
    vtkm::Vec<T, 3> v2 = inputArray2.GetPortalConstControl().Get(i);
    T ans = answer[static_cast<std::size_t>(i)];

    VTKM_TEST_ASSERT(test_equal(ans, vtkm::Dot(v1, v2)), "Wrong result for dot product");
  }
}

void TestDotProductWorklets()
{
  std::cout << "Testing DotProduct Worklet" << std::endl;
  TestDotProduct<vtkm::Float32>();
  //  TestDotProduct<vtkm::Float64>();
}
}

int UnitTestDotProduct(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestDotProductWorklets, argc, argv);
}
