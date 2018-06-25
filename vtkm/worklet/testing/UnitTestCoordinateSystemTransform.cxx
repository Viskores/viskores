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

#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/CoordinateSystemTransform.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <random>
#include <vector>

namespace
{
std::mt19937 randGenerator;

enum CoordinateType
{
  CART = 0,
  CYL,
  SPH
};

vtkm::cont::DataSet MakeTestDataSet(const CoordinateType& cType)
{
  vtkm::cont::DataSet dataSet;

  std::vector<vtkm::Vec<vtkm::FloatDefault, 3>> coordinates;
  const vtkm::Id dim = 5;
  if (cType == CART)
  {
    for (vtkm::Id j = 0; j < dim; ++j)
    {
      vtkm::FloatDefault z =
        static_cast<vtkm::FloatDefault>(j) / static_cast<vtkm::FloatDefault>(dim - 1);
      for (vtkm::Id i = 0; i < dim; ++i)
      {
        vtkm::FloatDefault x =
          static_cast<vtkm::FloatDefault>(i) / static_cast<vtkm::FloatDefault>(dim - 1);
        vtkm::FloatDefault y = (x * x + z * z) / 2.0f;
        coordinates.push_back(vtkm::make_Vec(x + 0, y + 0, z + 0));
      }
    }
  }
  else if (cType == CYL)
  {
    vtkm::FloatDefault R = 1.0f;
    for (vtkm::Id j = 0; j < dim; j++)
    {
      vtkm::FloatDefault Z =
        static_cast<vtkm::FloatDefault>(j) / static_cast<vtkm::FloatDefault>(dim - 1);
      for (vtkm::Id i = 0; i < dim; i++)
      {
        vtkm::FloatDefault Theta = vtkm::TwoPif() *
          (static_cast<vtkm::FloatDefault>(i) / static_cast<vtkm::FloatDefault>(dim - 1));
        coordinates.push_back(vtkm::make_Vec(R, Theta, Z));
      }
    }
  }
  else if (cType == SPH)
  {
    //Spherical coordinates have some degenerate cases, so provide some good cases.
    vtkm::FloatDefault R = 1.0f;
    vtkm::FloatDefault eps = vtkm::Epsilon<float>();
    std::vector<vtkm::FloatDefault> Thetas = {
      eps, vtkm::Pif() / 4, vtkm::Pif() / 3, vtkm::Pif() / 2, vtkm::Pif() - eps
    };
    std::vector<vtkm::FloatDefault> Phis = {
      eps, vtkm::TwoPif() / 4, vtkm::TwoPif() / 3, vtkm::TwoPif() / 2, vtkm::TwoPif() - eps
    };
    for (std::size_t i = 0; i < Thetas.size(); i++)
      for (std::size_t j = 0; j < Phis.size(); j++)
        coordinates.push_back(vtkm::make_Vec(R, Thetas[i], Phis[j]));
  }

  vtkm::Id numCells = (dim - 1) * (dim - 1);
  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, vtkm::CopyFlag::On));

  vtkm::cont::CellSetExplicit<> cellSet("cells");
  cellSet.PrepareToAddCells(numCells, numCells * 4);
  for (vtkm::Id j = 0; j < dim - 1; ++j)
  {
    for (vtkm::Id i = 0; i < dim - 1; ++i)
    {
      cellSet.AddCell(vtkm::CELL_SHAPE_QUAD,
                      4,
                      vtkm::make_Vec<vtkm::Id>(
                        j * dim + i, j * dim + i + 1, (j + 1) * dim + i + 1, (j + 1) * dim + i));
    }
  }
  cellSet.CompleteAddingCells(vtkm::Id(coordinates.size()));

  dataSet.AddCellSet(cellSet);
  return dataSet;
}

