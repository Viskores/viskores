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

// Make sure nothing relies on default storage or device adapter.
#define VTKM_STORAGE VTKM_STORAGE_ERROR
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR

// Make sure nothing relies on default lists.
#define VTKM_DEFAULT_TYPE_LIST_TAG ::vtkm::ListTagEmpty
#define VTKM_DEFAULT_STORAGE_LIST_TAG ::vtkm::ListTagEmpty

#include <vtkm/cont/PointCoordinatesArray.h>
#include <vtkm/cont/PointCoordinatesUniform.h>

#include <vtkm/Extent.h>
#include <vtkm/TypeListTag.h>

#include <vtkm/cont/DeviceAdapterSerial.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace {

const vtkm::Extent3 EXTENT = vtkm::Extent3(vtkm::Id3(0,0,0), vtkm::Id3(9,9,9));
const vtkm::Vec<vtkm::FloatDefault,3> ORIGIN =
    vtkm::Vec<vtkm::FloatDefault,3>(0, 0, 0);
const vtkm::Vec<vtkm::FloatDefault,3> SPACING =
    vtkm::Vec<vtkm::FloatDefault,3>(1, 1, 1);

const vtkm::Id3 DIMENSION = vtkm::ExtentPointDimensions(EXTENT);
const vtkm::Id ARRAY_SIZE = DIMENSION[0]*DIMENSION[1]*DIMENSION[2];

typedef vtkm::cont::StorageTagBasic StorageTag;

struct StorageListTag : vtkm::cont::StorageListTagBasic {  };

vtkm::Vec<vtkm::FloatDefault,3> ExpectedCoordinates(vtkm::Id index)
{
  vtkm::Id3 index3d = vtkm::ExtentPointFlatIndexToTopologyIndex(index, EXTENT);
  return vtkm::make_Vec(vtkm::FloatDefault(index3d[0]),
                        vtkm::FloatDefault(index3d[1]),
                        vtkm::FloatDefault(index3d[2]));
}

struct CheckArray
{
  template<typename ArrayType>
  void operator()(const ArrayType &array) const
  {
    typedef typename ArrayType::ValueType ValueType;

    std::cout << "    In CastAndCall functor" << std::endl;
    typename ArrayType::PortalConstControl portal =
        array.GetPortalConstControl();

    VTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE,
                     "Array has wrong number of values.");

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      const ValueType receivedValue = portal.Get(index);
      const ValueType expectedValue = ExpectedCoordinates(index);
      VTKM_TEST_ASSERT(receivedValue == expectedValue,
                       "Got bad value in array.");
    }
  }
};

struct TestPointCoordinatesArray
{
  template<typename Vector3>
  void operator()(Vector3) const
  {
    std::cout << "Testing PointCoordinatesArray" << std::endl;

    std::cout << "  Creating buffer of data values" << std::endl;
    std::vector<Vector3> buffer(ARRAY_SIZE);
    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      buffer[index] = ExpectedCoordinates(index);
    }

    std::cout << "  Creating and checking array handle" << std::endl;
    vtkm::cont::ArrayHandle<Vector3,StorageTag> array =
        vtkm::cont::make_ArrayHandle(buffer);
    CheckArray()(array);

    std::cout << "  Creating and checking PointCoordinatesArray" << std::endl;
    vtkm::cont::PointCoordinatesArray pointCoordinates =
        vtkm::cont::PointCoordinatesArray(array);
    pointCoordinates.CastAndCall(
          CheckArray(),
          vtkm::ListTagEmpty(), // Internally sets to Vector3
          vtkm::cont::StorageListTagBasic());
  }
};

void TestPointCoordinatesUniform()
{
  std::cout << "Testing PointCoordinatesUniform" << std::endl;

  vtkm::cont::PointCoordinatesUniform pointCoordinates =
      vtkm::cont::PointCoordinatesUniform(EXTENT, ORIGIN, SPACING);
  pointCoordinates.CastAndCall(
        CheckArray(),
        vtkm::ListTagEmpty(), // Not used
        vtkm::ListTagEmpty()); // Not used
}

void PointCoordinatesTests()
{
  vtkm::testing::Testing::TryTypes(TestPointCoordinatesArray(),
                                   vtkm::TypeListTagFieldVec3());
  TestPointCoordinatesUniform();
}

} // anonymous namespace

int UnitTestPointCoordinates(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(PointCoordinatesTests);
}
