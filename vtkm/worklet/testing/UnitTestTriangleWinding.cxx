//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#include <vtkm/worklet/TriangleWinding.h>

#include <vtkm/Types.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

using MyNormalT = vtkm::Vec<vtkm::Float32, 3>;

namespace
{

vtkm::cont::DataSet GenerateDataSet()
{
  auto ds = vtkm::cont::testing::MakeTestDataSet{}.Make3DExplicitDataSetPolygonal();
  const auto numCells = ds.GetNumberOfCells();

  vtkm::cont::ArrayHandle<MyNormalT> cellNormals;
  vtkm::cont::Algorithm::Fill(cellNormals, MyNormalT{ 1., 0., 0. }, numCells);

  ds.AddField(vtkm::cont::make_FieldCell("normals", cellNormals));
  return ds;
}

void Validate(vtkm::cont::DataSet dataSet)
{
  const auto cellSet = dataSet.GetCellSet().Cast<vtkm::cont::CellSetExplicit<>>();
  const auto coordsArray = dataSet.GetCoordinateSystem().GetData();
  const auto conn =
    cellSet.GetConnectivityArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
  const auto offsets =
    cellSet.GetOffsetsArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{});
  const auto offsetsTrim =
    vtkm::cont::make_ArrayHandleView(offsets, 0, offsets.GetNumberOfValues() - 1);
  const auto cellArray = vtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsetsTrim);
  const auto cellNormalsVar = dataSet.GetCellField("normals").GetData();
  const auto cellNormalsArray = cellNormalsVar.Cast<vtkm::cont::ArrayHandle<MyNormalT>>();

  const auto cellPortal = cellArray.GetPortalConstControl();
  const auto cellNormals = cellNormalsArray.GetPortalConstControl();
  const auto coords = coordsArray.GetPortalConstControl();

  const auto numCells = cellPortal.GetNumberOfValues();
  VTKM_TEST_ASSERT(numCells == cellNormals.GetNumberOfValues());

  for (vtkm::Id cellId = 0; cellId < numCells; ++cellId)
  {
    const auto cell = cellPortal.Get(cellId);
    if (cell.GetNumberOfComponents() != 3)
    { // Triangles only!
      continue;
    }

    const MyNormalT cellNormal = cellNormals.Get(cellId);
    const MyNormalT p0 = coords.Get(cell[0]);
    const MyNormalT p1 = coords.Get(cell[1]);
    const MyNormalT p2 = coords.Get(cell[2]);
    const MyNormalT v01 = p1 - p0;
    const MyNormalT v02 = p2 - p0;
    const MyNormalT triangleNormal = vtkm::Cross(v01, v02);
    VTKM_TEST_ASSERT(vtkm::Dot(triangleNormal, cellNormal) > 0,
                     "Triangle at index ",
                     cellId,
                     " incorrectly wound.");
  }
}

void DoTest()
{
  auto ds = GenerateDataSet();

  // Ensure that the test dataset needs to be rewound:
  bool threw = false;
  try
  {
    std::cerr << "Expecting an exception...\n";
    Validate(ds);
  }
  catch (...)
  {
    threw = true;
  }

  VTKM_TEST_ASSERT(threw, "Test dataset is already wound consistently wrt normals.");

  auto cellSet = ds.GetCellSet().Cast<vtkm::cont::CellSetExplicit<>>();
  const auto coords = ds.GetCoordinateSystem().GetData();
  const auto cellNormalsVar = ds.GetCellField("normals").GetData();
  const auto cellNormals = cellNormalsVar.Cast<vtkm::cont::ArrayHandle<MyNormalT>>();


  auto newCells = vtkm::worklet::TriangleWinding::Run(cellSet, coords, cellNormals);

  vtkm::cont::DataSet result;
  result.AddCoordinateSystem(ds.GetCoordinateSystem());
  result.SetCellSet(newCells);
  for (vtkm::Id i = 0; i < ds.GetNumberOfFields(); ++i)
  {
    result.AddField(ds.GetField(i));
  }

  Validate(result);
}

} // end anon namespace

int UnitTestTriangleWinding(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
