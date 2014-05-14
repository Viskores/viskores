//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

// Make sure ArrayHandleCompositeVector does not rely on default container or
// device adapter.
#define VTKM_ARRAY_CONTAINER_CONTROL VTKM_ARRAY_CONTAINER_CONTROL_ERROR
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR

#include <vtkm/cont/ArrayHandleCompositeVector.h>

#include <vtkm/VectorTraits.h>

#include <vtkm/cont/ArrayContainerControlBasic.h>
#include <vtkm/cont/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace {

const vtkm::Id ARRAY_SIZE = 10;

typedef vtkm::cont::ArrayContainerControlTagBasic Container;

vtkm::Scalar TestValue(vtkm::Id index, int inComponentIndex, int inArrayId)
{
  return index + vtkm::Scalar(0.1)*inComponentIndex + vtkm::Scalar(0.01)*inArrayId;
}

template<typename ValueType>
vtkm::cont::ArrayHandle<ValueType, Container>
MakeInputArray(int arrayId)
{
  typedef vtkm::VectorTraits<ValueType> VTraits;

  // Create a buffer with valid test values.
  ValueType buffer[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    for (int componentIndex = 0;
         componentIndex < VTraits::NUM_COMPONENTS;
         componentIndex++)
    {
      VTraits::SetComponent(buffer[index],
                            componentIndex,
                            TestValue(index, componentIndex, arrayId));
    }
  }

  // Make an array handle that points to this buffer.
  typedef vtkm::cont::ArrayHandle<ValueType, Container> ArrayHandleType;
  ArrayHandleType bufferHandle =
      vtkm::cont::make_ArrayHandle(buffer, ARRAY_SIZE, Container());

  // When this function returns, the array is going to go out of scope, which
  // will invalidate the array handle we just created. So copy to a new buffer
  // that will stick around after we return.
  ArrayHandleType copyHandle;
  vtkm::cont::DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagSerial>::Copy(
        bufferHandle, copyHandle);

  return copyHandle;
}

template<typename ValueType, typename C>
void CheckArray(const vtkm::cont::ArrayHandle<ValueType,C> &outArray,
                const int *inComponents,
                const int *inArrayIds)
{
  // ArrayHandleCompositeVector currently does not implement the ability to
  // get to values on the control side, so copy to an array that is accessible.
  typedef vtkm::cont::ArrayHandle<ValueType, Container> ArrayHandleType;
  ArrayHandleType arrayCopy;
  vtkm::cont::DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagSerial>::Copy(
        outArray, arrayCopy);

  typename ArrayHandleType::PortalConstControl portal =
      arrayCopy.GetPortalConstControl();
  typedef vtkm::VectorTraits<ValueType> VTraits;
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    ValueType retreivedValue = portal.Get(index);
    for (int componentIndex = 0;
         componentIndex < VTraits::NUM_COMPONENTS;
         componentIndex++)
    {
      vtkm::Scalar retrievedComponent =
          VTraits::GetComponent(retreivedValue, componentIndex);
      vtkm::Scalar expectedComponent = TestValue(index,
                                                 inComponents[componentIndex],
                                                 inArrayIds[componentIndex]);
      VTKM_TEST_ASSERT(retrievedComponent == expectedComponent,
                       "Got bad value.");
    }
  }
}

template<int inComponents>
void TryScalarArray()
{
  std::cout << "Creating a scalar array from one of "
            << inComponents << " components." << std::endl;

  typedef vtkm::Tuple<vtkm::Scalar,inComponents> InValueType;
  typedef vtkm::cont::ArrayHandle<InValueType, Container> InArrayType;
  int inArrayId = 0;
  InArrayType inArray = MakeInputArray<InValueType>(inArrayId);

  typedef vtkm::cont::ArrayHandleCompositeVector<vtkm::Scalar(InArrayType)>
      OutArrayType;
  for (int inComponentIndex = 0;
       inComponentIndex < inComponents;
       inComponentIndex++)
  {
    OutArrayType outArray =
        vtkm::cont::make_ArrayHandleCompositeVector(inArray, inComponentIndex);
    CheckArray(outArray, &inComponentIndex, &inArrayId);
  }
}

template<typename T1, typename T2, typename T3, typename T4>
void TryVector4(vtkm::cont::ArrayHandle<T1,Container> array1,
                vtkm::cont::ArrayHandle<T2,Container> array2,
                vtkm::cont::ArrayHandle<T3,Container> array3,
                vtkm::cont::ArrayHandle<T4,Container> array4)
{
  int arrayIds[4] = {0, 1, 2, 3};
  int inComponents[4];

  for (inComponents[0] = 0;
       inComponents[0] < vtkm::VectorTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    for (inComponents[1] = 0;
         inComponents[1] < vtkm::VectorTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      for (inComponents[2] = 0;
           inComponents[2] < vtkm::VectorTraits<T3>::NUM_COMPONENTS;
           inComponents[2]++)
      {
        for (inComponents[3] = 0;
             inComponents[3] < vtkm::VectorTraits<T4>::NUM_COMPONENTS;
             inComponents[3]++)
        {
          CheckArray(
              vtkm::cont::make_ArrayHandleCompositeVector(
                array1, inComponents[0],
                array2, inComponents[1],
                array3, inComponents[2],
                array4, inComponents[3]),
              inComponents,
              arrayIds);
        }
      }
    }
  }
}

