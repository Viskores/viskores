//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/CoordinateSystemTransform.h>

#include <string>
#include <vector>

namespace
{

enum CoordinateType
{
  CART = 0,
  CYL,
  SPH
};

vtkm::cont::DataSet MakeTestDataSet(const CoordinateType& cType)
{
  vtkm::cont::DataSet dataSet;

  std::vector<vtkm::Vec3f> coordinates;
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

  vtkm::cont::CellSetExplicit<> cellSet;
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

  dataSet.SetCellSet(cellSet);
  return dataSet;
}

void ValidateCoordTransform(const vtkm::cont::DataSet& ds,
                            const vtkm::cont::DataSet& dsTrn,
                            const std::vector<bool>& isAngle)
{
  auto points = ds.GetCoordinateSystem().GetData();
  auto pointsTrn = dsTrn.GetCoordinateSystem().GetData();
  VTKM_TEST_ASSERT(points.GetNumberOfValues() == pointsTrn.GetNumberOfValues(),
                   "Incorrect number of points in point transform");

  auto pointsPortal = points.GetPortalConstControl();
  auto pointsTrnPortal = pointsTrn.GetPortalConstControl();

  for (vtkm::Id i = 0; i < points.GetNumberOfValues(); i++)
  {
    vtkm::Vec3f p = pointsPortal.Get(i);
    vtkm::Vec3f r = pointsTrnPortal.Get(i);
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
  std::cout << "Testing CylindricalCoordinateTransform Filter" << std::endl;

  //Test cartesian to cyl
  vtkm::cont::DataSet dsCart = MakeTestDataSet(CART);
  vtkm::filter::CylindricalCoordinateTransform cylTrn;

  cylTrn.SetOutputFieldName("cylindricalCoords");
  cylTrn.SetUseCoordinateSystemAsField(true);
  cylTrn.SetCartesianToCylindrical();
  vtkm::cont::DataSet carToCylDataSet = cylTrn.Execute(dsCart);

  cylTrn.SetCylindricalToCartesian();
  cylTrn.SetUseCoordinateSystemAsField(true);
  cylTrn.SetOutputFieldName("cartesianCoords");
  vtkm::cont::DataSet cylToCarDataSet = cylTrn.Execute(carToCylDataSet);
  ValidateCoordTransform(dsCart, cylToCarDataSet, { false, false, false });

  //Test cyl to cart.
  vtkm::cont::DataSet dsCyl = MakeTestDataSet(CYL);
  cylTrn.SetCylindricalToCartesian();
  cylTrn.SetUseCoordinateSystemAsField(true);
  cylTrn.SetOutputFieldName("cartesianCoords");
  cylToCarDataSet = cylTrn.Execute(dsCyl);

  cylTrn.SetCartesianToCylindrical();
  cylTrn.SetUseCoordinateSystemAsField(true);
  cylTrn.SetOutputFieldName("cylindricalCoords");
  carToCylDataSet = cylTrn.Execute(cylToCarDataSet);
  ValidateCoordTransform(dsCyl, carToCylDataSet, { false, true, false });

  std::cout << "Testing SphericalCoordinateTransform Filter" << std::endl;

  vtkm::filter::SphericalCoordinateTransform sphTrn;
  sphTrn.SetOutputFieldName("sphericalCoords");
  sphTrn.SetUseCoordinateSystemAsField(true);
  sphTrn.SetCartesianToSpherical();
  vtkm::cont::DataSet carToSphDataSet = sphTrn.Execute(dsCart);

  sphTrn.SetOutputFieldName("cartesianCoords");
  sphTrn.SetUseCoordinateSystemAsField(true);
  sphTrn.SetSphericalToCartesian();
  vtkm::cont::DataSet sphToCarDataSet = sphTrn.Execute(carToSphDataSet);
  ValidateCoordTransform(dsCart, sphToCarDataSet, { false, true, true });

  vtkm::cont::DataSet dsSph = MakeTestDataSet(SPH);
  sphTrn.SetSphericalToCartesian();
  sphTrn.SetUseCoordinateSystemAsField(true);
  sphTrn.SetOutputFieldName("sphericalCoords");
  sphToCarDataSet = sphTrn.Execute(dsSph);

  sphTrn.SetCartesianToSpherical();
  sphTrn.SetUseCoordinateSystemAsField(true);
  sphTrn.SetOutputFieldName("sphericalCoords");
  carToSphDataSet = cylTrn.Execute(sphToCarDataSet);
  ValidateCoordTransform(dsSph, carToSphDataSet, { false, true, true });
}


int UnitTestCoordinateSystemTransform(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestCoordinateSystemTransform, argc, argv);
}
