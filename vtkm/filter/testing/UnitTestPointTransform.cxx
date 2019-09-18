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

  std::vector<vtkm::Vec3f> coordinates;
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

void ValidatePointTransform(const vtkm::cont::CoordinateSystem& coords,
                            const std::string fieldName,
                            const vtkm::cont::DataSet& result,
                            const vtkm::Matrix<vtkm::FloatDefault, 4, 4>& matrix)
{
  //verify the result
  VTKM_TEST_ASSERT(result.HasField(fieldName, vtkm::cont::Field::Association::POINTS),
                   "Output field missing.");

  vtkm::cont::ArrayHandle<vtkm::Vec3f> resultArrayHandle;
  result.GetField(fieldName, vtkm::cont::Field::Association::POINTS)
    .GetData()
    .CopyTo(resultArrayHandle);

  vtkm::cont::ArrayHandleVirtualCoordinates outPointsArrayHandle =
    result.GetCoordinateSystem().GetData();

  auto points = coords.GetData();
  VTKM_TEST_ASSERT(points.GetNumberOfValues() == resultArrayHandle.GetNumberOfValues(),
                   "Incorrect number of points in point transform");

  auto pointsPortal = points.GetPortalConstControl();
  auto resultsPortal = resultArrayHandle.GetPortalConstControl();
  auto outPointsPortal = outPointsArrayHandle.GetPortalConstControl();

  for (vtkm::Id i = 0; i < points.GetNumberOfValues(); i++)
  {
    VTKM_TEST_ASSERT(
      test_equal(resultsPortal.Get(i), vtkm::Transform3DPoint(matrix, pointsPortal.Get(i))),
      "Wrong result for PointTransform worklet");
    VTKM_TEST_ASSERT(
      test_equal(outPointsPortal.Get(i), vtkm::Transform3DPoint(matrix, pointsPortal.Get(i))),
      "Wrong result for PointTransform worklet");
  }
}


void TestPointTransformTranslation(const vtkm::cont::DataSet& ds, const vtkm::Vec3f& trans)
{
  vtkm::filter::PointTransform filter;

  filter.SetOutputFieldName("translation");
  filter.SetTranslation(trans);
  vtkm::cont::DataSet result = filter.Execute(ds);

  ValidatePointTransform(
    ds.GetCoordinateSystem(), "translation", result, Transform3DTranslate(trans));
}

void TestPointTransformScale(const vtkm::cont::DataSet& ds, const vtkm::Vec3f& scale)
{
  vtkm::filter::PointTransform filter;

  filter.SetOutputFieldName("scale");
  filter.SetScale(scale);
  vtkm::cont::DataSet result = filter.Execute(ds);

  ValidatePointTransform(ds.GetCoordinateSystem(), "scale", result, Transform3DScale(scale));
}

void TestPointTransformRotation(const vtkm::cont::DataSet& ds,
                                const vtkm::FloatDefault& angle,
                                const vtkm::Vec3f& axis)
{
  vtkm::filter::PointTransform filter;

  filter.SetOutputFieldName("rotation");
  filter.SetRotation(angle, axis);
  vtkm::cont::DataSet result = filter.Execute(ds);

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
  TestPointTransformTranslation(ds, vtkm::Vec3f(0, 0, 0));
  TestPointTransformTranslation(ds, vtkm::Vec3f(1, 1, 1));
  TestPointTransformTranslation(ds, vtkm::Vec3f(-1, -1, -1));

  std::uniform_real_distribution<vtkm::FloatDefault> transDist(-100, 100);
  for (int i = 0; i < N; i++)
    TestPointTransformTranslation(
      ds,
      vtkm::Vec3f(transDist(randGenerator), transDist(randGenerator), transDist(randGenerator)));

  //Test scaling
  TestPointTransformScale(ds, vtkm::Vec3f(1, 1, 1));
  TestPointTransformScale(ds, vtkm::Vec3f(.23f, .23f, .23f));
  TestPointTransformScale(ds, vtkm::Vec3f(1, 2, 3));
  TestPointTransformScale(ds, vtkm::Vec3f(3.23f, 9.23f, 4.23f));

  std::uniform_real_distribution<vtkm::FloatDefault> scaleDist(0.0001f, 100);
  for (int i = 0; i < N; i++)
  {
    TestPointTransformScale(ds, vtkm::Vec3f(scaleDist(randGenerator)));
    TestPointTransformScale(
      ds,
      vtkm::Vec3f(scaleDist(randGenerator), scaleDist(randGenerator), scaleDist(randGenerator)));
  }

  //Test rotation
  std::vector<vtkm::FloatDefault> angles;
  std::uniform_real_distribution<vtkm::FloatDefault> angleDist(0, 360);
  for (int i = 0; i < N; i++)
    angles.push_back(angleDist(randGenerator));

  std::vector<vtkm::Vec3f> axes;
  axes.push_back(vtkm::Vec3f(1, 0, 0));
  axes.push_back(vtkm::Vec3f(0, 1, 0));
  axes.push_back(vtkm::Vec3f(0, 0, 1));
  axes.push_back(vtkm::Vec3f(1, 1, 1));
  axes.push_back(-axes[0]);
  axes.push_back(-axes[1]);
  axes.push_back(-axes[2]);
  axes.push_back(-axes[3]);

  std::uniform_real_distribution<vtkm::FloatDefault> axisDist(-1, 1);
  for (int i = 0; i < N; i++)
    axes.push_back(
      vtkm::Vec3f(axisDist(randGenerator), axisDist(randGenerator), axisDist(randGenerator)));

  for (std::size_t i = 0; i < angles.size(); i++)
    for (std::size_t j = 0; j < axes.size(); j++)
      TestPointTransformRotation(ds, angles[i], axes[j]);
}


int UnitTestPointTransform(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestPointTransform, argc, argv);
}
