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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

// Make sure ArrayHandleCompositeVector does not rely on default storage or
// device adapter.
#define VTKM_STORAGE VTKM_STORAGE_ERROR
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR

#include <vtkm/cont/ArrayHandleCompositeVector.h>

#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/DeviceAdapterSerial.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace {

const vtkm::Id ARRAY_SIZE = 10;

typedef vtkm::cont::StorageTagBasic StorageTag;

vtkm::FloatDefault TestValue3Ids(vtkm::Id index,
                                 vtkm::IdComponent inComponentIndex,
                                 int inArrayId)
{
  return (vtkm::FloatDefault(index) +
          0.1f*vtkm::FloatDefault(inComponentIndex) +
          0.01f*vtkm::FloatDefault(inArrayId));
}

template<typename ValueType>
vtkm::cont::ArrayHandle<ValueType, StorageTag>
MakeInputArray(int arrayId)
{
  typedef vtkm::VecTraits<ValueType> VTraits;

  // Create a buffer with valid test values.
  ValueType buffer[ARRAY_SIZE];
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < VTraits::NUM_COMPONENTS;
         componentIndex++)
    {
      VTraits::SetComponent(buffer[index],
                            componentIndex,
                            TestValue3Ids(index, componentIndex, arrayId));
    }
  }

  // Make an array handle that points to this buffer.
  typedef vtkm::cont::ArrayHandle<ValueType, StorageTag> ArrayHandleType;
  ArrayHandleType bufferHandle =
      vtkm::cont::make_ArrayHandle(buffer, ARRAY_SIZE);

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
                const vtkm::IdComponent *inComponents,
                const int *inArrayIds)
{
  // ArrayHandleCompositeVector currently does not implement the ability to
  // get to values on the control side, so copy to an array that is accessible.
  typedef vtkm::cont::ArrayHandle<ValueType, StorageTag> ArrayHandleType;
  ArrayHandleType arrayCopy;
  vtkm::cont::DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagSerial>::Copy(
        outArray, arrayCopy);

  typename ArrayHandleType::PortalConstControl portal =
      arrayCopy.GetPortalConstControl();
  typedef vtkm::VecTraits<ValueType> VTraits;
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    ValueType retreivedValue = portal.Get(index);
    for (vtkm::IdComponent componentIndex = 0;
         componentIndex < VTraits::NUM_COMPONENTS;
         componentIndex++)
    {
      vtkm::FloatDefault retrievedComponent =
          VTraits::GetComponent(retreivedValue, componentIndex);
      vtkm::FloatDefault expectedComponent = TestValue3Ids(index,
                                                 inComponents[componentIndex],
                                                 inArrayIds[componentIndex]);
      VTKM_TEST_ASSERT(retrievedComponent == expectedComponent,
                       "Got bad value.");
    }
  }
}

template<vtkm::IdComponent inComponents>
void TryScalarArray()
{
  std::cout << "Creating a scalar array from one of "
            << inComponents << " components." << std::endl;

  typedef vtkm::Vec<vtkm::FloatDefault,inComponents> InValueType;
  typedef vtkm::cont::ArrayHandle<InValueType, StorageTag> InArrayType;
  int inArrayId = 0;
  InArrayType inArray = MakeInputArray<InValueType>(inArrayId);

  typedef typename vtkm::cont::ArrayHandleCompositeVectorType<InArrayType>::type
      OutArrayType;
  for (vtkm::IdComponent inComponentIndex = 0;
       inComponentIndex < inComponents;
       inComponentIndex++)
  {
    OutArrayType outArray =
        vtkm::cont::make_ArrayHandleCompositeVector(inArray, inComponentIndex);
    CheckArray(outArray, &inComponentIndex, &inArrayId);
  }
}

