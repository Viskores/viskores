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
//Modified the content of the Make3DExplicitDataSetZoo() function
inline vtkm::cont::DataSet Make3DExplicitDataSet()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;

  using CoordType = vtkm::Vec3f_64;

  std::vector<CoordType> coords = {
    { 0.00, 0.00, 0.00 }, { 1.00, 0.00, 0.00 }, { 2.00, 0.00, 0.00 }, { 0.00, 0.00, 1.00 },
    { 1.00, 0.00, 1.00 }, { 2.00, 0.00, 1.00 }, { 0.00, 1.00, 0.00 }, { 1.00, 1.00, 0.00 },
    { 2.00, 1.00, 0.00 }, { 0.00, 1.00, 1.00 }, { 1.00, 1.00, 1.00 }, { 2.00, 1.00, 1.00 },
    { 0.00, 2.00, 0.00 }, { 1.00, 2.00, 0.00 }, { 2.00, 2.00, 0.00 }, { 0.00, 2.00, 1.00 },
    { 1.00, 2.00, 1.00 }, { 2.00, 2.00, 1.00 }, { 1.00, 3.00, 1.00 }, { 2.75, 0.00, 1.00 },
    { 3.00, 0.00, 0.75 }, { 3.00, 0.25, 1.00 }, { 3.00, 1.00, 1.00 }, { 3.00, 1.00, 0.00 },
    { 2.57, 2.00, 1.00 }, { 3.00, 1.75, 1.00 }, { 3.00, 1.75, 0.75 }, { 3.00, 0.00, 0.00 },
    { 2.57, 0.42, 0.57 }, { 2.59, 1.43, 0.71 }
  };

  std::vector<vtkm::UInt8> shapes;
  std::vector<vtkm::IdComponent> numindices;
  std::vector<vtkm::Id> conn;

  //Construct the shapes/cells of the dataset
  //This is a zoo of points, lines, polygons, and polyhedra
  shapes.push_back(vtkm::CELL_SHAPE_HEXAHEDRON);
  numindices.push_back(8);
  conn.push_back(0);
  conn.push_back(3);
  conn.push_back(4);
  conn.push_back(1);
  conn.push_back(6);
  conn.push_back(9);
  conn.push_back(10);
  conn.push_back(7);

  shapes.push_back(vtkm::CELL_SHAPE_HEXAHEDRON);
  numindices.push_back(8);
  conn.push_back(1);
  conn.push_back(4);
  conn.push_back(5);
  conn.push_back(2);
  conn.push_back(7);
  conn.push_back(10);
  conn.push_back(11);
  conn.push_back(8);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(24);
  conn.push_back(26);
  conn.push_back(25);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_TETRA);
  numindices.push_back(4);
  conn.push_back(8);
  conn.push_back(17);
  conn.push_back(11);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(24);
  conn.push_back(17);
  conn.push_back(8);
  conn.push_back(23);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_PYRAMID);
  numindices.push_back(5);
  conn.push_back(25);
  conn.push_back(22);
  conn.push_back(11);
  conn.push_back(17);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_WEDGE);
  numindices.push_back(6);
  conn.push_back(8);
  conn.push_back(14);
  conn.push_back(17);
  conn.push_back(7);
  conn.push_back(13);
  conn.push_back(16);

  shapes.push_back(vtkm::CELL_SHAPE_WEDGE);
  numindices.push_back(6);
  conn.push_back(11);
  conn.push_back(8);
  conn.push_back(17);
  conn.push_back(10);
  conn.push_back(7);
  conn.push_back(16);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(0);

  shapes.push_back(vtkm::CELL_SHAPE_VERTEX);
  numindices.push_back(1);
  conn.push_back(29);

  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(0);
  conn.push_back(1);

  shapes.push_back(vtkm::CELL_SHAPE_LINE);
  numindices.push_back(2);
  conn.push_back(15);
  conn.push_back(16);

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(2);
  conn.push_back(4);
  conn.push_back(15);

  shapes.push_back(vtkm::CELL_SHAPE_TRIANGLE);
  numindices.push_back(3);
  conn.push_back(5);
  conn.push_back(6);
  conn.push_back(7);

  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(0);
  conn.push_back(3);
  conn.push_back(5);
  conn.push_back(2);

  shapes.push_back(vtkm::CELL_SHAPE_QUAD);
  numindices.push_back(4);
  conn.push_back(5);
  conn.push_back(4);
  conn.push_back(10);
  conn.push_back(11);

  shapes.push_back(vtkm::CELL_SHAPE_POLYGON);
  numindices.push_back(3);
  conn.push_back(4);
  conn.push_back(7);
  conn.push_back(1);

  shapes.push_back(vtkm::CELL_SHAPE_POLYGON);
  numindices.push_back(4);
  conn.push_back(1);
  conn.push_back(6);
  conn.push_back(7);
  conn.push_back(2);

  dataSet = dsb.Create(coords, shapes, numindices, conn, "coordinates");

  return dataSet;
}

template <typename T>
std::vector<std::string> TestMeshQualityFilter(const vtkm::cont::DataSet& input,
                                               const std::vector<vtkm::FloatDefault>& expectedVals,
                                               T filter)
{
  fprintf(stderr, "In TMQF\n");
  std::vector<std::string> errors;
  vtkm::cont::DataSet output;
  try
  {
    fprintf(stderr, "Try execute\n");
    output = filter.Execute(input);
    fprintf(stderr, "Complete execute\n");
  }
  catch (vtkm::cont::ErrorExecution&)
  {
    fprintf(stderr, "Catch error\n");
    errors.push_back("Error occured while executing filter. Exiting...");
    return errors;
  }

  //Test the computed metric values (for all cells) and expected metric
  //values for equality.
  const vtkm::Id numFields = output.GetNumberOfFields();
  vtkm::cont::testing::TestEqualResult result = vtkm::cont::testing::test_equal_ArrayHandles(
    vtkm::cont::make_ArrayHandle(expectedVals), output.GetField(numFields - 1).GetData());
  if (!result)
    result.PushMessage(std::string("Data doesn't match"));

  return result.GetMessages();
}

