//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/ScalarRenderer.h>
#include <vtkm/rendering/testing/RenderTest.h>
#include <vtkm/source/Tangle.h>

namespace
{

void RenderTests()
{
  vtkm::cont::testing::MakeTestDataSet maker;
  vtkm::cont::DataSet dataset = maker.Make3DRegularDataSet0();
  vtkm::Bounds bounds = dataset.GetCoordinateSystem().GetBounds();

  vtkm::rendering::Camera camera;
  camera.ResetToBounds(bounds);
  camera.Azimuth(-40.f);
  camera.Elevation(15.f);

  vtkm::rendering::ScalarRenderer renderer;
  renderer.SetInput(dataset);
  vtkm::rendering::ScalarRenderer::Result res = renderer.Render(camera);

  res.SaveVTK("scalar");
}

} //namespace

int UnitTestScalarRenderer(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(RenderTests, argc, argv);
}
