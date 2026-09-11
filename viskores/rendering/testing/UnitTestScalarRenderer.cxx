//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/io/VTKDataSetWriter.h>
#include <viskores/rendering/ScalarRenderer.h>
#include <viskores/rendering/raytracing/RayTracer.h>
#include <viskores/rendering/testing/RenderTest.h>

namespace
{

void RenderTests()
{
  viskores::cont::testing::MakeTestDataSet maker;
  viskores::cont::DataSet dataset = maker.Make3DRegularDataSet0();
  viskores::Bounds bounds = dataset.GetCoordinateSystem().GetBounds();

  viskores::rendering::Camera camera;
  camera.ResetToBounds(bounds);
  camera.Azimuth(-40.f);
  camera.Elevation(15.f);

  viskores::rendering::ScalarRenderer renderer;
  renderer.SetInput(dataset);

  viskores::rendering::ScalarRenderer::Result res = renderer.Render(camera);
  viskores::cont::DataSet result = res.ToDataSet();
  viskores::io::VTKDataSetWriter writer("scalar.vtk");
  writer.WriteDataSet(result);
}

viskores::cont::DataSet MakeTriangleDataSet()
{
  // The triangle lies in the xy plane and faces the camera, which is on the
  // positive z axis. It is deliberately large enough to contain the center
  // ray without placing the intersection near an edge.
  const std::vector<viskores::Vec3f_32> coordinates = { { -2.f, -2.f, 0.f },
                                                        { 2.f, -2.f, 0.f },
                                                        { 0.f, 2.f, 0.f } };
  const std::vector<viskores::Id> connectivity = { 0, 1, 2 };

  viskores::cont::DataSetBuilderExplicit builder;
  viskores::cont::DataSet dataSet =
    builder.Create(coordinates, viskores::CellShapeTagTriangle{}, 3, connectivity, "coordinates");
  dataSet.AddPointField("scalar", std::vector<viskores::Float32>{ 1.f, 1.f, 1.f });
  return dataSet;
}

viskores::Float32 RenderCenterShading(viskores::cont::DataSet& dataSet,
                                      const viskores::rendering::Camera& camera,
                                      bool setLightPosition,
                                      const viskores::Vec3f_32& lightPosition)
{
  // A 2x2 image has a ray at (1, 1) that passes through the camera's look-at
  // point. Sampling that ray gives an analytically predictable intersection
  // at the origin for the triangle used by this test.
  constexpr viskores::Int32 width = 2;
  constexpr viskores::Int32 height = 2;
  constexpr viskores::Id centerPixel = 3;

  viskores::rendering::ScalarRenderer renderer;
  renderer.SetInput(dataSet);
  renderer.SetWidth(width);
  renderer.SetHeight(height);
  if (setLightPosition)
    renderer.SetLightPosition(lightPosition);

  const viskores::rendering::ScalarRenderer::Result result = renderer.Render(camera);
  for (std::size_t index = 0; index < result.ScalarNames.size(); ++index)
  {
    if (result.ScalarNames[index] == "shading")
      return result.Scalars[index].ReadPortal().Get(centerPixel);
  }

  throw viskores::cont::ErrorBadValue("ScalarRenderer did not produce a shading field");
}

void ScalarRendererLightPositionTests()
{
  viskores::cont::DataSet dataSet = MakeTriangleDataSet();

  viskores::rendering::Camera camera;
  camera.SetPosition(viskores::Vec3f_32(0.f, 0.f, 3.f));
  camera.SetLookAt(viskores::Vec3f_32(0.f, 0.f, 0.f));
  camera.SetViewUp(viskores::Vec3f_32(0.f, 1.f, 0.f));
  camera.SetClippingRange(0.1f, 10.f);

  // With no explicit light, ScalarRenderer should use the camera position.
  // A light explicitly placed at the camera must therefore produce the same
  // shading. Both cases illuminate the front-facing triangle fully.
  const viskores::Float32 defaultShading =
    RenderCenterShading(dataSet, camera, false, viskores::Vec3f_32{});
  const viskores::Float32 cameraLightShading =
    RenderCenterShading(dataSet, camera, true, camera.GetPosition());
  VISKORES_TEST_ASSERT(test_equal(defaultShading, cameraLightShading),
                       "The default light position should be the camera position");
  VISKORES_TEST_ASSERT(test_equal(defaultShading, 1.f),
                       "A front light should fully illuminate the triangle");

  // A light directly behind the triangle contributes neither diffuse nor
  // specular illumination. The remaining value is the renderer's 0.5 ambient
  // term. This assertion verifies that SetLightPosition affects rendering, not
  // just the value returned by GetLightPosition.
  const viskores::Float32 rearLightShading =
    RenderCenterShading(dataSet, camera, true, viskores::Vec3f_32(0.f, 0.f, -3.f));
  VISKORES_TEST_ASSERT(test_equal(rearLightShading, 0.5f),
                       "A rear light should leave only ambient illumination");
}

void RayTracerLightPositionTests()
{
  viskores::rendering::raytracing::RayTracer tracer;

  const viskores::Vec3f_32 cameraPosition(3.f, 2.f, 1.f);
  tracer.GetCamera().SetPosition(cameraPosition);
  VISKORES_TEST_ASSERT(tracer.GetLightPosition() == cameraPosition);

  const viskores::Vec3f_32 lightPosition(-1.f, -2.f, -3.f);
  tracer.SetLightPosition(lightPosition);
  tracer.GetCamera().SetPosition(viskores::Vec3f_32(4.f, 5.f, 6.f));
  VISKORES_TEST_ASSERT(tracer.GetLightPosition() == lightPosition);
}

void RunTests()
{
  RenderTests();
  ScalarRendererLightPositionTests();
  RayTracerLightPositionTests();
}

} //namespace

int UnitTestScalarRenderer(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}
