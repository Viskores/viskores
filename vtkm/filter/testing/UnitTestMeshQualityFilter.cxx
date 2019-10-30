//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <stdio.h>
#include <string>
#include <typeinfo>
#include <vector>
#include <vtkm/cont/ArrayRangeCompute.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/MeshQuality.h>
#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>

//Adapted from vtkm/cont/testing/MakeTestDataSet.h
//Modified the content of the MakeExplicitDataSetZoo() function
inline vtkm::cont::DataSet MakeExplicitDataSet()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;

  using CoordType = vtkm::Vec3f_64;

  std::vector<CoordType> coords = {
    { 0, 0, 0 },  { 3, 0, 0 },  { 2, 2, 0 },  { 4, 0, 0 },  { 7, 0, 0 },  { 7, 2, 0 },
    { 6, 2, 0 },  { 8, 0, 0 },  { 11, 0, 0 }, { 9, 2, 0 },  { 9, 1, 1 },  { 9, 3, 0 },
    { 11, 3, 0 }, { 11, 5, 0 }, { 9, 5, 0 },  { 10, 4, 1 }, { 12, 0, 0 }, { 12, 3, 0 },
    { 12, 2, 1 }, { 15, 0, 0 }, { 15, 3, 0 }, { 15, 1, 1 }, { 16, 0, 0 }, { 18, 0, 0 },
    { 18, 2, 0 }, { 16, 2, 0 }, { 17, 1, 1 }, { 19, 1, 1 }, { 19, 3, 1 }, { 17, 3, 1 }
  };

  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numindices;
  std::vector<vtkm::Id> conn;

  //Construct the shapes/cells of the dataset
  //This is a zoo of points, lines, polygons, and polyhedra
  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(0);
  conn.push_back(1);
  conn.push_back(2);

  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(3);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(6);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(7);
  conn.push_back(8);
  conn.push_back(9);
  conn.push_back(10);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(11);
  conn.push_back(12);
  conn.push_back(13);
  conn.push_back(14);
  conn.push_back(15);

  shapes.push_back(vtkm::CELL_SHAPE_WEDGE);
  numindices.push_back(6);
  conn.push_back(16);
  conn.push_back(17);
  conn.push_back(18);
  conn.push_back(19);
  conn.push_back(20);
  conn.push_back(21);

  shapes.push_back(vtkm::CELL_SHAPE_HEXAHEDRON);
  numindices.push_back(8);
  conn.push_back(22);
  conn.push_back(23);
  conn.push_back(24);
  conn.push_back(25);
  conn.push_back(26);
  conn.push_back(27);
  conn.push_back(28);
  conn.push_back(29);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  return dataSet;
}

bool TestMeshQualityFilter(const vtkm::cont::DataSet& input,
                           const std::vector<vtkm::FloatDefault>& expectedVals,
                           const std::string& outputname,
                           vtkm::filter::MeshQuality& filter)
{
  vtkm::cont::DataSet output;
  try
  {
    output = filter.Execute(input);
  }
  catch (vtkm::cont::ErrorExecution&)
  {
    return true;
  }

  //Test the computed metric values (for all cells) and expected metric
  //values for equality.
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> values;
  output.GetField(outputname).GetData().CopyTo(values);
  auto portal1 = values.GetPortalConstControl();
  if (portal1.GetNumberOfValues() != (vtkm::Id)expectedVals.size())
  {
    printf("Number of expected values for %s does not match.\n", outputname.c_str());
    return true;
  }

  std::vector<std::string> cellTypes = { "triangle", "quadrilateral", "tetrahedron",
                                         "pyramid",  "wedge",         "hexahedron" };
  bool anyFailures = false;
  for (unsigned long i = 0; i < expectedVals.size(); i++)
  {
    vtkm::Id id = (vtkm::Id)i;
    if (!test_equal(portal1.Get(id), expectedVals[i]))
    {
      printf("Metric \"%s\" for cell type \"%s\" does not match.  Expected %f and got %f\n",
             outputname.c_str(),
             cellTypes[i].c_str(),
             expectedVals[i],
             portal1.Get(id));
      anyFailures = true;
    }
  }
  return anyFailures;
}

