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
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/PointTransform.h>

#include <random>
#include <vector>

namespace
{
std::mt19937 randGenerator;

vtkm::cont::DataSet MakePointTransformTestDataSet()
{
  vtkm::cont::DataSet dataSet;

  std::vector<vtkm::Vec<vtkm::Float32, 3>> coordinates;
  const vtkm::Id dim = 5;
  for (vtkm::Id j = 0; j < dim; ++j)
  {
    vtkm::Float32 z = static_cast<vtkm::Float32>(j) / static_cast<vtkm::Float32>(dim - 1);
    for (vtkm::Id i = 0; i < dim; ++i)
    {
      vtkm::Float32 x = static_cast<vtkm::Float32>(i) / static_cast<vtkm::Float32>(dim - 1);
      vtkm::Float32 y = (x * x + z * z) / 2.0f;
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
                            const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>>& result,
                            const vtkm::Matrix<vtkm::Float32, 4, 4>& matrix)
{
  auto points = coords.GetData();
  VTKM_TEST_ASSERT(points.GetNumberOfValues() == result.GetNumberOfValues(),
                   "Incorrect number of points in point transform");

  auto pointsPortal = points.GetPortalConstControl();
  auto resultsPortal = result.GetPortalConstControl();

  for (vtkm::Id i = 0; i < points.GetNumberOfValues(); i++)
    VTKM_TEST_ASSERT(
      test_equal(resultsPortal.Get(i), vtkm::Transform3DPoint(matrix, pointsPortal.Get(i))),
      "Wrong result for PointTransform worklet");
}


void TestPointTransformTranslation(const vtkm::cont::DataSet& ds,
                                   const vtkm::Vec<vtkm::Float32, 3>& trans)
{
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> result;
  vtkm::worklet::PointTransform<vtkm::Float32> worklet;

  worklet.SetTranslation(trans);
  vtkm::worklet::DispatcherMapField<vtkm::worklet::PointTransform<vtkm::Float32>> dispatcher(
    worklet);
  dispatcher.Invoke(ds.GetCoordinateSystem(), result);

  ValidatePointTransform(ds.GetCoordinateSystem(), result, Transform3DTranslate(trans));
}

void TestPointTransformScale(const vtkm::cont::DataSet& ds,
                             const vtkm::Vec<vtkm::Float32, 3>& scale)
{
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> result;
  vtkm::worklet::PointTransform<vtkm::Float32> worklet;

  worklet.SetScale(scale);
  vtkm::worklet::DispatcherMapField<vtkm::worklet::PointTransform<vtkm::Float32>> dispatcher(
    worklet);
  dispatcher.Invoke(ds.GetCoordinateSystem(), result);

  ValidatePointTransform(ds.GetCoordinateSystem(), result, Transform3DScale(scale));
}

void TestPointTransformRotation(const vtkm::cont::DataSet& ds,
                                const vtkm::Float32& angle,
                                const vtkm::Vec<vtkm::Float32, 3>& axis)
{
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> result;
  vtkm::worklet::PointTransform<vtkm::Float32> worklet;

  worklet.SetRotation(angle, axis);
  vtkm::worklet::DispatcherMapField<vtkm::worklet::PointTransform<vtkm::Float32>> dispatcher(
    worklet);
  dispatcher.Invoke(ds.GetCoordinateSystem(), result);

  ValidatePointTransform(ds.GetCoordinateSystem(), result, Transform3DRotate(angle, axis));
}
}

void TestPointTransform()
{
  std::cout << "Testing PointTransform Worklet" << std::endl;

  vtkm::cont::DataSet ds = MakePointTransformTestDataSet();
  int N = 41;

  //Test translation
  TestPointTransformTranslation(ds, vtkm::Vec<vtkm::Float32, 3>(0, 0, 0));
  TestPointTransformTranslation(ds, vtkm::Vec<vtkm::Float32, 3>(1, 1, 1));
  TestPointTransformTranslation(ds, vtkm::Vec<vtkm::Float32, 3>(-1, -1, -1));

  std::uniform_real_distribution<vtkm::Float32> transDist(-100, 100);
  for (int i = 0; i < N; i++)
    TestPointTransformTranslation(ds,
                                  vtkm::Vec<vtkm::Float32, 3>(transDist(randGenerator),
                                                              transDist(randGenerator),
                                                              transDist(randGenerator)));

  //Test scaling
  TestPointTransformScale(ds, vtkm::Vec<vtkm::Float32, 3>(1, 1, 1));
  TestPointTransformScale(ds, vtkm::Vec<vtkm::Float32, 3>(.23f, .23f, .23f));
  TestPointTransformScale(ds, vtkm::Vec<vtkm::Float32, 3>(1, 2, 3));
  TestPointTransformScale(ds, vtkm::Vec<vtkm::Float32, 3>(3.23f, 9.23f, 4.23f));

  std::uniform_real_distribution<vtkm::Float32> scaleDist(0.0001f, 100);
  for (int i = 0; i < N; i++)
  {
    TestPointTransformScale(ds, vtkm::Vec<vtkm::Float32, 3>(scaleDist(randGenerator)));
    TestPointTransformScale(ds,
                            vtkm::Vec<vtkm::Float32, 3>(scaleDist(randGenerator),
                                                        scaleDist(randGenerator),
                                                        scaleDist(randGenerator)));
  }

  //Test rotation
  std::vector<vtkm::Float32> angles;
  std::uniform_real_distribution<vtkm::Float32> angleDist(0, 360);
  for (int i = 0; i < N; i++)
    angles.push_back(angleDist(randGenerator));

  std::vector<vtkm::Vec<vtkm::Float32, 3>> axes;
  axes.push_back(vtkm::Vec<vtkm::Float32, 3>(1, 0, 0));
  axes.push_back(vtkm::Vec<vtkm::Float32, 3>(0, 1, 0));
  axes.push_back(vtkm::Vec<vtkm::Float32, 3>(0, 0, 1));
  axes.push_back(vtkm::Vec<vtkm::Float32, 3>(1, 1, 1));
  axes.push_back(-axes[0]);
  axes.push_back(-axes[1]);
  axes.push_back(-axes[2]);
  axes.push_back(-axes[3]);

  std::uniform_real_distribution<vtkm::Float32> axisDist(-1, 1);
  for (int i = 0; i < N; i++)
    axes.push_back(vtkm::Vec<vtkm::Float32, 3>(
      axisDist(randGenerator), axisDist(randGenerator), axisDist(randGenerator)));

  for (std::size_t i = 0; i < angles.size(); i++)
    for (std::size_t j = 0; j < axes.size(); j++)
      TestPointTransformRotation(ds, angles[i], axes[j]);
}

int UnitTestPointTransform(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestPointTransform);
}
