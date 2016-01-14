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
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderRegular.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/ImplicitFunctions.h>

#include <vector>

typedef vtkm::Vec<vtkm::FloatDefault, 3> Coord3D;

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
  std::vector<Coord3D> coords;
  coords.push_back( Coord3D(0.0f, 0.0f, 0.0f) );
  coords.push_back( Coord3D(1.0f, 0.0f, 0.0f) );
  coords.push_back( Coord3D(1.0f, 1.0f, 0.0f) );
  coords.push_back( Coord3D(0.0f, 1.0f, 0.0f) );

  std::vector<vtkm::Id> connectivity;
  connectivity.push_back(0);
  connectivity.push_back(1);
  connectivity.push_back(3);
  connectivity.push_back(3);
  connectivity.push_back(1);
  connectivity.push_back(2);

  vtkm::cont::DataSet ds;
  vtkm::cont::DataSetBuilderExplicit builder;
  ds = builder.Create(coords, vtkm::CellShapeTagTriangle(), connectivity, "coords");

  std::vector<vtkm::Float32> values;
  values.push_back(1.0);
  values.push_back(2.0);
  values.push_back(1.0);
  values.push_back(0.0);
  vtkm::cont::DataSetFieldAdd fieldAdder;
  fieldAdder.AddPointField(ds, "scalars", values);

  return ds;
}

vtkm::cont::DataSet MakeTestDatasetStructured()
{
  static const vtkm::Id xdim = 3, ydim = 3;
  static const vtkm::Id2 dim(xdim, ydim);
  static const vtkm::Id numVerts = xdim * ydim;

  vtkm::Float32 scalars[numVerts];
  for (vtkm::Id i = 0; i < numVerts; ++i)
  {
    scalars[i] = 1.0f;
  }
  scalars[4] = 0.0f;

  vtkm::cont::DataSet ds;
  vtkm::cont::DataSetBuilderRegular builder;
  ds = builder.Create(dim);

  vtkm::cont::DataSetFieldAdd fieldAdder;
  fieldAdder.AddPointField(ds, "scalars", scalars, numVerts);

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
      clip.ProcessField(ds.GetCoordinateSystem("coords").GetData());
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
      TestArrayHandle(outputCellSet.GetConnectivityArray(
        vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell()),
        expectedConnectivity,
        connectivitySize),
      "Got incorrect conectivity");

  VTKM_TEST_ASSERT(
      TestArrayHandle(
          coords.CastToTypeStorage<Coord3D,VTKM_DEFAULT_STORAGE_TAG>(),
          expectedCoords,
          fieldSize),
      "Got incorrect coordinates");

  VTKM_TEST_ASSERT(
      TestArrayHandle(
          scalars.CastToTypeStorage<vtkm::Float32,VTKM_DEFAULT_STORAGE_TAG>(),
          expectedScalars,
          fieldSize),
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
      clip.ProcessField(ds.GetCoordinateSystem("coords").GetData());
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
      TestArrayHandle(outputCellSet.GetConnectivityArray(
        vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell()),
        expectedConnectivity,
        connectivitySize),
      "Got incorrect conectivity");

  VTKM_TEST_ASSERT(
      TestArrayHandle(
          coords.CastToTypeStorage<Coord3D,VTKM_DEFAULT_STORAGE_TAG>(),
          expectedCoords,
          fieldSize),
      "Got incorrect coordinates");

  VTKM_TEST_ASSERT(
      TestArrayHandle(
          scalars.CastToTypeStorage<vtkm::Float32,VTKM_DEFAULT_STORAGE_TAG>(),
          expectedScalars,
          fieldSize),
      "Got incorrect scalars");
}

template <typename DeviceAdapter>
void TestClippingWithImplicitFunction()
{
  vtkm::Vec<vtkm::FloatDefault, 3> center(1, 1, 0);
  vtkm::FloatDefault radius(0.5);
  vtkm::Sphere sphere(center, radius);

  vtkm::cont::DataSet ds = MakeTestDatasetStructured();

  vtkm::worklet::Clip<DeviceAdapter> clip;
  vtkm::cont::CellSetExplicit<> outputCellSet =
      clip.Run(ds.GetCellSet(0), sphere, ds.GetCoordinateSystem("coords"));


  vtkm::cont::DynamicArrayHandle coords =
      clip.ProcessField(ds.GetCoordinateSystem("coords").GetData());
  vtkm::cont::DynamicArrayHandle scalars =
      clip.ProcessField(ds.GetField("scalars").GetData());

  vtkm::Id connectivitySize = 36;
  vtkm::Id fieldSize = 13;
  vtkm::Id expectedConnectivity[] = {
    0,  1,  9,   0,  9, 10,   0, 10,  3,   1,  2,  9,   2, 11,  9,   2,  5, 11,
    3, 10,  6,  10, 12,  6,  12,  7,  6,  11,  5,  8,  11,  8, 12,   8,  7, 12 };
  Coord3D expectedCoords[] = {
    Coord3D(0.0f,  0.0f, 0.0f), Coord3D( 1.0f, 0.0f, 0.0f), Coord3D( 2.0f, 0.0f, 0.0f),
    Coord3D(0.0f,  1.0f, 0.0f), Coord3D( 1.0f, 1.0f, 0.0f), Coord3D( 2.0f, 1.0f, 0.0f),
    Coord3D(0.0f,  2.0f, 0.0f), Coord3D( 1.0f, 2.0f, 0.0f), Coord3D( 2.0f, 2.0f, 0.0f),
    Coord3D(1.0f, 0.75f, 0.0f), Coord3D(0.75f, 1.0f, 0.0f), Coord3D(1.25f, 1.0f, 0.0f),
    Coord3D(1.0f, 1.25f, 0.0f),
  };
  vtkm::Float32 expectedScalars[] = { 1, 1, 1, 1, 0, 1, 1, 1, 1,
                                      0.25, 0.25, 0.25, 0.25 };

  VTKM_TEST_ASSERT(
      TestArrayHandle(outputCellSet.GetConnectivityArray(
        vtkm::TopologyElementTagPoint(),vtkm::TopologyElementTagCell()),
        expectedConnectivity,
        connectivitySize),
      "Got incorrect conectivity");

  VTKM_TEST_ASSERT(
      TestArrayHandle(
          coords.CastToTypeStorage<Coord3D,VTKM_DEFAULT_STORAGE_TAG>(),
          expectedCoords,
          fieldSize),
      "Got incorrect coordinates");

  VTKM_TEST_ASSERT(
      TestArrayHandle(
          scalars.CastToTypeStorage<vtkm::Float32,VTKM_DEFAULT_STORAGE_TAG>(),
          expectedScalars,
          fieldSize),
      "Got incorrect scalars");
}

template <typename DeviceAdapter>
void TestClipping()
{
  std::cout << "Testing explicit dataset:" << std::endl;
  TestClippingExplicit<DeviceAdapter>();
  std::cout << "Testing structured dataset:" << std::endl;
  TestClippingStrucutred<DeviceAdapter>();
  std::cout << "Testing clipping with implicit function (sphere):" << std::endl;
  TestClippingWithImplicitFunction<DeviceAdapter>();
}

int UnitTestClipping(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(
      TestClipping<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>);
}
