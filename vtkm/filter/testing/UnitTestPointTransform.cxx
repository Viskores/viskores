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
#include <vtkm/filter/PointTransform.h>

#include <random>
#include <string>
#include <vector>

namespace
{
std::mt19937 randGenerator;

vtkm::cont::DataSet MakePointTransformTestDataSet()
{
  vtkm::cont::DataSet dataSet;

  std::vector<vtkm::Vec<vtkm::FloatDefault, 3>> coordinates;
  const vtkm::Id dim = 5;
  for (vtkm::Id j = 0; j < dim; ++j)
  {
    vtkm::FloatDefault z =
      static_cast<vtkm::FloatDefault>(j) / static_cast<vtkm::FloatDefault>(dim - 1);
    for (vtkm::Id i = 0; i < dim; ++i)
    {
      vtkm::FloatDefault x =
        static_cast<vtkm::FloatDefault>(i) / static_cast<vtkm::FloatDefault>(dim - 1);
      vtkm::FloatDefault y = (x * x + z * z) / 2.0f;
      coordinates.push_back(vtkm::make_Vec(x, y, z));
    }
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

void ValidatePointTransform(const vtkm::cont::CoordinateSystem& coords,
                            const std::string fieldName,
                            const vtkm::cont::DataSet& result,
                            const vtkm::Matrix<vtkm::FloatDefault, 4, 4>& matrix)
{
  //verify the result
  VTKM_TEST_ASSERT(result.HasField(fieldName, vtkm::cont::Field::Association::POINTS),
                   "Output field missing.");

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>> resultArrayHandle;
  result.GetField(fieldName, vtkm::cont::Field::Association::POINTS)
    .GetData()
    .CopyTo(resultArrayHandle);

  auto points = coords.GetData();
  VTKM_TEST_ASSERT(points.GetNumberOfValues() == resultArrayHandle.GetNumberOfValues(),
                   "Incorrect number of points in point transform");

  auto pointsPortal = points.GetPortalControl();
  auto resultsPortal = resultArrayHandle.GetPortalControl();

  for (vtkm::Id i = 0; i < points.GetNumberOfValues(); i++)
    VTKM_TEST_ASSERT(
      test_equal(resultsPortal.Get(i), vtkm::Transform3DPoint(matrix, pointsPortal.Get(i))),
      "Wrong result for PointTransform worklet");
}


void TestPointTransformTranslation(const vtkm::cont::DataSet& ds,
                                   const vtkm::Vec<vtkm::FloatDefault, 3>& trans)
{
  vtkm::filter::PointTransform<vtkm::FloatDefault> filter;

  filter.SetOutputFieldName("translation");
  filter.SetUseCoordinateSystemAsField(true);
  filter.SetTranslation(trans);
  auto result = filter.Execute(ds);

  ValidatePointTransform(
    ds.GetCoordinateSystem(), "translation", result, Transform3DTranslate(trans));
}

void TestPointTransformScale(const vtkm::cont::DataSet& ds,
                             const vtkm::Vec<vtkm::FloatDefault, 3>& scale)
{
  vtkm::filter::PointTransform<vtkm::FloatDefault> filter;

  filter.SetOutputFieldName("scale");
  filter.SetUseCoordinateSystemAsField(true);
  filter.SetScale(scale);
  auto result = filter.Execute(ds);

  ValidatePointTransform(ds.GetCoordinateSystem(), "scale", result, Transform3DScale(scale));
}

void TestPointTransformRotation(const vtkm::cont::DataSet& ds,
                                const vtkm::FloatDefault& angle,
                                const vtkm::Vec<vtkm::FloatDefault, 3>& axis)
{
  vtkm::filter::PointTransform<vtkm::FloatDefault> filter;

  filter.SetOutputFieldName("rotation");
  filter.SetUseCoordinateSystemAsField(true);
  filter.SetRotation(angle, axis);
  auto result = filter.Execute(ds);

  ValidatePointTransform(
    ds.GetCoordinateSystem(), "rotation", result, Transform3DRotate(angle, axis));
}
}

void TestPointTransform()
{
  std::cout << "Testing PointTransform Worklet" << std::endl;

  vtkm::cont::DataSet ds = MakePointTransformTestDataSet();
  int N = 41;

  //Test translation
  TestPointTransformTranslation(ds, vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 0));
  TestPointTransformTranslation(ds, vtkm::Vec<vtkm::FloatDefault, 3>(1, 1, 1));
  TestPointTransformTranslation(ds, vtkm::Vec<vtkm::FloatDefault, 3>(-1, -1, -1));

  std::uniform_real_distribution<vtkm::FloatDefault> transDist(-100, 100);
  for (int i = 0; i < N; i++)
    TestPointTransformTranslation(ds,
                                  vtkm::Vec<vtkm::FloatDefault, 3>(transDist(randGenerator),
                                                                   transDist(randGenerator),
                                                                   transDist(randGenerator)));

  //Test scaling
  TestPointTransformScale(ds, vtkm::Vec<vtkm::FloatDefault, 3>(1, 1, 1));
  TestPointTransformScale(ds, vtkm::Vec<vtkm::FloatDefault, 3>(.23f, .23f, .23f));
  TestPointTransformScale(ds, vtkm::Vec<vtkm::FloatDefault, 3>(1, 2, 3));
  TestPointTransformScale(ds, vtkm::Vec<vtkm::FloatDefault, 3>(3.23f, 9.23f, 4.23f));

  std::uniform_real_distribution<vtkm::FloatDefault> scaleDist(0.0001f, 100);
  for (int i = 0; i < N; i++)
  {
    TestPointTransformScale(ds, vtkm::Vec<vtkm::FloatDefault, 3>(scaleDist(randGenerator)));
    TestPointTransformScale(ds,
                            vtkm::Vec<vtkm::FloatDefault, 3>(scaleDist(randGenerator),
                                                             scaleDist(randGenerator),
                                                             scaleDist(randGenerator)));
  }

  //Test rotation
  std::vector<vtkm::FloatDefault> angles;
  std::uniform_real_distribution<vtkm::FloatDefault> angleDist(0, 360);
  for (int i = 0; i < N; i++)
    angles.push_back(angleDist(randGenerator));

  std::vector<vtkm::Vec<vtkm::FloatDefault, 3>> axes;
  axes.push_back(vtkm::Vec<vtkm::FloatDefault, 3>(1, 0, 0));
  axes.push_back(vtkm::Vec<vtkm::FloatDefault, 3>(0, 1, 0));
  axes.push_back(vtkm::Vec<vtkm::FloatDefault, 3>(0, 0, 1));
  axes.push_back(vtkm::Vec<vtkm::FloatDefault, 3>(1, 1, 1));
  axes.push_back(-axes[0]);
  axes.push_back(-axes[1]);
  axes.push_back(-axes[2]);
  axes.push_back(-axes[3]);

  std::uniform_real_distribution<vtkm::FloatDefault> axisDist(-1, 1);
  for (int i = 0; i < N; i++)
    axes.push_back(vtkm::Vec<vtkm::FloatDefault, 3>(
      axisDist(randGenerator), axisDist(randGenerator), axisDist(randGenerator)));

  for (std::size_t i = 0; i < angles.size(); i++)
    for (std::size_t j = 0; j < axes.size(); j++)
      TestPointTransformRotation(ds, angles[i], axes[j]);
}


int UnitTestPointTransform(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPointTransform, argc, argv);
}