template<typename T1, typename T2, typename T3, typename T4>
void TryVector4(vtkm::cont::ArrayHandle<T1,StorageTag> array1,
                vtkm::cont::ArrayHandle<T2,StorageTag> array2,
                vtkm::cont::ArrayHandle<T3,StorageTag> array3,
                vtkm::cont::ArrayHandle<T4,StorageTag> array4)
{
  int arrayIds[4] = {0, 1, 2, 3};
  vtkm::IdComponent inComponents[4];

  for (inComponents[0] = 0;
       inComponents[0] < vtkm::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    for (inComponents[1] = 0;
         inComponents[1] < vtkm::VecTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      for (inComponents[2] = 0;
           inComponents[2] < vtkm::VecTraits<T3>::NUM_COMPONENTS;
           inComponents[2]++)
      {
        for (inComponents[3] = 0;
             inComponents[3] < vtkm::VecTraits<T4>::NUM_COMPONENTS;
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
void TryVector3(vtkm::cont::ArrayHandle<T1,StorageTag> array1,
                vtkm::cont::ArrayHandle<T2,StorageTag> array2,
                vtkm::cont::ArrayHandle<T3,StorageTag> array3)
{
  int arrayIds[3] = {0, 1, 2};
  vtkm::IdComponent inComponents[3];

  for (inComponents[0] = 0;
       inComponents[0] < vtkm::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    for (inComponents[1] = 0;
         inComponents[1] < vtkm::VecTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      for (inComponents[2] = 0;
           inComponents[2] < vtkm::VecTraits<T3>::NUM_COMPONENTS;
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
  TryVector4(array1, array2, array3, MakeInputArray<vtkm::FloatDefault>(3));
  std::cout << "        Fourth component from Vector4." << std::endl;
  TryVector4(array1,
             array2,
             array3,
             MakeInputArray<vtkm::Vec<vtkm::FloatDefault,4> >(3));
}

template<typename T1, typename T2>
void TryVector2(vtkm::cont::ArrayHandle<T1,StorageTag> array1,
                vtkm::cont::ArrayHandle<T2,StorageTag> array2)
{
  int arrayIds[2] = {0, 1};
  vtkm::IdComponent inComponents[2];

  for (inComponents[0] = 0;
       inComponents[0] < vtkm::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    for (inComponents[1] = 0;
         inComponents[1] < vtkm::VecTraits<T2>::NUM_COMPONENTS;
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
  TryVector3(array1, array2, MakeInputArray<vtkm::FloatDefault>(2));
  std::cout << "      Third component from Vector2." << std::endl;
  TryVector3(array1,
             array2,
             MakeInputArray<vtkm::Vec<vtkm::FloatDefault,2> >(2));
}

template<typename T1>
void TryVector1(vtkm::cont::ArrayHandle<T1,StorageTag> array1)
{
  int arrayIds[1] = {0};
  vtkm::IdComponent inComponents[1];

  for (inComponents[0] = 0;
       inComponents[0] < vtkm::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    CheckArray(
          vtkm::cont::make_ArrayHandleCompositeVector(array1, inComponents[0]),
          inComponents,
          arrayIds);
  }

  std::cout << "    Second component from Scalar." << std::endl;
  TryVector2(array1, MakeInputArray<vtkm::FloatDefault>(1));
  std::cout << "    Second component from Vector4." << std::endl;
  TryVector2(array1, MakeInputArray<vtkm::Vec<vtkm::FloatDefault,4> >(1));
}

void TryVector()
{
  std::cout << "Trying many permutations of composite vectors." << std::endl;

  std::cout << "  First component from Scalar." << std::endl;
  TryVector1(MakeInputArray<vtkm::FloatDefault>(0));
  std::cout << "  First component from Vector3." << std::endl;
  TryVector1(MakeInputArray<vtkm::Vec<vtkm::FloatDefault,3> >(0));
}

void TrySpecialArrays()
{
  std::cout << "Trying special arrays." << std::endl;

  typedef vtkm::cont::ArrayHandleIndex ArrayType1;
  ArrayType1 array1(ARRAY_SIZE);

  typedef vtkm::cont::ArrayHandleConstant<vtkm::Id> ArrayType2;
  ArrayType2 array2(295, ARRAY_SIZE);

  typedef vtkm::cont::ArrayHandleCompositeVectorType<ArrayType1,ArrayType2>::type
      CompositeArrayType;

  CompositeArrayType compositeArray =
      vtkm::cont::make_ArrayHandleCompositeVector(array1, 0, array2, 0);

  vtkm::cont::printSummary_ArrayHandle(compositeArray, std::cout);
  std::cout << std::endl;

  VTKM_TEST_ASSERT(compositeArray.GetNumberOfValues() == ARRAY_SIZE,
                   "Wrong array size.");

  CompositeArrayType::PortalConstControl compositePortal =
      compositeArray.GetPortalConstControl();
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    VTKM_TEST_ASSERT(
          test_equal(compositePortal.Get(index), vtkm::Id2(index,295)),
          "Bad value.");
  }
}

void TestBadArrayLengths() {
  std::cout << "Checking behavior when size of input arrays do not agree."
            << std::endl;

  typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault, StorageTag> InArrayType;
  InArrayType longInArray = MakeInputArray<vtkm::FloatDefault>(0);
  InArrayType shortInArray = MakeInputArray<vtkm::FloatDefault>(1);
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

  TrySpecialArrays();

  TestBadArrayLengths();
}

} // anonymous namespace

int UnitTestArrayHandleCompositeVector(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestCompositeVector);
}
