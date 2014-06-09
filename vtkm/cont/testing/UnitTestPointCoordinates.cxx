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

// Make sure nothing relies on default container or device adapter.
#define VTKM_ARRAY_CONTAINER_CONTROL VTKM_ARRAY_CONTAINER_CONTROL_ERROR
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_ERROR

// Make sure nothing relies on default lists.
#define VTKM_DEFAULT_TYPE_LIST_TAG ::vtkm::ListTagEmpty
#define VTKM_DEFAULT_CONTAINER_LIST_TAG ::vtkm::ListTagEmpty

#include <vtkm/cont/PointCoordinatesArray.h>
#include <vtkm/cont/PointCoordinatesUniform.h>

#include <vtkm/Extent.h>
#include <vtkm/TypeListTag.h>

#include <vtkm/cont/ArrayContainerControlBasic.h>
#include <vtkm/cont/DeviceAdapterSerial.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace {

const vtkm::Extent3 EXTENT = vtkm::Extent3(vtkm::Id3(0,0,0), vtkm::Id3(9,9,9));
const vtkm::Vector3 ORIGIN = vtkm::Vector3(0, 0, 0);
const vtkm::Vector3 SPACING = vtkm::Vector3(1, 1, 1);

const vtkm::Id3 DIMENSION = vtkm::ExtentPointDimensions(EXTENT);
const vtkm::Id ARRAY_SIZE = DIMENSION[0]*DIMENSION[1]*DIMENSION[2];

typedef vtkm::cont::ArrayContainerControlTagBasic Container;

struct ContainerListTag : vtkm::cont::ContainerListTagBasic {  };

vtkm::Vector3 TestValue(vtkm::Id index)
{
  vtkm::Id3 index3d = vtkm::ExtentPointFlatIndexToTopologyIndex(index, EXTENT);
  return vtkm::Vector3(index3d[0], index3d[1], index3d[2]);
}

struct CheckArray
{
  template<typename C>
  void operator()(
      const vtkm::cont::ArrayHandle<vtkm::Vector3,C> &array) const
  {
    std::cout << "    In CastAndCall functor" << std::endl;
    typename vtkm::cont::ArrayHandle<vtkm::Vector3,C>::PortalConstControl portal =
        array.GetPortalConstControl();

    VTKM_TEST_ASSERT(portal.GetNumberOfValues() == ARRAY_SIZE,
                     "Array has wrong number of values.");

    for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
    {
      const vtkm::Vector3 receivedValue = portal.Get(index);
      const vtkm::Vector3 expectedValue = TestValue(index);
      VTKM_TEST_ASSERT(receivedValue == expectedValue,
                       "Got bad value in array.");
    }
  }
};

void TestPointCoordinatesArray()
{
  std::cout << "Testing PointCoordinatesArray" << std::endl;

  std::cout << "  Creating buffer of data values" << std::endl;
  std::vector<vtkm::Vector3> buffer(ARRAY_SIZE);
  for (vtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    buffer[index] = TestValue(index);
  }

  std::cout << "  Creating and checking array handle" << std::endl;
  vtkm::cont::ArrayHandle<vtkm::Vector3,Container> array =
      vtkm::cont::make_ArrayHandle(buffer, Container());
  CheckArray()(array);

  std::cout << "  Creating and checking PointCoordinatesArray" << std::endl;
  vtkm::cont::PointCoordinatesArray pointCoordinates =
      vtkm::cont::PointCoordinatesArray(array);
  pointCoordinates.CastAndCall(
        CheckArray(),
        vtkm::ListTagEmpty(), // Internally sets to Vector3
        vtkm::cont::ContainerListTagBasic());
}

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
  TestPointCoordinatesArray();
  TestPointCoordinatesUniform();
}

} // anonymous namespace

int UnitTestPointCoordinates(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(PointCoordinatesTests);
}
