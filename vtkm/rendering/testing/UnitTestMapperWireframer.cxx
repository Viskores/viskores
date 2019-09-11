//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
                                               vtkm::Vec3f_32(center, center, center),
                                               vtkm::Vec3f_32(1.0f, 1.0f, 1.0f));
  const char* fieldName = "pointvar";
  vtkm::Id numValues = dataSet.GetNumberOfPoints();
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
  using CoordType = vtkm::Vec3f_32;
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
  dataSet.AddCoordinateSystem(
    vtkm::cont::make_CoordinateSystem("coordinates", coordinates, nVerts, vtkm::CopyFlag::On));
  vtkm::cont::CellSetSingleType<> cellSet;

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
  dataSet.SetCellSet(cellSet);
  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(dataSet, "pointVar", pointVar);
  dsf.AddCellField(dataSet, "cellVar", cellVar);

  return dataSet;
}

void RenderTests()
{
  using M = vtkm::rendering::MapperWireframer;
  using C = vtkm::rendering::CanvasRayTracer;
  using V3 = vtkm::rendering::View3D;
  using V2 = vtkm::rendering::View2D;
  using V1 = vtkm::rendering::View1D;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::ColorTable colorTable("samsel fire");

  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRegularDataSet0(), "pointvar", colorTable, "wf_reg3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DRectilinearDataSet0(), "pointvar", colorTable, "wf_rect3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    maker.Make3DExplicitDataSet4(), "pointvar", colorTable, "wf_expl3D.pnm");
  vtkm::rendering::testing::Render<M, C, V3>(
    Make3DUniformDataSet(), "pointvar", colorTable, "wf_uniform3D.pnm");
  vtkm::rendering::testing::Render<M, C, V2>(
    Make2DExplicitDataSet(), "cellVar", colorTable, "wf_lines2D.pnm");
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
    maker.Make1DUniformDataSet0(), fields, colors, "wf_lines1D.pnm");
  //test log y
  vtkm::rendering::Color red = vtkm::rendering::Color::red;
  vtkm::rendering::testing::Render<M, C, V1>(
    maker.Make1DUniformDataSet1(), "pointvar", red, "wf_linesLogY1D.pnm", true);
}

} //namespace

int UnitTestMapperWireframer(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
