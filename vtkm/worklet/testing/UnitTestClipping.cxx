//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/ImplicitFunctionHandle.h>
#include <vtkm/cont/testing/Testing.h>

#include <vector>

using Coord3D = vtkm::Vec<vtkm::FloatDefault, 3>;

const vtkm::Float32 clipValue = 0.5;

template <typename T, typename Storage>
bool TestArrayHandle(const vtkm::cont::ArrayHandle<T, Storage>& ah,
                     const T* expected,
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
  coords.push_back(Coord3D(0.0f, 0.0f, 0.0f));
  coords.push_back(Coord3D(1.0f, 0.0f, 0.0f));
  coords.push_back(Coord3D(1.0f, 1.0f, 0.0f));
  coords.push_back(Coord3D(0.0f, 1.0f, 0.0f));

  std::vector<vtkm::Id> connectivity;
  connectivity.push_back(0);
  connectivity.push_back(1);
  connectivity.push_back(3);
  connectivity.push_back(3);
  connectivity.push_back(1);
  connectivity.push_back(2);

  vtkm::cont::DataSet ds;
  vtkm::cont::DataSetBuilderExplicit builder;
  ds = builder.Create(coords, vtkm::CellShapeTagTriangle(), 3, connectivity, "coords");

  vtkm::cont::DataSetFieldAdd fieldAdder;

  std::vector<vtkm::Float32> values;
  values.push_back(1.0);
  values.push_back(2.0);
  values.push_back(1.0);
  values.push_back(0.0);
  fieldAdder.AddPointField(ds, "scalars", values);

  values.clear();
  values.push_back(100.f);
  values.push_back(-100.f);
  fieldAdder.AddCellField(ds, "cellvar", values);

  return ds;
}

vtkm::cont::DataSet MakeTestDatasetStructured()
{
  static constexpr vtkm::Id xdim = 3, ydim = 3;
  static const vtkm::Id2 dim(xdim, ydim);
  static constexpr vtkm::Id numVerts = xdim * ydim;

  vtkm::Float32 scalars[numVerts];
  for (vtkm::Id i = 0; i < numVerts; ++i)
  {
    scalars[i] = 1.0f;
  }
  scalars[4] = 0.0f;

  vtkm::cont::DataSet ds;
  vtkm::cont::DataSetBuilderUniform builder;
  ds = builder.Create(dim);

  vtkm::cont::DataSetFieldAdd fieldAdder;
  fieldAdder.AddPointField(ds, "scalars", scalars, numVerts);

  std::vector<vtkm::Float32> cellvar = { -100.f, 100.f, 30.f, -30.f };
  fieldAdder.AddCellField(ds, "cellvar", cellvar);

  return ds;
}

template <typename DeviceAdapter>
void TestClippingExplicit()
{
  vtkm::cont::DataSet ds = MakeTestDatasetExplicit();

  vtkm::worklet::Clip clip;
  bool invertClip = false;
  vtkm::cont::CellSetExplicit<> outputCellSet = clip.Run(
    ds.GetCellSet(0), ds.GetField("scalars").GetData(), clipValue, invertClip, DeviceAdapter());

  auto coordsIn = ds.GetCoordinateSystem("coords").GetData();
  vtkm::cont::ArrayHandle<Coord3D> coords = clip.ProcessPointField(coordsIn, DeviceAdapter());

  vtkm::cont::ArrayHandle<vtkm::Float32> scalarsIn;
  ds.GetField("scalars").GetData().CopyTo(scalarsIn);
  vtkm::cont::ArrayHandle<vtkm::Float32> scalars =
    clip.ProcessPointField(scalarsIn, DeviceAdapter());

  vtkm::cont::ArrayHandle<vtkm::Float32> cellvarIn;
  ds.GetField("cellvar").GetData().CopyTo(cellvarIn);
  vtkm::cont::ArrayHandle<vtkm::Float32> cellvar =
    clip.ProcessCellField(cellvarIn, DeviceAdapter());

  vtkm::Id connectivitySize = 12;
  vtkm::Id fieldSize = 7;
  vtkm::Id expectedConnectivity[] = { 5, 4, 0, 5, 0, 1, 5, 1, 6, 6, 1, 2 };
  Coord3D expectedCoords[] = {
    Coord3D(0.00f, 0.00f, 0.0f), Coord3D(1.00f, 0.00f, 0.0f), Coord3D(1.00f, 1.00f, 0.0f),
    Coord3D(0.00f, 1.00f, 0.0f), Coord3D(0.00f, 0.50f, 0.0f), Coord3D(0.25f, 0.75f, 0.0f),
    Coord3D(0.50f, 1.00f, 0.0f),
  };
  vtkm::Float32 expectedScalars[] = { 1, 2, 1, 0, 0.5, 0.5, 0.5 };
  std::vector<vtkm::Float32> expectedCellvar = { 100.f, 100.f, -100.f, -100.f };

  VTKM_TEST_ASSERT(outputCellSet.GetNumberOfPoints() == fieldSize,
                   "Wrong number of points in cell set.");

  VTKM_TEST_ASSERT(
    TestArrayHandle(outputCellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                       vtkm::TopologyElementTagCell()),
                    expectedConnectivity,
                    connectivitySize),
    "Got incorrect conectivity");

  VTKM_TEST_ASSERT(TestArrayHandle(coords, expectedCoords, fieldSize), "Got incorrect coordinates");

  VTKM_TEST_ASSERT(TestArrayHandle(scalars, expectedScalars, fieldSize), "Got incorrect scalars");

  VTKM_TEST_ASSERT(
    TestArrayHandle(cellvar, expectedCellvar.data(), static_cast<vtkm::Id>(expectedCellvar.size())),
    "Got incorrect cellvar");
}

