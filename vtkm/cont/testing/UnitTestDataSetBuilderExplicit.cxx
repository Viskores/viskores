//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/testing/ExplicitTestData.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace DataSetBuilderExplicitNamespace
{

using DFA = vtkm::cont::Algorithm;

template <typename T>
vtkm::Bounds ComputeBounds(std::size_t numPoints, const T* coords)
{
  vtkm::Bounds bounds;

  for (std::size_t i = 0; i < numPoints; i++)
  {
    bounds.Include(vtkm::Vec<T, 3>(coords[i * 3 + 0], coords[i * 3 + 1], coords[i * 3 + 2]));
  }

  return bounds;
}

void ValidateDataSet(const vtkm::cont::DataSet& ds,
                     vtkm::Id numPoints,
                     vtkm::Id numCells,
                     const vtkm::Bounds& bounds)
{
  //Verify basics..
  VTKM_TEST_ASSERT(ds.GetNumberOfFields() == 2, "Wrong number of fields.");
  VTKM_TEST_ASSERT(ds.GetNumberOfCoordinateSystems() == 1, "Wrong number of coordinate systems.");
  VTKM_TEST_ASSERT(ds.GetNumberOfPoints() == numPoints, "Wrong number of coordinates.");
  VTKM_TEST_ASSERT(ds.GetNumberOfCells() == numCells, "Wrong number of cells.");

  // test various field-getting methods and associations
  try
  {
    ds.GetCellField("cellvar");
  }
  catch (...)
  {
    VTKM_TEST_FAIL("Failed to get field 'cellvar' with Association::CELL_SET.");
  }

  try
  {
    ds.GetPointField("pointvar");
  }
  catch (...)
  {
    VTKM_TEST_FAIL("Failed to get field 'pointvar' with ASSOC_POINT_SET.");
  }

  //Make sure bounds are correct.
  vtkm::Bounds computedBounds = ds.GetCoordinateSystem().GetBounds();
  VTKM_TEST_ASSERT(test_equal(bounds, computedBounds), "Bounds of coordinates do not match");
}

template <typename T>
std::vector<T> createVec(std::size_t n, const T* data)
{
  std::vector<T> vec(n);
  for (std::size_t i = 0; i < n; i++)
  {
    vec[i] = data[i];
  }
  return vec;
}

template <typename T>
vtkm::cont::ArrayHandle<T> createAH(std::size_t n, const T* data)
{
  vtkm::cont::ArrayHandle<T> arr;
  DFA::Copy(vtkm::cont::make_ArrayHandle(data, static_cast<vtkm::Id>(n)), arr);
  return arr;
}

template <typename T>
vtkm::cont::DataSet CreateDataSetArr(bool useSeparatedCoords,
                                     std::size_t numPoints,
                                     const T* coords,
                                     std::size_t numCells,
                                     std::size_t numConn,
                                     const vtkm::Id* conn,
                                     const vtkm::IdComponent* indices,
                                     const vtkm::UInt8* shape)
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetFieldAdd dsf;
  vtkm::cont::DataSetBuilderExplicit dsb;
  float f = 0.0f;
  if (useSeparatedCoords)
  {
    std::vector<T> xvals(numPoints), yvals(numPoints), zvals(numPoints);
    std::vector<T> varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      xvals[i] = coords[i * 3 + 0];
      yvals[i] = coords[i * 3 + 1];
      zvals[i] = coords[i * 3 + 2];
      varP[i] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i] = static_cast<T>(f * 1.1f);
    }
    vtkm::cont::ArrayHandle<T> X, Y, Z, P, C;
    DFA::Copy(vtkm::cont::make_ArrayHandle(xvals), X);
    DFA::Copy(vtkm::cont::make_ArrayHandle(yvals), Y);
    DFA::Copy(vtkm::cont::make_ArrayHandle(zvals), Z);
    DFA::Copy(vtkm::cont::make_ArrayHandle(varP), P);
    DFA::Copy(vtkm::cont::make_ArrayHandle(varC), C);
    dataSet = dsb.Create(
      X, Y, Z, createAH(numCells, shape), createAH(numCells, indices), createAH(numConn, conn));
    dsf.AddPointField(dataSet, "pointvar", P);
    dsf.AddCellField(dataSet, "cellvar", C);
    return dataSet;
  }
  else
  {
    std::vector<vtkm::Vec<T, 3>> tmp(numPoints);
    std::vector<vtkm::Vec<T, 1>> varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      tmp[i][0] = coords[i * 3 + 0];
      tmp[i][1] = coords[i * 3 + 1];
      tmp[i][2] = coords[i * 3 + 2];
      varP[i][0] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i][0] = static_cast<T>(f * 1.1f);
    }
    vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>> pts;
    DFA::Copy(vtkm::cont::make_ArrayHandle(tmp), pts);
    dataSet = dsb.Create(
      pts, createAH(numCells, shape), createAH(numCells, indices), createAH(numConn, conn));
    dsf.AddPointField(dataSet, "pointvar", varP);
    dsf.AddCellField(dataSet, "cellvar", varC);
    return dataSet;
  }
}

