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

#include <vtkm/worklet/Clip.h>

#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/testing/Testing.h>

#include <vector>

typedef vtkm::Vec<vtkm::Float32, 3> Coord3D;
typedef vtkm::Vec<vtkm::Float32, 2> Coord2D;

const vtkm::Float32 clipValue = 0.5;


template<typename T, typename Storage>
bool TestArrayHandle(const vtkm::cont::ArrayHandle<T, Storage> &ah, const T *expected,
                     vtkm::Id size)
{
  if (size != ah.GetNumberOfValues())
  {
    return false;
  }

  for (vtkm::Id i = 0; i < size; ++i)
  {
    if (ah.GetPortalConstControl().Get(i) != expected[i])
    {
      return false;
    }
  }

  return true;
}

vtkm::cont::DataSet MakeTestDatasetExplicit()
{
  static const vtkm::Id numVerts = 4;
  static const vtkm::Id numCells = 2;
  static const Coord3D coords[numVerts] = {
    Coord3D(0.0f, 0.0f, 0.0f),
    Coord3D(1.0f, 0.0f, 0.0f),
    Coord3D(1.0f, 1.0f, 0.0f),
    Coord3D(0.0f, 1.0f, 0.0f),
  };
  static vtkm::Float32 values[] = { 1.0, 2.0, 1.0, 0.0 };
  static vtkm::Id shapes[] = { vtkm::VTKM_TRIANGLE, vtkm::VTKM_TRIANGLE };
  static vtkm::Id numInds[] = { 3, 3 };
  static vtkm::Id connectivity[] = {  0, 1, 3, 3, 1, 2 };

  vtkm::cont::DataSet ds;
  ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", 1, coords, numVerts));
  ds.AddField(vtkm::cont::Field("scalars", 1, vtkm::cont::Field::ASSOC_POINTS, values,
                     numVerts));

  std::vector<vtkm::Id> shapesVec(shapes, shapes + numCells);
  std::vector<vtkm::Id> numIndsVec(numInds, numInds + numCells);
  std::vector<vtkm::Id> connectivityVec(connectivity, connectivity + (numCells * 3));

  vtkm::cont::CellSetExplicit<> cs("cells", 3);
  cs.FillViaCopy(shapesVec, numIndsVec, connectivityVec);

  ds.AddCellSet(cs);

  return ds;
}

vtkm::cont::DataSet MakeTestDatasetStructured()
{
  static const vtkm::Vec<vtkm::Float32, 3> origin(0.0f, 0.0f, 0.0f);
  static const vtkm::Vec<vtkm::Float32, 3> spacing(1.0f, 1.0f, 1.0f);
  static const vtkm::Id3 dim(3, 3, 1);
  static const vtkm::Id numVerts = dim[0] * dim[1];

  vtkm::Float32 scalars[numVerts];
  for (vtkm::Id i = 0; i < numVerts; ++i)
  {
    scalars[i] = 1.0f;
  }
  scalars[4] = 0.0f;

  vtkm::cont::DataSet ds;
  ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", 1, dim, origin, spacing));

  ds.AddField(vtkm::cont::Field("scalars", 1, vtkm::cont::Field::ASSOC_POINTS,
                                scalars, numVerts));

  vtkm::cont::CellSetStructured<2> cs("cells");
  cs.SetPointDimensions(vtkm::make_Vec(dim[0], dim[1]));
  ds.AddCellSet(cs);

  return ds;
}

