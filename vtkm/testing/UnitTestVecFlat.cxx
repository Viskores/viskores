//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/VecFlat.h>

#include <vtkm/VecAxisAlignedPointCoordinates.h>

#include <vtkm/cont/Logging.h>

#include <vtkm/testing/Testing.h>

namespace
{

template <typename T>
void CheckTraits(const T&, vtkm::IdComponent numComponents)
{
  VTKM_TEST_ASSERT((std::is_same<typename vtkm::TypeTraits<T>::DimensionalityTag,
                                 vtkm::TypeTraitsVectorTag>::value));
  VTKM_TEST_ASSERT(vtkm::VecTraits<T>::NUM_COMPONENTS == numComponents);
}

void TryBasicVec()
{
  using NestedVecType = vtkm::Vec<vtkm::Vec<vtkm::Id, 2>, 3>;
  std::cout << "Trying " << vtkm::cont::TypeToString<NestedVecType>() << std::endl;

  NestedVecType nestedVec = { { 0, 1 }, { 2, 3 }, { 4, 5 } };
  std::cout << "  original: " << nestedVec << std::endl;

  auto flatVec = vtkm::make_VecFlat(nestedVec);
  std::cout << "  flat: " << flatVec << std::endl;
  CheckTraits(flatVec, 6);
  VTKM_TEST_ASSERT(decltype(flatVec)::NUM_COMPONENTS == 6);
  VTKM_TEST_ASSERT(flatVec[0] == 0);
  VTKM_TEST_ASSERT(flatVec[1] == 1);
  VTKM_TEST_ASSERT(flatVec[2] == 2);
  VTKM_TEST_ASSERT(flatVec[3] == 3);
  VTKM_TEST_ASSERT(flatVec[4] == 4);
  VTKM_TEST_ASSERT(flatVec[5] == 5);

  flatVec = vtkm::VecFlat<NestedVecType>{ 5, 4, 3, 2, 1, 0 };
  std::cout << "  flat backward: " << flatVec << std::endl;
  VTKM_TEST_ASSERT(flatVec[0] == 5);
  VTKM_TEST_ASSERT(flatVec[1] == 4);
  VTKM_TEST_ASSERT(flatVec[2] == 3);
  VTKM_TEST_ASSERT(flatVec[3] == 2);
  VTKM_TEST_ASSERT(flatVec[4] == 1);
  VTKM_TEST_ASSERT(flatVec[5] == 0);

  nestedVec = flatVec;
  std::cout << "  nested backward: " << nestedVec << std::endl;
  VTKM_TEST_ASSERT(nestedVec[0][0] == 5);
  VTKM_TEST_ASSERT(nestedVec[0][1] == 4);
  VTKM_TEST_ASSERT(nestedVec[1][0] == 3);
  VTKM_TEST_ASSERT(nestedVec[1][1] == 2);
  VTKM_TEST_ASSERT(nestedVec[2][0] == 1);
  VTKM_TEST_ASSERT(nestedVec[2][1] == 0);
}

void TryScalar()
{
  using ScalarType = vtkm::Id;
  std::cout << "Trying " << vtkm::cont::TypeToString<ScalarType>() << std::endl;

  ScalarType scalar = TestValue(0, ScalarType{});
  std::cout << "  original: " << scalar << std::endl;

  auto flatVec = vtkm::make_VecFlat(scalar);
  std::cout << "  flat: " << flatVec << std::endl;
  CheckTraits(flatVec, 1);
  VTKM_TEST_ASSERT(decltype(flatVec)::NUM_COMPONENTS == 1);
  VTKM_TEST_ASSERT(test_equal(flatVec[0], TestValue(0, ScalarType{})));
}

void TrySpecialVec()
{
  using NestedVecType = vtkm::Vec<vtkm::VecAxisAlignedPointCoordinates<1>, 2>;
  std::cout << "Trying " << vtkm::cont::TypeToString<NestedVecType>() << std::endl;

  NestedVecType nestedVec = { { { 0, 0, 0 }, { 1, 1, 1 } }, { { 1, 1, 1 }, { 1, 1, 1 } } };
  std::cout << "  original: " << nestedVec << std::endl;

  auto flatVec = vtkm::make_VecFlat(nestedVec);
  std::cout << "  flat: " << flatVec << std::endl;
  CheckTraits(flatVec, 12);
  VTKM_TEST_ASSERT(decltype(flatVec)::NUM_COMPONENTS == 12);
  VTKM_TEST_ASSERT(test_equal(flatVec[0], nestedVec[0][0][0]));
  VTKM_TEST_ASSERT(test_equal(flatVec[1], nestedVec[0][0][1]));
  VTKM_TEST_ASSERT(test_equal(flatVec[2], nestedVec[0][0][2]));
  VTKM_TEST_ASSERT(test_equal(flatVec[3], nestedVec[0][1][0]));
  VTKM_TEST_ASSERT(test_equal(flatVec[4], nestedVec[0][1][1]));
  VTKM_TEST_ASSERT(test_equal(flatVec[5], nestedVec[0][1][2]));
  VTKM_TEST_ASSERT(test_equal(flatVec[6], nestedVec[1][0][0]));
  VTKM_TEST_ASSERT(test_equal(flatVec[7], nestedVec[1][0][1]));
  VTKM_TEST_ASSERT(test_equal(flatVec[8], nestedVec[1][0][2]));
  VTKM_TEST_ASSERT(test_equal(flatVec[9], nestedVec[1][1][0]));
  VTKM_TEST_ASSERT(test_equal(flatVec[10], nestedVec[1][1][1]));
  VTKM_TEST_ASSERT(test_equal(flatVec[11], nestedVec[1][1][2]));
}

void DoTest()
{
  TryBasicVec();
  TryScalar();
  TrySpecialVec();
}

} // anonymous namespace

int UnitTestVecFlat(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(DoTest, argc, argv);
}