//Either an error occurred during execution of the
//filter, or mismatches exist between the expected output
//and the computed filter output.
void CheckForErrors(const std::vector<std::string>& messages)
{
  if (!messages.empty())
  {
    std::cout << "FAIL\n";
    for (std::string m : messages)
      std::cout << m << "\n";
  }
  else
    std::cout << "SUCCESS\n";
}

int TestMeshQuality()
{
  using FloatVec = std::vector<vtkm::FloatDefault>;
  using PairVec = std::vector<vtkm::Pair<vtkm::UInt8, vtkm::filter::CellMetric>>;
  using StringVec = std::vector<std::string>;
  using CharVec = std::vector<vtkm::UInt8>;
  using QualityFilter = vtkm::filter::MeshQuality;

  //Test variables
  vtkm::cont::DataSet input = Make3DExplicitDataSet();
  std::unique_ptr<QualityFilter> filter;
  std::string metricSuffix;
  StringVec fieldNames;
  CharVec testedShapes;
  PairVec shapeMetricPairs;
  FloatVec expectedValues;

  /***************************************************
   * Test 1: Volume metric
   ***************************************************/

  std::cout << "Testing MeshQuality filter: Volume metric"
            << "\n++++++++++++++++++++++++++++++++++++++++++++++++++\n";

  //The ground truth metric value for each cell in the input dataset.
  //These values are generated from VisIt using the equivalent pseudocolor
  //mesh quality metric.
  expectedValues = { 1, 1, 0.0100042, 0.0983333, 0.0732667, 0.0845833, -0.5, -0.5, 0,
                     0, 1, 1,         1.5,       0.7071068, 2,         1,    0.5,  1 };

  filter.reset(new QualityFilter(vtkm::filter::CellMetric::VOLUME));
  std::vector<std::string> errors =
    TestMeshQualityFilter<QualityFilter>(input, expectedValues, *filter);
  std::cout << "Volume metric test: ";
  CheckForErrors(errors);

  /***************************************************
   * Test 2: Diagonal Ratio metric
   ***************************************************/

  std::cout << "Testing MeshQuality filter: Diagonal Ratio metric"
            << "\n++++++++++++++++++++++++++++++++++++++++++++++++++\n";


  expectedValues = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 2.23607 };

  filter.reset(new QualityFilter(vtkm::filter::CellMetric::DIAGONAL_RATIO));
  errors = TestMeshQualityFilter<QualityFilter>(input, expectedValues, *filter);
  std::cout << "Diagonal ratio metric test: ";
  CheckForErrors(errors);

  /***************************************************
   * Test 3: Jacobian metric
   ***************************************************/

  std::cout << "Testing MeshQuality filter: Jacobian metric"
            << "\n++++++++++++++++++++++++++++++++++++++++++++++++++\n";


  expectedValues = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 2.23607 };

  filter.reset(new QualityFilter(vtkm::filter::CellMetric::JACOBIAN));
  errors = TestMeshQualityFilter<QualityFilter>(input, expectedValues, *filter);
  std::cout << "Jacobian metric test: ";
  CheckForErrors(errors);

#if 0
  /***************************************************
   * Test 5: Oddy metric
   ***************************************************/

  std::cout << "Testing MeshQuality filter: Oddy metric"
            << "\n++++++++++++++++++++++++++++++++++++++++++++++++++\n";

  testedShapes = {vtkm::CELL_SHAPE_HEXAHEDRON, vtkm::CELL_SHAPE_POLYGON,
                  vtkm::CELL_SHAPE_QUAD};

  shapeMetricPairs.clear();
  for (auto s : testedShapes)
    shapeMetricPairs.push_back(vtkm::make_Pair(s, vtkm::filter::CellMetric::ODDY));
  expectedValues = {0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 1.125, 0,
                    0, 2.5}; /*(all cells, volume value)*/

  filter.reset(new QualityFilter(shapeMetricPairs));
  errors = TestMeshQualityFilter<QualityFilter>(input, expectedValues, *filter);
  std::cout << "Oddy metric test: ";
  CheckForErrors(errors);

  /***************************************************
   * Test 4: Relative Size Squared metric
   ***************************************************/

  std::cout << "Testing MeshQuality filter: Relative Size Squared metric"
            << "\n++++++++++++++++++++++++++++++++++++++++++++++++++\n";

  testedShapes = {vtkm::CELL_SHAPE_HEXAHEDRON, vtkm::CELL_SHAPE_POLYGON,
                  vtkm::CELL_SHAPE_QUAD, vtkm::CELL_SHAPE_TRIANGLE,
                  vtkm::CELL_SHAPE_TETRA};

  shapeMetricPairs.clear();
  for (auto s : testedShapes)
    shapeMetricPairs.push_back(vtkm::make_Pair(s, vtkm::filter::CellMetric::RELATIVE_SIZE));
  expectedValues = {1, 1, 0.0341086, 0.303456, 0, 0, 0,
                    0, 0, 0, 0, 0, 0.361898, 0.614047, 1, 1,
                    0.307024, 1};

  filter.reset(new QualityFilter(shapeMetricPairs));
  errors = TestMeshQualityFilter<QualityFilter>(input, expectedValues, *filter);
  std::cout << "Relative Size Square metric test: ";
  CheckForErrors(errors);
#endif

  return 0;
}

int UnitTestMeshQualityFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestMeshQuality, argc, argv);
}