template<typename T1, typename T2, typename T3>
void TryVector3(vtkm::cont::ArrayHandle<T1,Container> array1,
                vtkm::cont::ArrayHandle<T2,Container> array2,
                vtkm::cont::ArrayHandle<T3,Container> array3)
{
  int arrayIds[3] = {0, 1, 2};
  int inComponents[3];

  for (inComponents[0] = 0;
       inComponents[0] < vtkm::VectorTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    for (inComponents[1] = 0;
         inComponents[1] < vtkm::VectorTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      for (inComponents[2] = 0;
           inComponents[2] < vtkm::VectorTraits<T3>::NUM_COMPONENTS;
           inComponents[2]++)
      {
        CheckArray(
            vtkm::cont::make_ArrayHandleCompositeVector(
              array1, inComponents[0],
              array2, inComponents[1],
              array3, inComponents[2]),
            inComponents,
            arrayIds);
      }
    }
  }

  std::cout << "        Fourth component from Scalar." << std::endl;
  TryVector4(array1, array2, array3, MakeInputArray<vtkm::Scalar>(3));
  std::cout << "        Fourth component from Vector4." << std::endl;
  TryVector4(array1, array2, array3, MakeInputArray<vtkm::Vector4>(3));
}

template<typename T1, typename T2>
void TryVector2(vtkm::cont::ArrayHandle<T1,Container> array1,
                vtkm::cont::ArrayHandle<T2,Container> array2)
{
  int arrayIds[2] = {0, 1};
  int inComponents[2];

  for (inComponents[0] = 0;
       inComponents[0] < vtkm::VectorTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    for (inComponents[1] = 0;
         inComponents[1] < vtkm::VectorTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      CheckArray(
          vtkm::cont::make_ArrayHandleCompositeVector(
            array1, inComponents[0],
            array2, inComponents[1]),
          inComponents,
          arrayIds);
    }
  }

  std::cout << "      Third component from Scalar." << std::endl;
  TryVector3(array1, array2, MakeInputArray<vtkm::Scalar>(2));
  std::cout << "      Third component from Vector2." << std::endl;
  TryVector3(array1, array2, MakeInputArray<vtkm::Vector2>(2));
}

template<typename T1>
void TryVector1(vtkm::cont::ArrayHandle<T1,Container> array1)
{
  int arrayIds[1] = {0};
  int inComponents[1];

  for (inComponents[0] = 0;
       inComponents[0] < vtkm::VectorTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    CheckArray(
          vtkm::cont::make_ArrayHandleCompositeVector(array1, inComponents[0]),
          inComponents,
          arrayIds);
  }

  std::cout << "    Second component from Scalar." << std::endl;
  TryVector2(array1, MakeInputArray<vtkm::Scalar>(1));
  std::cout << "    Second component from Vector4." << std::endl;
  TryVector2(array1, MakeInputArray<vtkm::Vector4>(1));
}

void TryVector()
{
  std::cout << "Trying many permutations of composite vectors." << std::endl;

  std::cout << "  First component from Scalar." << std::endl;
  TryVector1(MakeInputArray<vtkm::Scalar>(0));
  std::cout << "  First component from Vector3." << std::endl;
  TryVector1(MakeInputArray<vtkm::Vector3>(0));
}

void TestBadArrayLengths() {
  std::cout << "Checking behavior when size of input arrays do not agree."
            << std::endl;

  typedef vtkm::cont::ArrayHandle<vtkm::Id, Container> InArrayType;
  InArrayType longInArray = MakeInputArray<vtkm::Id>(0);
  InArrayType shortInArray = MakeInputArray<vtkm::Id>(1);
  shortInArray.Shrink(ARRAY_SIZE/2);

  try
  {
    vtkm::cont::make_ArrayHandleCompositeVector(longInArray,0, shortInArray,0);
    VTKM_TEST_FAIL("Did not get exception like expected.");
  }
  catch (vtkm::cont::ErrorControlBadValue error)
  {
    std::cout << "Got expected error: " << std::endl
              << error.GetMessage() << std::endl;
  }
}

void TestCompositeVector() {
  TryScalarArray<2>();
  TryScalarArray<3>();
  TryScalarArray<4>();

  TryVector();

  TestBadArrayLengths();
}

} // anonymous namespace

int UnitTestArrayHandleCompositeVector(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestCompositeVector);
}
