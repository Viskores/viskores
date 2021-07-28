//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/MapFieldMergeAverage.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/Field.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 26;
constexpr vtkm::Id3 ARRAY3_DIM = { 3, 3, 3 };
constexpr vtkm::Id REDUCED_SIZE = 7;

vtkm::worklet::Keys<vtkm::Id> MakeKeys(vtkm::Id originalArraySize)
{
  vtkm::cont::ArrayHandle<vtkm::Id> keyArray;
  keyArray.Allocate(originalArraySize);
  {
    auto portal = keyArray.WritePortal();
    for (vtkm::Id i = 0; i < originalArraySize; ++i)
    {
      portal.Set(i, i % REDUCED_SIZE);
    }
  }

  return vtkm::worklet::Keys<vtkm::Id>(keyArray);
}

// Make an array of the expected output of mapping the given array using the keys returned from
// MakeKeys but with a different mechanism.
template <typename T, typename S>
vtkm::cont::ArrayHandle<T> MakeExpectedOutput(const vtkm::cont::ArrayHandle<T, S>& inputArray)
{
  using ComponentType = typename vtkm::VecTraits<T>::ComponentType;

  auto inputPortal = inputArray.ReadPortal();

  vtkm::cont::ArrayHandle<T> outputArray;
  outputArray.Allocate(REDUCED_SIZE);
  auto outputPortal = outputArray.WritePortal();

  for (vtkm::Id reducedI = 0; reducedI < REDUCED_SIZE; ++reducedI)
  {
    T sum = vtkm::TypeTraits<T>::ZeroInitialization();
    ComponentType num = 0;
    for (vtkm::Id fullI = reducedI; fullI < inputArray.GetNumberOfValues(); fullI += REDUCED_SIZE)
    {
      sum = static_cast<T>(sum + inputPortal.Get(fullI));
      num = static_cast<ComponentType>(num + ComponentType(1));
    }
    outputPortal.Set(reducedI, sum / T(num));
  }

  return outputArray;
}

template <typename T, typename S>
void TryArray(const vtkm::cont::ArrayHandle<T, S>& inputArray)
{
  std::cout << "Input" << std::endl;
  vtkm::cont::printSummary_ArrayHandle(inputArray, std::cout);

  vtkm::cont::Field::Association association =
    ((sizeof(T) < 8) ? vtkm::cont::Field::Association::POINTS
                     : vtkm::cont::Field::Association::CELL_SET);

  vtkm::cont::Field inputField("my-array", association, inputArray);

  vtkm::worklet::Keys<vtkm::Id> keys = MakeKeys(inputArray.GetNumberOfValues());

  vtkm::cont::ArrayHandle<T> expectedOutputArray = MakeExpectedOutput(inputArray);
  std::cout << "Expected output" << std::endl;
  vtkm::cont::printSummary_ArrayHandle(expectedOutputArray, std::cout);

  vtkm::cont::Field outputField;
  bool result = vtkm::filter::MapFieldMergeAverage(inputField, keys, outputField);
  VTKM_TEST_ASSERT(result, "Could not map the array.");

  VTKM_TEST_ASSERT(outputField.GetAssociation() == association);
  VTKM_TEST_ASSERT(outputField.GetName() == "my-array");

  vtkm::cont::ArrayHandle<T> outputArray;
  outputField.GetData().AsArrayHandle(outputArray);
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

int UnitTestMapFieldMergeAverage(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
