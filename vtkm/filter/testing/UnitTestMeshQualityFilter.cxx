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
    if (portal1.Get(id) != expectedVals[i])
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

  /*
  FloatVec volumeExpectedValues = { 0, 0, 1, (float) 1.333333333, 4, 4 };
  expectedValues.push_back(volumeExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::VOLUME);
  metricName.push_back("volume");
 */

  FloatVec jacobianExpectedValues = { 0, 2, 6, 0, 0, 4 };
  expectedValues.push_back(jacobianExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::JACOBIAN);
  metricName.push_back("jacobian");

  /*
  FloatVec diagonalRatioExpectedValues = { -1, -1, -1, -1, -1, (float) 0.39 };
  expectedValues.push_back(diagonalRatioExpectedValues);
  metrics.push_back(vtkm::filter::CellMetric::DIAGONAL_RATIO);
  metricName.push_back("diagonalRatio");
 */

  unsigned long numTests = metrics.size();
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