void ValidateCoordTransform(
  const vtkm::cont::CoordinateSystem& coords,
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& transform,
  const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>>& doubleTransform,
  const std::vector<bool>& isAngle)
{
  auto points = coords.GetData();
  VTKM_TEST_ASSERT(points.GetNumberOfValues() == transform.GetNumberOfValues() &&
                     points.GetNumberOfValues() == doubleTransform.GetNumberOfValues(),
                   "Incorrect number of points in point transform");

  //The double transform should produce the same result.
  auto pointsPortal = points.GetPortalConstControl();
  auto resultsPortal = doubleTransform.GetPortalConstControl();

  for (vtkm::Id i = 0; i < points.GetNumberOfValues(); i++)
  {
    vtkm::Vec<vtkm::FloatDefault, 3> p = pointsPortal.Get(i);
    vtkm::Vec<vtkm::FloatDefault, 3> r = resultsPortal.Get(i);
    bool isEqual = true;
    for (vtkm::IdComponent j = 0; j < 3; j++)
    {
      if (isAngle[static_cast<std::size_t>(j)])
        isEqual &= (test_equal(p[j], r[j]) || test_equal(p[j] + vtkm::TwoPif(), r[j]) ||
                    test_equal(p[j], r[j] + vtkm::TwoPif()));
      else
        isEqual &= test_equal(p[j], r[j]);
    }
    VTKM_TEST_ASSERT(isEqual, "Wrong result for PointTransform worklet");
  }
}
}

void TestCoordinateSystemTransform()
{
  std::cout << "Testing CylindricalCoordinateTransform Worklet" << std::endl;

  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;

  //Test cartesian to cyl
  vtkm::cont::DataSet dsCart = MakeTestDataSet(CART);
  vtkm::worklet::CylindricalCoordinateTransform cylTrn;

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> carToCylPts;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> revResult;

  cylTrn.SetCartesianToCylindrical();
  cylTrn.Run(dsCart.GetCoordinateSystem(), carToCylPts, DeviceAdapter());

  cylTrn.SetCylindricalToCartesian();
  cylTrn.Run(carToCylPts, revResult, DeviceAdapter());
  ValidateCoordTransform(
    dsCart.GetCoordinateSystem(), carToCylPts, revResult, { false, false, false });

  //Test cylindrical to cartesian
  vtkm::cont::DataSet dsCyl = MakeTestDataSet(CYL);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> cylToCarPts;
  cylTrn.SetCylindricalToCartesian();
  cylTrn.Run(dsCyl.GetCoordinateSystem(), cylToCarPts, DeviceAdapter());

  cylTrn.SetCartesianToCylindrical();
  cylTrn.Run(cylToCarPts, revResult, DeviceAdapter());
  ValidateCoordTransform(
    dsCyl.GetCoordinateSystem(), cylToCarPts, revResult, { false, true, false });

  //Spherical transform
  //Test cartesian to sph
  vtkm::worklet::SphericalCoordinateTransform sphTrn;
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> carToSphPts;

  sphTrn.SetCartesianToSpherical();
  sphTrn.Run(dsCart.GetCoordinateSystem(), carToSphPts, DeviceAdapter());

  sphTrn.SetSphericalToCartesian();
  sphTrn.Run(carToSphPts, revResult, DeviceAdapter());
  ValidateCoordTransform(
    dsCart.GetCoordinateSystem(), carToSphPts, revResult, { false, true, true });

  //Test spherical to cartesian
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> sphToCarPts;
  vtkm::cont::DataSet dsSph = MakeTestDataSet(SPH);

  sphTrn.SetSphericalToCartesian();
  sphTrn.Run(dsSph.GetCoordinateSystem(), sphToCarPts, DeviceAdapter());

  sphTrn.SetCartesianToSpherical();
  sphTrn.Run(sphToCarPts, revResult, DeviceAdapter());

  ValidateCoordTransform(
    dsSph.GetCoordinateSystem(), sphToCarPts, revResult, { false, true, true });
  sphTrn.SetSphericalToCartesian();
  sphTrn.Run(dsSph.GetCoordinateSystem(), sphToCarPts, DeviceAdapter());
  sphTrn.SetCartesianToSpherical();
  sphTrn.Run(sphToCarPts, revResult, DeviceAdapter());
  ValidateCoordTransform(
    dsSph.GetCoordinateSystem(), sphToCarPts, revResult, { false, true, true });
}

int UnitTestCoordinateSystemTransform(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestCoordinateSystemTransform);
}
