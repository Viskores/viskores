//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperConnectivity.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

// Multi-mapper rendering is something of a hack right now. A view only supports one mapper
// at a time, so to use different mapper types you have to render the mappers yourself.
template <typename MapperType1, typename MapperType2>
void MultiMapperRender(const vtkm::cont::DataSet& ds1,
                       const vtkm::cont::DataSet& ds2,
                       const std::string& fieldNm,
                       const vtkm::cont::ColorTable& colorTable1,
                       const vtkm::cont::ColorTable& colorTable2,
                       const std::string& outputFile)
{
  MapperType1 mapper1;
  MapperType2 mapper2;

  vtkm::rendering::CanvasRayTracer canvas(300, 300);
  canvas.SetBackgroundColor(vtkm::rendering::Color(0.8f, 0.8f, 0.8f, 1.0f));
  canvas.Clear();

  vtkm::Bounds totalBounds =
    ds1.GetCoordinateSystem().GetBounds() + ds2.GetCoordinateSystem().GetBounds();
  vtkm::rendering::Camera camera;
  camera.ResetToBounds(totalBounds);
  camera.Azimuth(45.0f);
  camera.Elevation(45.0f);

  mapper1.SetCanvas(&canvas);
  mapper1.SetActiveColorTable(colorTable1);
  mapper1.SetCompositeBackground(false);

  mapper2.SetCanvas(&canvas);
  mapper2.SetActiveColorTable(colorTable2);

  const vtkm::cont::Field field1 = ds1.GetField(fieldNm);
  vtkm::Range range1;
  field1.GetRange(&range1);

  const vtkm::cont::Field field2 = ds2.GetField(fieldNm);
  vtkm::Range range2;
  field2.GetRange(&range2);

  mapper1.RenderCells(
    ds1.GetCellSet(), ds1.GetCoordinateSystem(), field1, colorTable1, camera, range1);

  mapper2.RenderCells(
    ds2.GetCellSet(), ds2.GetCoordinateSystem(), field2, colorTable2, camera, range2);

  VTKM_TEST_ASSERT(test_equal_images(canvas, outputFile));
}

void RenderTests()
{
  using M1 = vtkm::rendering::MapperVolume;
  using M2 = vtkm::rendering::MapperConnectivity;
  using R = vtkm::rendering::MapperRayTracer;

  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::ColorTable colorTable("inferno");


  vtkm::cont::ColorTable colorTable2("cool to warm");
  colorTable2.AddPointAlpha(0.0, .02f);
  colorTable2.AddPointAlpha(1.0, .02f);

  MultiMapperRender<R, M2>(maker.Make3DExplicitDataSetPolygonal(),
                           maker.Make3DRectilinearDataSet0(),
                           "pointvar",
                           colorTable,
                           colorTable2,
                           "rendering/multimapper/raytracer-connectivity.png");

  MultiMapperRender<R, M1>(maker.Make3DExplicitDataSet4(),
                           maker.Make3DRectilinearDataSet0(),
                           "pointvar",
                           colorTable,
                           colorTable2,
                           "rendering/multimapper/raytracer-volume.png");
}

} //namespace

int UnitTestMultiMapper(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