template <typename DeviceAdapter>
void TestClippingStructured()
{
  using CoordsValueType = vtkm::cont::ArrayHandleUniformPointCoordinates::ValueType;
  using CoordsOutType = vtkm::cont::ArrayHandle<CoordsValueType>;

  vtkm::cont::DataSet ds = MakeTestDatasetStructured();

  bool invertClip = false;
  vtkm::worklet::Clip clip;
  vtkm::cont::CellSetExplicit<> outputCellSet = clip.Run(
    ds.GetCellSet(0), ds.GetField("scalars").GetData(), clipValue, invertClip, DeviceAdapter());

  auto coordsIn = ds.GetCoordinateSystem("coords").GetData();
  CoordsOutType coords = clip.ProcessPointField(coordsIn, DeviceAdapter());

  vtkm::cont::ArrayHandle<vtkm::Float32> scalarsIn;
  ds.GetField("scalars").GetData().CopyTo(scalarsIn);
  vtkm::cont::ArrayHandle<vtkm::Float32> scalars =
    clip.ProcessPointField(scalarsIn, DeviceAdapter());

  vtkm::cont::ArrayHandle<vtkm::Float32> cellvarIn;
  ds.GetField("cellvar").GetData().CopyTo(cellvarIn);
  vtkm::cont::ArrayHandle<vtkm::Float32> cellvar =
    clip.ProcessCellField(cellvarIn, DeviceAdapter());


  vtkm::Id connectivitySize = 36;
  vtkm::Id fieldSize = 13;
  vtkm::Id expectedConnectivity[] = { 0,  1,  9, 0,  9, 10, 0,  10, 3,  1,  2,  9,
                                      2,  11, 9, 2,  5, 11, 3,  10, 6,  10, 12, 6,
                                      12, 7,  6, 11, 5, 8,  11, 8,  12, 8,  7,  12 };
  Coord3D expectedCoords[] = {
    Coord3D(0.0f, 0.0f, 0.0f), Coord3D(1.0f, 0.0f, 0.0f), Coord3D(2.0f, 0.0f, 0.0f),
    Coord3D(0.0f, 1.0f, 0.0f), Coord3D(1.0f, 1.0f, 0.0f), Coord3D(2.0f, 1.0f, 0.0f),
    Coord3D(0.0f, 2.0f, 0.0f), Coord3D(1.0f, 2.0f, 0.0f), Coord3D(2.0f, 2.0f, 0.0f),
    Coord3D(1.0f, 0.5f, 0.0f), Coord3D(0.5f, 1.0f, 0.0f), Coord3D(1.5f, 1.0f, 0.0f),
    Coord3D(1.0f, 1.5f, 0.0f),
  };
  vtkm::Float32 expectedScalars[] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.5, 0.5, 0.5, 0.5 };
  std::vector<vtkm::Float32> expectedCellvar = { -100.f, -100.f, -100.f, 100.f, 100.f, 100.f,
                                                 30.f,   30.f,   30.f,   -30.f, -30.f, -30.f };

  VTKM_TEST_ASSERT(outputCellSet.GetNumberOfPoints() == fieldSize,
                   "Wrong number of points in cell set.");

  VTKM_TEST_ASSERT(
    TestArrayHandle(outputCellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                       vtkm::TopologyElementTagCell()),
                    expectedConnectivity,
                    connectivitySize),
    "Got incorrect conectivity");

  VTKM_TEST_ASSERT(TestArrayHandle(coords, expectedCoords, fieldSize), "Got incorrect coordinates");

  VTKM_TEST_ASSERT(TestArrayHandle(scalars, expectedScalars, fieldSize), "Got incorrect scalars");

  VTKM_TEST_ASSERT(
    TestArrayHandle(cellvar, expectedCellvar.data(), static_cast<vtkm::Id>(expectedCellvar.size())),
    "Got incorrect cellvar");
}

