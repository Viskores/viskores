//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/MapFieldPermutation.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/Field.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 26;
constexpr vtkm::Id3 ARRAY3_DIM = { 3, 3, 3 };

template <typename T, typename S>
void TryArray(const vtkm::cont::ArrayHandle<T, S>& inputArray)
{
  std::cout << "Input" << std::endl;
  vtkm::cont::printSummary_ArrayHandle(inputArray, std::cout);

  vtkm::cont::Field::Association association =
    ((sizeof(T) < 8) ? vtkm::cont::Field::Association::POINTS
                     : vtkm::cont::Field::Association::CELL_SET);

  vtkm::cont::Field inputField("my-array", association, inputArray);

  vtkm::cont::ArrayHandle<vtkm::Id> permutationArray;
  vtkm::cont::ArrayCopy(
    vtkm::cont::make_ArrayHandleCounting<vtkm::Id>(0, 2, inputArray.GetNumberOfValues() / 2),
    permutationArray);

  vtkm::cont::ArrayHandle<T> expectedOutputArray;
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandlePermutation(permutationArray, inputArray),
                        expectedOutputArray);
  std::cout << "Expected output" << std::endl;
  vtkm::cont::printSummary_ArrayHandle(expectedOutputArray, std::cout);

  vtkm::cont::Field outputField;
  bool result = vtkm::filter::MapFieldPermutation(inputField, permutationArray, outputField);
  VTKM_TEST_ASSERT(result, "Could not permute the array.");

  VTKM_TEST_ASSERT(outputField.GetAssociation() == association);
  VTKM_TEST_ASSERT(outputField.GetName() == "my-array");

  vtkm::cont::ArrayHandle<T> outputArray;
  outputField.GetData().CopyTo(outputArray);
  std::cout << "Actual output" << std::endl;
  vtkm::cont::printSummary_ArrayHandle(outputArray, std::cout);

  VTKM_TEST_ASSERT(test_equal_portals(expectedOutputArray.ReadPortal(), outputArray.ReadPortal()));
}

template <typename T>
void TryType(T)
{
  vtkm::cont::ArrayHandle<T> inputArray;
  inputArray.Allocate(ARRAY_SIZE);
  SetPortal(inputArray.WritePortal());
  TryArray(inputArray);
}

struct TryTypeFunctor
{
  template <typename T>
  void operator()(T x) const
  {
    TryType(x);
  }
};

void TryCartesianProduct()
{
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> axes[3];
  for (vtkm::IdComponent i = 0; i < 3; ++i)
  {
    axes[i].Allocate(ARRAY3_DIM[i]);
    SetPortal(axes[i].WritePortal());
  }

  TryArray(vtkm::cont::make_ArrayHandleCartesianProduct(axes[0], axes[1], axes[2]));
}

void DoTest()
{
  std::cout << "**** Test Basic Arrays *****" << std::endl;
  vtkm::testing::Testing::TryTypes(TryTypeFunctor{});

  std::cout << std::endl << "**** Test Uniform Point Coordiantes *****" << std::endl;
  TryArray(vtkm::cont::ArrayHandleUniformPointCoordinates(ARRAY3_DIM));

  std::cout << std::endl << "**** Test Cartesian Product *****" << std::endl;
  TryCartesianProduct();
}

} // anonymous namespace

int UnitTestMapFieldPermutation(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