int TestMeshQuality()
{
  using FloatVec = std::vector<vtkm::FloatDefault>;

  //Test variables
  vtkm::cont::DataSet input = MakeExplicitDataSet();

  int numFailures = 0;
  bool testFailed = false;

  std::vector<FloatVec> expectedValues;
  std::vector<vtkm::filter::CellMetric> metrics;
  std::vector<std::string> metricName;

  FloatVec volumeExpectedValues = { 0, 0, 1, (float)1.333333333, 4, 4 };
  expectedValues.push_back(volumeExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::VOLUME);
  metricName.push_back("volume");

  FloatVec areaExpectedValues = { 3, 4, 0, 0, 0, 0 };
  expectedValues.push_back(areaExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::AREA);
  metricName.push_back("area");

  FloatVec aspectRatioExpectedValues = { (float)1.164010, (float)1.118034, (float)1.648938, 0, 0,
                                         (float)1.1547 };
  expectedValues.push_back(aspectRatioExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::ASPECT_RATIO);
  metricName.push_back("aspectRatio");

  FloatVec aspectGammaExpectedValues = { 0, 0, (float)1.52012, 0, 0, 0 };
  expectedValues.push_back(aspectGammaExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::ASPECT_GAMMA);
  metricName.push_back("aspectGamma");

  FloatVec conditionExpectedValues = {
    (float)1.058475, 2.25, (float)1.354007, 0, 0, (float)1.563472
  };
  expectedValues.push_back(conditionExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::CONDITION);
  metricName.push_back("condition");

  FloatVec minAngleExpectedValues = { 45, 45, -1, -1, -1, -1 };
  expectedValues.push_back(minAngleExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::MIN_ANGLE);
  metricName.push_back("minAngle");

  FloatVec maxAngleExpectedValues = { (float)71.56505, 135, -1, -1, -1, -1 };
  expectedValues.push_back(maxAngleExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::MAX_ANGLE);
  metricName.push_back("maxAngle");

  FloatVec minDiagonalExpectedValues = { -1, -1, -1, -1, -1, (float)1.73205 };
  expectedValues.push_back(minDiagonalExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::MIN_DIAGONAL);
  metricName.push_back("minDiagonal");

  FloatVec maxDiagonalExpectedValues = { -1, -1, -1, -1, -1, (float)4.3589 };
  expectedValues.push_back(maxDiagonalExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::MAX_DIAGONAL);
  metricName.push_back("maxDiagonal");

  FloatVec jacobianExpectedValues = { 0, 2, 6, 0, 0, 4 };
  expectedValues.push_back(jacobianExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::JACOBIAN);
  metricName.push_back("jacobian");

  FloatVec scaledJacobianExpectedValues = {
    (float)0.816497, (float)0.707107, (float)0.408248, -2, -2, (float)0.57735
  };
  expectedValues.push_back(scaledJacobianExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::SCALED_JACOBIAN);
  metricName.push_back("scaledJacobian");

  FloatVec oddyExpectedValues = { -1, 8.125, -1, -1, -1, (float)2.62484 };
  expectedValues.push_back(oddyExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::ODDY);
  metricName.push_back("oddy");

  FloatVec diagonalRatioExpectedValues = { -1, (float)0.620174, -1, -1, -1, (float)0.397360 };
  expectedValues.push_back(diagonalRatioExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::DIAGONAL_RATIO);
  metricName.push_back("diagonalRatio");

  FloatVec shapeExpectedValues = { (float)0.944755, (float)0.444444, (float)0.756394, -1, -1,
                                   (float)0.68723 };
  expectedValues.push_back(shapeExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::SHAPE);
  metricName.push_back("shape");

  FloatVec shearExpectedValues = { (float)-1, (float).707107, -1, -1, -1, (float)0.57735 };
  expectedValues.push_back(shearExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::SHEAR);
  metricName.push_back("shear");

  FloatVec skewExpectedValues = { (float)-1, (float)0.447214, -1, -1, -1, (float)0.57735 };
  expectedValues.push_back(skewExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::SKEW);
  metricName.push_back("skew");

  FloatVec stretchExpectedValues = { -1, (float)0.392232, -1, -1, -1, (float)0.688247 };
  expectedValues.push_back(stretchExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::STRETCH);
  metricName.push_back("stretch");

  FloatVec taperExpectedValues = { -1, 0.5, -1, -1, -1, 0 };
  expectedValues.push_back(taperExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::TAPER);
  metricName.push_back("taper");

  FloatVec warpageExpectedValues = { -1, 1, -1, -1, -1, -1 };
  expectedValues.push_back(warpageExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::WARPAGE);
  metricName.push_back("warpage");

  FloatVec dimensionExpectedValues = { -1, -1, -1, -1, -1, (float)0.707107 };
  expectedValues.push_back(dimensionExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::DIMENSION);
  metricName.push_back("dimension");

  FloatVec relSizeExpectedValues = { (float)0.151235, (float)0.085069, (float)0.337149, -1, -1,
                                     (float)0.185378 };
  expectedValues.push_back(relSizeExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::RELATIVE_SIZE_SQUARED);
  metricName.push_back("relativeSizeSquared");

  FloatVec shapeAndSizeExpectedValues = { (float)0.142880, (float)0.037809, (float)0.255017, -1, -1,
                                          (float)0.127397 };
  expectedValues.push_back(shapeAndSizeExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::SHAPE_AND_SIZE);
  metricName.push_back("shapeAndSize");

  unsigned long numTests = (unsigned long)metrics.size();
  for (unsigned long i = 0; i < numTests; i++)
  {
    printf("Testing metric %s\n", metricName[i].c_str());
    vtkm::filter::MeshQuality filter(metrics[i]);
    testFailed = TestMeshQualityFilter(input, expectedValues[i], metricName[i], filter);
    if (testFailed)
    {
      numFailures++;
      printf("\ttest \"%s\" failed\n", metricName[i].c_str());
    }
    else
      printf("\t... passed\n");
  }

  if (numFailures > 0)
  {
    printf("Number of failed metrics is %d\n", numFailures);
    bool see_previous_messages = false; // this variable name plays well with macro
    VTKM_TEST_ASSERT(see_previous_messages, "Failure occurred during test");
  }
  return 0;
}

int UnitTestMeshQualityFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMeshQuality, argc, argv);
}