template <typename T>
vtkm::cont::DataSet CreateDataSetVec(bool useSeparatedCoords,
                                     std::size_t numPoints,
                                     const T* coords,
                                     std::size_t numCells,
                                     std::size_t numConn,
                                     const vtkm::Id* conn,
                                     const vtkm::IdComponent* indices,
                                     const vtkm::UInt8* shape)
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetFieldAdd dsf;
  vtkm::cont::DataSetBuilderExplicit dsb;

  float f = 0.0f;
  if (useSeparatedCoords)
  {
    std::vector<T> X(numPoints), Y(numPoints), Z(numPoints), varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      X[i] = coords[i * 3 + 0];
      Y[i] = coords[i * 3 + 1];
      Z[i] = coords[i * 3 + 2];
      varP[i] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i] = static_cast<T>(f * 1.1f);
    }
    dataSet = dsb.Create(
      X, Y, Z, createVec(numCells, shape), createVec(numCells, indices), createVec(numConn, conn));
    dsf.AddPointField(dataSet, "pointvar", varP);
    dsf.AddCellField(dataSet, "cellvar", varC);
    return dataSet;
  }
  else
  {
    std::vector<vtkm::Vec<T, 3>> pts(numPoints);
    std::vector<vtkm::Vec<T, 1>> varP(numPoints), varC(numCells);
    for (std::size_t i = 0; i < numPoints; i++, f++)
    {
      pts[i][0] = coords[i * 3 + 0];
      pts[i][1] = coords[i * 3 + 1];
      pts[i][2] = coords[i * 3 + 2];
      varP[i][0] = static_cast<T>(f * 1.1f);
    }
    f = 0.0f;
    for (std::size_t i = 0; i < numCells; i++, f++)
    {
      varC[i][0] = static_cast<T>(f * 1.1f);
    }
    dataSet = dsb.Create(
      pts, createVec(numCells, shape), createVec(numCells, indices), createVec(numConn, conn));
    dsf.AddPointField(dataSet, "pointvar", varP);
    dsf.AddCellField(dataSet, "cellvar", varC);
    return dataSet;
  }
}

#define TEST_DATA(num)                                                                             \
  vtkm::cont::testing::ExplicitData##num::numPoints,                                               \
    vtkm::cont::testing::ExplicitData##num::coords,                                                \
    vtkm::cont::testing::ExplicitData##num::numCells,                                              \
    vtkm::cont::testing::ExplicitData##num::numConn, vtkm::cont::testing::ExplicitData##num::conn, \
    vtkm::cont::testing::ExplicitData##num::numIndices,                                            \
    vtkm::cont::testing::ExplicitData##num::shapes
#define TEST_NUMS(num)                                                                             \
  vtkm::cont::testing::ExplicitData##num::numPoints,                                               \
    vtkm::cont::testing::ExplicitData##num::numCells
#define TEST_BOUNDS(num)                                                                           \
  vtkm::cont::testing::ExplicitData##num::numPoints, vtkm::cont::testing::ExplicitData##num::coords

void TestDataSetBuilderExplicit()
{
  vtkm::cont::DataSetBuilderExplicit dsb;
  vtkm::cont::DataSet ds;
  vtkm::Bounds bounds;

  //Iterate over organization of coordinates.
  for (int i = 0; i < 2; i++)
  {
    //Test ExplicitData0
    bounds = ComputeBounds(TEST_BOUNDS(0));
    ds = CreateDataSetArr(i == 0, TEST_DATA(0));
    ValidateDataSet(ds, TEST_NUMS(0), bounds);
    ds = CreateDataSetVec(i == 0, TEST_DATA(0));
    ValidateDataSet(ds, TEST_NUMS(0), bounds);

    //Test ExplicitData1
    bounds = ComputeBounds(TEST_BOUNDS(1));
    ds = CreateDataSetArr(i == 0, TEST_DATA(1));
    ValidateDataSet(ds, TEST_NUMS(1), bounds);
    ds = CreateDataSetVec(i == 0, TEST_DATA(1));
    ValidateDataSet(ds, TEST_NUMS(1), bounds);

    //Test ExplicitData2
    bounds = ComputeBounds(TEST_BOUNDS(2));
    ds = CreateDataSetArr(i == 0, TEST_DATA(2));
    ValidateDataSet(ds, TEST_NUMS(2), bounds);
    ds = CreateDataSetVec(i == 0, TEST_DATA(2));
    ValidateDataSet(ds, TEST_NUMS(2), bounds);
  }
}

} // namespace DataSetBuilderExplicitNamespace

int UnitTestDataSetBuilderExplicit(int argc, char* argv[])
{
  using namespace DataSetBuilderExplicitNamespace;
  return vtkm::cont::testing::Testing::Run(TestDataSetBuilderExplicit, argc, argv);
}