template <typename DeviceAdapter>
void TestClippingWithImplicitFunction()
{
  vtkm::Vec<vtkm::FloatDefault, 3> center(1, 1, 0);
  vtkm::FloatDefault radius(0.5);

  vtkm::cont::DataSet ds = MakeTestDatasetStructured();
  bool invertClip = false;
  vtkm::worklet::Clip clip;
  vtkm::cont::CellSetExplicit<> outputCellSet =
    clip.Run(ds.GetCellSet(0),
             vtkm::cont::make_ImplicitFunctionHandle<vtkm::Sphere>(center, radius),
             ds.GetCoordinateSystem("coords"),
             invertClip,
             DeviceAdapter());

  auto coordsIn = ds.GetCoordinateSystem("coords").GetData();
  vtkm::cont::ArrayHandle<Coord3D> coords = clip.ProcessPointField(coordsIn, DeviceAdapter());

  vtkm::cont::ArrayHandle<vtkm::Float32> scalarsIn;
  ds.GetField("scalars").GetData().CopyTo(scalarsIn);
  vtkm::cont::ArrayHandle<vtkm::Float32> scalars =
    clip.ProcessPointField(scalarsIn, DeviceAdapter());

  vtkm::cont::ArrayHandle<vtkm::Float32> cellvarIn;
  ds.GetField("cellvar").GetData().CopyTo(cellvarIn);
  vtkm::cont::ArrayHandle<vtkm::Float32> cellvar =
    clip.ProcessCellField(cellvarIn, DeviceAdapter());

  vtkm::Id connectivitySize = 36;
  vtkm::Id fieldSize = 13;
  vtkm::Id expectedConnectivity[] = { 0,  1,  9, 0,  9, 10, 0,  10, 3,  1,  2,  9,
                                      2,  11, 9, 2,  5, 11, 3,  10, 6,  10, 12, 6,
                                      12, 7,  6, 11, 5, 8,  11, 8,  12, 8,  7,  12 };
  Coord3D expectedCoords[] = {
    Coord3D(0.0f, 0.0f, 0.0f),  Coord3D(1.0f, 0.0f, 0.0f),  Coord3D(2.0f, 0.0f, 0.0f),
    Coord3D(0.0f, 1.0f, 0.0f),  Coord3D(1.0f, 1.0f, 0.0f),  Coord3D(2.0f, 1.0f, 0.0f),
    Coord3D(0.0f, 2.0f, 0.0f),  Coord3D(1.0f, 2.0f, 0.0f),  Coord3D(2.0f, 2.0f, 0.0f),
    Coord3D(1.0f, 0.75f, 0.0f), Coord3D(0.75f, 1.0f, 0.0f), Coord3D(1.25f, 1.0f, 0.0f),
    Coord3D(1.0f, 1.25f, 0.0f),
  };
  vtkm::Float32 expectedScalars[] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  std::vector<vtkm::Float32> expectedCellvar = { -100.f, -100.f, -100.f, 100.f, 100.f, 100.f,
                                                 30.f,   30.f,   30.f,   -30.f, -30.f, -30.f };


  VTKM_TEST_ASSERT(
    TestArrayHandle(outputCellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                       vtkm::TopologyElementTagCell()),
                    expectedConnectivity,
                    connectivitySize),
    "Got incorrect conectivity");

  VTKM_TEST_ASSERT(TestArrayHandle(coords, expectedCoords, fieldSize), "Got incorrect coordinates");

  VTKM_TEST_ASSERT(TestArrayHandle(scalars, expectedScalars, fieldSize), "Got incorrect scalars");

  VTKM_TEST_ASSERT(
    TestArrayHandle(cellvar, expectedCellvar.data(), static_cast<vtkm::Id>(expectedCellvar.size())),
    "Got incorrect cellvar");
}

