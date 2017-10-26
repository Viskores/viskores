//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperWireframer.h>
#include <vtkm/rendering/testing/RenderTest.h>

namespace
{

vtkm::cont::DataSet Make3DUniformDataSet(vtkm::Id size = 64)
{
  vtkm::Float32 center = static_cast<vtkm::Float32>(-size) / 2.0f;
  vtkm::cont::DataSetBuilderUniform builder;
  vtkm::cont::DataSet dataSet = builder.Create(vtkm::Id3(size, size, size),
                                               vtkm::Vec<vtkm::Float32, 3>(center, center, center),
                                               vtkm::Vec<vtkm::Float32, 3>(1.0f, 1.0f, 1.0f));
  const char* fieldName = "pointvar";
  vtkm::Id numValues = dataSet.GetCoordinateSystem().GetData().GetNumberOfValues();
  vtkm::cont::ArrayHandleCounting<vtkm::Float32> fieldValues(
    0.0f, 10.0f / static_cast<vtkm::Float32>(numValues), numValues);
  vtkm::cont::ArrayHandle<vtkm::Float32> scalarField;
  vtkm::cont::ArrayCopy(fieldValues, scalarField);
  vtkm::cont::DataSetFieldAdd().AddPointField(dataSet, fieldName, scalarField);
  return dataSet;
}

vtkm::cont::DataSet Make2DExplicitDataSet()
{
  vtkm::cont::DataSet dataSet;
  vtkm::cont::DataSetBuilderExplicit dsb;
  const int nVerts = 5;
  using CoordType = vtkm::Vec<vtkm::Float32, 3>;
  std::vector<CoordType> coords(nVerts);
  CoordType coordinates[nVerts] = { CoordType(0.f, 0.f, 0.f),
                                    CoordType(1.f, .5f, 0.f),
                                    CoordType(2.f, 1.f, 0.f),
                                    CoordType(3.f, 1.7f, 0.f),
                                    CoordType(4.f, 3.f, 0.f) };

  std::vector<vtkm::Float32> cellVar;
  cellVar.push_back(10);
  cellVar.push_back(12);
  cellVar.push_back(13);
  cellVar.push_back(14);
  std::vector<vtkm::Float32> pointVar;
  pointVar.push_back(10);
  pointVar.push_back(12);
  pointVar.push_back(13);
  pointVar.push_back(14);
  pointVar.push_back(15);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates, nVerts));
  vtkm::cont::CellSetSingleType<> cellSet("cells");

  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  connectivity.Allocate(8);
  auto connPortal = connectivity.GetPortalControl();
  connPortal.Set(0, 0);
  connPortal.Set(1, 1);

  connPortal.Set(2, 1);
  connPortal.Set(3, 2);

  connPortal.Set(4, 2);
  connPortal.Set(5, 3);

  connPortal.Set(6, 3);
  connPortal.Set(7, 4);

  cellSet.Fill(nVerts, vtkm::CELL_SHAPE_LINE, 2, connectivity);
  dataSet.AddCellSet(cellSet);
  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointVar", pointVar);
  dsf.AddCellField(dataSet, "cellVar", cellVar);

  return dataSet;
}

void RenderTests()
{
  typedef vtkm::rendering::MapperWireframer M;
  typedef vtkm::rendering::CanvasRayTracer C;
  typedef vtkm::rendering::View3D V3;
  typedef vtkm::rendering::View2D V2;
  typedef vtkm::rendering::View1D V1;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::rendering::ColorTable colorTable("thermal");

  //vtkm::rendering::testing::Render<M, C, V3>(
  //  maker.Make3DRegularDataSet0(), "pointvar", colorTable, "reg3D.pnm");
  //vtkm::rendering::testing::Render<M, C, V3>(
  //  maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "rect3D.pnm");
  //vtkm::rendering::testing::Render<M, C, V3>(
  //  maker.Make3DExplicitDataSet4(), "pointvar", colorTable, "expl3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    Make3DUniformDataSet(), "pointvar", colorTable, "uniform3D.pnm");
  vtkm::rendering::testing::Render<M, C, V2>(
    Make2DExplicitDataSet(), "cellVar", colorTable, "lines2D.pnm");
  //
  // Test the 1D cell set line plot with multiple lines
  //
  std::vector<std::string> fields;
  fields.push_back("pointvar");
  fields.push_back("pointvar2");
  std::vector<vtkm::rendering::Color> colors;
  colors.push_back(vtkm::rendering::Color(1.f, 0.f, 0.f));
  colors.push_back(vtkm::rendering::Color(0.f, 1.f, 0.f));
  vtkm::rendering::testing::Render<M, C, V1>(
    maker.Make1DUniformDataSet0(), fields, colors, "lines1D.pnm");
}

} //namespace

int UnitTestMapperWireframer(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(RenderTests);
}