template <typename DeviceAdapter>
void TestClippingExplicit()
{
  vtkm::cont::DataSet ds = MakeTestDatasetExplicit();

  vtkm::worklet::Clip<DeviceAdapter> clip;
  vtkm::cont::CellSetExplicit<> outputCellSet =
      clip.Run(ds.GetCellSet(0), ds.GetField("scalars").GetData(), clipValue);


  vtkm::cont::DynamicArrayHandle coords =
      clip.ProcessField(ds.GetCoordinateSystem("coordinates").GetData());
  vtkm::cont::DynamicArrayHandle scalars =
      clip.ProcessField(ds.GetField("scalars").GetData());


  vtkm::Id connectivitySize = 12;
  vtkm::Id fieldSize = 7;
  vtkm::Id expectedConnectivity[] = { 5, 4, 0, 5, 0, 1, 5, 1, 6, 6, 1, 2 };
  Coord3D expectedCoords[] = {
    Coord3D(0.00f, 0.00f, 0.0f), Coord3D(1.00f, 0.00f, 0.0f),
    Coord3D(1.00f, 1.00f, 0.0f), Coord3D(0.00f, 1.00f, 0.0f),
    Coord3D(0.00f, 0.50f, 0.0f), Coord3D(0.25f, 0.75f, 0.0f),
    Coord3D(0.50f, 1.00f, 0.0f),
  };
  vtkm::Float32 expectedScalars[] = { 1, 2, 1, 0, 0.5, 0.5, 0.5 };

  VTKM_TEST_ASSERT(
      TestArrayHandle(outputCellSet.GetConnectivityArray(), expectedConnectivity,
        connectivitySize),
      "Got incorrect conectivity");

  VTKM_TEST_ASSERT(
      TestArrayHandle(coords.CastToArrayHandle(Coord3D(),
        VTKM_DEFAULT_STORAGE_TAG()), expectedCoords, fieldSize),
      "Got incorrect coordinates");

  VTKM_TEST_ASSERT(
      TestArrayHandle(scalars.CastToArrayHandle(vtkm::Float32(),
        VTKM_DEFAULT_STORAGE_TAG()), expectedScalars, fieldSize),
      "Got incorrect scalars");
}

template <typename DeviceAdapter>
void TestClippingStrucutred()
{
  vtkm::cont::DataSet ds = MakeTestDatasetStructured();

  vtkm::worklet::Clip<DeviceAdapter> clip;
  vtkm::cont::CellSetExplicit<> outputCellSet =
      clip.Run(ds.GetCellSet(0), ds.GetField("scalars").GetData(), clipValue);


  vtkm::cont::DynamicArrayHandle coords =
      clip.ProcessField(ds.GetCoordinateSystem("coordinates").GetData());
  vtkm::cont::DynamicArrayHandle scalars =
      clip.ProcessField(ds.GetField("scalars").GetData());

  vtkm::Id connectivitySize = 36;
  vtkm::Id fieldSize = 13;
  vtkm::Id expectedConnectivity[] = {
    0,  1,  9,   0,  9, 10,   0, 10,  3,   1,  2,  9,   2, 11,  9,   2,  5, 11,
    3, 10,  6,  10, 12,  6,  12,  7,  6,  11,  5,  8,  11,  8, 12,   8,  7, 12 };
  Coord3D expectedCoords[] = {
    Coord3D(0.0f, 0.0f, 0.0f), Coord3D(1.0f, 0.0f, 0.0f), Coord3D(2.0f, 0.0f, 0.0f),
    Coord3D(0.0f, 1.0f, 0.0f), Coord3D(1.0f, 1.0f, 0.0f), Coord3D(2.0f, 1.0f, 0.0f),
    Coord3D(0.0f, 2.0f, 0.0f), Coord3D(1.0f, 2.0f, 0.0f), Coord3D(2.0f, 2.0f, 0.0f),
    Coord3D(1.0f, 0.5f, 0.0f), Coord3D(0.5f, 1.0f, 0.0f), Coord3D(1.5f, 1.0f, 0.0f),
    Coord3D(1.0f, 1.5f, 0.0f),
  };
  vtkm::Float32 expectedScalars[] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.5, 0.5, 0.5, 0.5 };

  VTKM_TEST_ASSERT(
      TestArrayHandle(outputCellSet.GetConnectivityArray(), expectedConnectivity,
        connectivitySize),
      "Got incorrect conectivity");

  VTKM_TEST_ASSERT(
      TestArrayHandle(coords.CastToArrayHandle(Coord3D(),
        VTKM_DEFAULT_STORAGE_TAG()), expectedCoords, fieldSize),
      "Got incorrect coordinates");

  VTKM_TEST_ASSERT(
      TestArrayHandle(scalars.CastToArrayHandle(vtkm::Float32(),
        VTKM_DEFAULT_STORAGE_TAG()), expectedScalars, fieldSize),
      "Got incorrect scalars");
}

template <typename DeviceAdapter>
void TestClipping()
{
  std::cout << "Testing explicit dataset:" << std::endl;
  TestClippingExplicit<DeviceAdapter>();
  std::cout << "Testing structured dataset:" << std::endl;
  TestClippingStrucutred<DeviceAdapter>();
}

int UnitTestClipping(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(
      TestClipping<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>);
}