template <typename DeviceAdapter>
void TestClippingWithImplicitFunctionInverted()
{
  vtkm::Vec<vtkm::FloatDefault, 3> center(1, 1, 0);
  vtkm::FloatDefault radius(0.5);

  vtkm::cont::DataSet ds = MakeTestDatasetStructured();
  bool invertClip = true;
  vtkm::worklet::Clip clip;
  vtkm::cont::CellSetExplicit<> outputCellSet =
    clip.Run(ds.GetCellSet(0),
             vtkm::cont::make_ImplicitFunctionHandle<vtkm::Sphere>(center, radius),
             ds.GetCoordinateSystem("coords"),
             invertClip,
             DeviceAdapter());

  auto coordsIn = ds.GetCoordinateSystem("coords").GetData();
  vtkm::cont::ArrayHandle<Coord3D> coords = clip.ProcessPointField(coordsIn, DeviceAdapter());

  vtkm::cont::ArrayHandle<vtkm::Float32> scalarsIn;
  ds.GetField("scalars").GetData().CopyTo(scalarsIn);
  vtkm::cont::ArrayHandle<vtkm::Float32> scalars =
    clip.ProcessPointField(scalarsIn, DeviceAdapter());

  vtkm::cont::ArrayHandle<vtkm::Float32> cellvarIn;
  ds.GetField("cellvar").GetData().CopyTo(cellvarIn);
  vtkm::cont::ArrayHandle<vtkm::Float32> cellvar =
    clip.ProcessCellField(cellvarIn, DeviceAdapter());

  vtkm::Id connectivitySize = 12;
  vtkm::Id fieldSize = 13;
  vtkm::Id expectedConnectivity[] = { 4, 10, 9, 4, 9, 11, 4, 12, 10, 4, 11, 12 };
  Coord3D expectedCoords[] = {
    Coord3D(0.0f, 0.0f, 0.0f),  Coord3D(1.0f, 0.0f, 0.0f),  Coord3D(2.0f, 0.0f, 0.0f),
    Coord3D(0.0f, 1.0f, 0.0f),  Coord3D(1.0f, 1.0f, 0.0f),  Coord3D(2.0f, 1.0f, 0.0f),
    Coord3D(0.0f, 2.0f, 0.0f),  Coord3D(1.0f, 2.0f, 0.0f),  Coord3D(2.0f, 2.0f, 0.0f),
    Coord3D(1.0f, 0.75f, 0.0f), Coord3D(0.75f, 1.0f, 0.0f), Coord3D(1.25f, 1.0f, 0.0f),
    Coord3D(1.0f, 1.25f, 0.0f),
  };
  vtkm::Float32 expectedScalars[] = { 1, 1, 1, 1, 0, 1, 1, 1, 1, 0.25, 0.25, 0.25, 0.25 };
  std::vector<vtkm::Float32> expectedCellvar = { -100.f, 100.f, 30.f, -30.f };

  VTKM_TEST_ASSERT(
    TestArrayHandle(outputCellSet.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                                       vtkm::TopologyElementTagCell()),
                    expectedConnectivity,
                    connectivitySize),
    "Got incorrect conectivity");

  VTKM_TEST_ASSERT(TestArrayHandle(coords, expectedCoords, fieldSize), "Got incorrect coordinates");

  VTKM_TEST_ASSERT(TestArrayHandle(scalars, expectedScalars, fieldSize), "Got incorrect scalars");

  VTKM_TEST_ASSERT(
    TestArrayHandle(cellvar, expectedCellvar.data(), static_cast<vtkm::Id>(expectedCellvar.size())),
    "Got incorrect cellvar");
}

template <typename DeviceAdapter>
void TestClipping()
{
  std::cout << "Testing explicit dataset:" << std::endl;
  TestClippingExplicit<DeviceAdapter>();
  std::cout << "Testing structured dataset:" << std::endl;
  TestClippingStructured<DeviceAdapter>();
  std::cout << "Testing clipping with implicit function (sphere):" << std::endl;
  TestClippingWithImplicitFunction<DeviceAdapter>();
  TestClippingWithImplicitFunctionInverted<DeviceAdapter>();
}

int UnitTestClipping(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestClipping<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>);
}
