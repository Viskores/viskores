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
#include <vtkm/worklet/Normalize.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

template <typename T>
void createVectors(std::vector<vtkm::Vec<T, 3>>& vecs)
{
  vecs.push_back(vtkm::make_Vec(2, 0, 0));
  vecs.push_back(vtkm::make_Vec(0, 2, 0));
  vecs.push_back(vtkm::make_Vec(0, 0, 2));
  vecs.push_back(vtkm::make_Vec(1, 1, 1));
  vecs.push_back(vtkm::make_Vec(2, 2, 2));
  vecs.push_back(vtkm::make_Vec(2, 1, 1));

  vecs.push_back(vtkm::make_Vec(1000000, 0, 0));

  vecs.push_back(vtkm::make_Vec(static_cast<T>(.1), static_cast<T>(0), static_cast<T>(0)));
  vecs.push_back(vtkm::make_Vec(static_cast<T>(.001), static_cast<T>(0), static_cast<T>(0)));
}

template <typename T>
void createVectors(std::vector<vtkm::Vec<T, 2>>& vecs)
{
  vecs.push_back(vtkm::make_Vec(1, 0));
  vecs.push_back(vtkm::make_Vec(0, 1));
  vecs.push_back(vtkm::make_Vec(1, 1));
  vecs.push_back(vtkm::make_Vec(2, 0));
  vecs.push_back(vtkm::make_Vec(0, 2));
  vecs.push_back(vtkm::make_Vec(2, 2));

  vecs.push_back(vtkm::make_Vec(1000000, 0));

  vecs.push_back(vtkm::make_Vec(static_cast<T>(.1), static_cast<T>(0)));
  vecs.push_back(vtkm::make_Vec(static_cast<T>(.001), static_cast<T>(0)));
}

template <typename T, int N>
void TestNormal()
{
  std::vector<vtkm::Vec<T, N>> inputVecs;
  createVectors(inputVecs);

  vtkm::cont::ArrayHandle<vtkm::Vec<T, N>> inputArray;
  vtkm::cont::ArrayHandle<vtkm::Vec<T, N>> outputArray;
  inputArray = vtkm::cont::make_ArrayHandle(inputVecs);

  vtkm::worklet::Normal normalWorklet;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::Normal> dispatcherNormal(normalWorklet);
  dispatcherNormal.Invoke(inputArray, outputArray);

  //Validate results.

  //Make sure the number of values match.
  VTKM_TEST_ASSERT(outputArray.GetNumberOfValues() == inputArray.GetNumberOfValues(),
                   "Wrong number of results for Normalize worklet");

  //Make sure each vector is correct.
  for (vtkm::Id i = 0; i < inputArray.GetNumberOfValues(); i++)
  {
    //Make sure that the value is correct.
    vtkm::Vec<T, N> v = inputArray.GetPortalConstControl().Get(i);
    vtkm::Vec<T, N> vN = outputArray.GetPortalConstControl().Get(i);
    T len = vtkm::Magnitude(v);
    VTKM_TEST_ASSERT(test_equal(v / len, vN), "Wrong result for Normalize worklet");

    //Make sure the magnitudes are all 1.0
    len = vtkm::Magnitude(vN);
    VTKM_TEST_ASSERT(test_equal(len, 1), "Wrong magnitude for Normalize worklet");
  }
}

template <typename T, int N>
void TestNormalize()
{
  std::vector<vtkm::Vec<T, N>> inputVecs;
  createVectors(inputVecs);

  vtkm::cont::ArrayHandle<vtkm::Vec<T, N>> inputArray;
  vtkm::cont::ArrayHandle<vtkm::Vec<T, N>> outputArray;
  inputArray = vtkm::cont::make_ArrayHandle(inputVecs);

  vtkm::worklet::Normalize normalizeWorklet;
  vtkm::worklet::DispatcherMapField<vtkm::worklet::Normalize> dispatcherNormalize(normalizeWorklet);
  dispatcherNormalize.Invoke(inputArray);

  //Make sure each vector is correct.
  for (vtkm::Id i = 0; i < inputArray.GetNumberOfValues(); i++)
  {
    //Make sure that the value is correct.
    vtkm::Vec<T, N> v = inputVecs[static_cast<std::size_t>(i)];
    vtkm::Vec<T, N> vN = inputArray.GetPortalConstControl().Get(i);
    T len = vtkm::Magnitude(v);
    VTKM_TEST_ASSERT(test_equal(v / len, vN), "Wrong result for Normalize worklet");

    //Make sure the magnitudes are all 1.0
    len = vtkm::Magnitude(vN);
    VTKM_TEST_ASSERT(test_equal(len, 1), "Wrong magnitude for Normalize worklet");
  }
}

void TestNormalWorklets()
{
  std::cout << "Testing Normal Worklet" << std::endl;

  TestNormal<vtkm::Float32, 2>();
  TestNormal<vtkm::Float64, 2>();
  TestNormal<vtkm::Float32, 3>();
  TestNormal<vtkm::Float64, 3>();

  std::cout << "Testing Normalize Worklet" << std::endl;
  TestNormalize<vtkm::Float32, 2>();
  TestNormalize<vtkm::Float64, 2>();
  TestNormalize<vtkm::Float32, 3>();
  TestNormalize<vtkm::Float64, 3>();
}
}

int UnitTestNormalize(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestNormalWorklets, argc, argv);
}
