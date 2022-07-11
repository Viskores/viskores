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
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
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
  dataSet.AddPointField(fieldName, scalarField);
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
  auto connPortal = connectivity.WritePortal();
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
  dataSet.AddPointField("pointVar", pointVar);
  dataSet.AddCellField("cellVar", cellVar);

  return dataSet;
}

void RenderTests()
{

  vtkm::cont::testing::MakeTestDataSet maker;

  {
    vtkm::rendering::testing::RenderTestOptions testOptions;
    testOptions.Mapper = vtkm::rendering::testing::MapperType::Wireframer;
    testOptions.Colors = { vtkm::rendering::Color::black };
    testOptions.AllowAnyDevice = false;

    vtkm::rendering::testing::RenderTest(
      maker.Make3DRegularDataSet0(), "pointvar", "rendering/wireframer/wf_reg3D.png", testOptions);
    vtkm::rendering::testing::RenderTest(maker.Make3DRectilinearDataSet0(),
                                         "pointvar",
                                         "rendering/wireframer/wf_rect3D.png",
                                         testOptions);
    testOptions.ViewDimension = 2;
    vtkm::rendering::testing::RenderTest(
      Make2DExplicitDataSet(), "cellVar", "rendering/wireframer/wf_lines2D.png", testOptions);
  }

  // These tests are very fickle on multiple machines and on different devices
  // Need to boost the maximum number of allowable error pixels manually
  {
    vtkm::rendering::testing::RenderTestOptions testOptions;
    testOptions.Mapper = vtkm::rendering::testing::MapperType::Wireframer;
    testOptions.Colors = { vtkm::rendering::Color::black };
    testOptions.AllowedPixelErrorRatio = 0.05f;
    testOptions.AllowAnyDevice = false;
    vtkm::rendering::testing::RenderTest(
      Make3DUniformDataSet(), "pointvar", "rendering/wireframer/wf_uniform3D.png", testOptions);
    vtkm::rendering::testing::RenderTest(maker.Make3DExplicitDataSet4(),
                                         "pointvar",
                                         "rendering/wireframer/wf_expl3D.png",
                                         testOptions);
  }

  //
  // Test the 1D cell set line plot with multiple lines
  //
  {
    vtkm::rendering::testing::RenderTestOptions testOptions;
    testOptions.ViewDimension = 1;
    testOptions.Mapper = vtkm::rendering::testing::MapperType::Wireframer;
    testOptions.Colors = { vtkm::rendering::Color::red, vtkm::rendering::Color::green };
    testOptions.AllowAnyDevice = false;

    vtkm::cont::DataSet dataSet0 = maker.Make1DUniformDataSet0();
    vtkm::rendering::testing::RenderTest({ { dataSet0, "pointvar" }, { dataSet0, "pointvar2" } },
                                         "rendering/wireframer/wf_lines1D.png",
                                         testOptions);

    //test log y and title
    testOptions.LogY = true;
    testOptions.Title = "1D Test Plot";
    vtkm::cont::DataSet dataSet1 = maker.Make1DUniformDataSet1();
    vtkm::rendering::testing::RenderTest(
      dataSet1, "pointvar", "rendering/wireframer/wf_linesLogY1D.png", testOptions);
  }
}

} //namespace

int UnitTestMapperWireframer(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
