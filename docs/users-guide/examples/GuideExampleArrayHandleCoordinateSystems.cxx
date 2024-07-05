//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

#include <vtkm/cont/testing/Testing.h>

namespace
{

void UniformPointCoordinates()
{
  std::cout << "Trying uniform point coordinates." << std::endl;

  ////
  //// BEGIN-EXAMPLE ArrayHandleUniformPointCoordinates
  ////
  // Create a set of point coordinates for a uniform grid in the space between
  // -5 and 5 in the x direction and -3 and 3 in the y and z directions. The
  // uniform sampling is spaced in 0.08 unit increments in the x direction (for
  // 126 samples), 0.08 unit increments in the y direction (for 76 samples) and
  // 0.24 unit increments in the z direction (for 26 samples). That makes
  // 248,976 values in the array total.
  vtkm::cont::ArrayHandleUniformPointCoordinates uniformCoordinates(
    vtkm::Id3(126, 76, 26),
    vtkm::Vec3f{ -5.0f, -3.0f, -3.0f },
    vtkm::Vec3f{ 0.08f, 0.08f, 0.24f });
  ////
  //// END-EXAMPLE ArrayHandleUniformPointCoordinates
  ////

  VTKM_TEST_ASSERT(uniformCoordinates.GetNumberOfValues() == 248976,
                   "Wrong number of values in uniform coordinates.");
  VTKM_TEST_ASSERT(
    test_equal(uniformCoordinates.ReadPortal().Get(0), vtkm::Vec3f{ -5.0, -3.0, -3.0 }),
    "Bad first point coordinate.");
  VTKM_TEST_ASSERT(test_equal(uniformCoordinates.ReadPortal().Get(248975),
                              vtkm::Vec3f{ 5.0, 3.0, 3.0 }),
                   "Bad last point coordinate.");
}

template<typename ArrayHandleType>
void CheckRectilinearPointCoordinates(ArrayHandleType rectilinearCoordinates)
{
  VTKM_TEST_ASSERT(rectilinearCoordinates.GetNumberOfValues() == 12,
                   "Wrong number of values.");

  VTKM_TEST_ASSERT(test_equal(rectilinearCoordinates.ReadPortal().Get(0),
                              vtkm::Vec3f{ 0.0f, 0.0f, 0.0f }),
                   "Bad value at 0.");
  VTKM_TEST_ASSERT(test_equal(rectilinearCoordinates.ReadPortal().Get(4),
                              vtkm::Vec3f{ 1.1f, 2.0f, 0.0f }),
                   "Bad value at 4.");
  VTKM_TEST_ASSERT(test_equal(rectilinearCoordinates.ReadPortal().Get(11),
                              vtkm::Vec3f{ 5.0f, 2.0f, 0.5f }),
                   "Bad value at 11.");
}

void RectilinearPointCoordinates()
{
  std::cout << "Trying rectilinear point coordinates." << std::endl;

  ////
  //// BEGIN-EXAMPLE ArrayHandleCartesianProduct
  ////
  using AxisArrayType = vtkm::cont::ArrayHandle<vtkm::Float32>;
  using AxisPortalType = AxisArrayType::WritePortalType;

  // Create array for x axis coordinates with values [0.0, 1.1, 5.0]
  AxisArrayType xAxisArray;
  xAxisArray.Allocate(3);
  AxisPortalType xAxisPortal = xAxisArray.WritePortal();
  xAxisPortal.Set(0, 0.0f);
  xAxisPortal.Set(1, 1.1f);
  xAxisPortal.Set(2, 5.0f);

  // Create array for y axis coordinates with values [0.0, 2.0]
  AxisArrayType yAxisArray;
  yAxisArray.Allocate(2);
  AxisPortalType yAxisPortal = yAxisArray.WritePortal();
  yAxisPortal.Set(0, 0.0f);
  yAxisPortal.Set(1, 2.0f);

  // Create array for z axis coordinates with values [0.0, 0.5]
  AxisArrayType zAxisArray;
  zAxisArray.Allocate(2);
  AxisPortalType zAxisPortal = zAxisArray.WritePortal();
  zAxisPortal.Set(0, 0.0f);
  zAxisPortal.Set(1, 0.5f);

  // Create point coordinates for a "rectilinear grid" with axis-aligned points
  // with variable spacing by taking the Cartesian product of the three
  // previously defined arrays. This generates the following 3x2x2 = 12 values:
  //
  // [0.0, 0.0, 0.0], [1.1, 0.0, 0.0], [5.0, 0.0, 0.0],
  // [0.0, 2.0, 0.0], [1.1, 2.0, 0.0], [5.0, 2.0, 0.0],
  // [0.0, 0.0, 0.5], [1.1, 0.0, 0.5], [5.0, 0.0, 0.5],
  // [0.0, 2.0, 0.5], [1.1, 2.0, 0.5], [5.0, 2.0, 0.5]
  vtkm::cont::ArrayHandleCartesianProduct<AxisArrayType, AxisArrayType, AxisArrayType>
    rectilinearCoordinates(xAxisArray, yAxisArray, zAxisArray);
  ////
  //// END-EXAMPLE ArrayHandleCartesianProduct
  ////
  CheckRectilinearPointCoordinates(rectilinearCoordinates);

  CheckRectilinearPointCoordinates(
    ////
    //// BEGIN-EXAMPLE MakeArrayHandleCartesianProduct
    ////
    vtkm::cont::make_ArrayHandleCartesianProduct(xAxisArray, yAxisArray, zAxisArray)
    ////
    //// END-EXAMPLE MakeArrayHandleCartesianProduct
    ////
  );
}

void Test()
{
  UniformPointCoordinates();
  RectilinearPointCoordinates();
}

} // anonymous namespace

int GuideExampleArrayHandleCoordinateSystems(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
