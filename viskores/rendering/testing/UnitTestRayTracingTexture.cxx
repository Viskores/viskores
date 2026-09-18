//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/CoordinateSystem.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/rendering/raytracing/RayTracer.h>
#include <viskores/rendering/raytracing/TriangleIntersector.h>

#include <memory>
#include <vector>

namespace
{

using Precision = viskores::Float32;
using Ray = viskores::rendering::raytracing::Ray<Precision>;

void InitializeRay(Ray& ray)
{
  ray.NumRays = 1;
  viskores::cont::ArrayCopy(
    viskores::cont::make_ArrayHandle({ viskores::Vec3f_64{ 0.25, 0.25, 1.0 } }), ray.Origin);
  viskores::cont::ArrayCopy(
    viskores::cont::make_ArrayHandle({ viskores::Vec3f_64{ 0.0, 0.0, -1.0 } }), ray.Dir);
  ray.MinDistance = viskores::cont::make_ArrayHandle<Precision>({ 0.0f });
  ray.MaxDistance = viskores::cont::make_ArrayHandle<Precision>({ 10.0f });
  ray.Distance = viskores::cont::make_ArrayHandle<Precision>({ 10.0f });
  ray.HitIdx = viskores::cont::make_ArrayHandle<viskores::Id>({ -1 });
  ray.PixelIdx = viskores::cont::make_ArrayHandle<viskores::Id>({ 0 });
  ray.Status = viskores::cont::make_ArrayHandle<viskores::UInt8>({ RAY_ACTIVE });
  ray.Buffers[0].Resize(1);
}

std::shared_ptr<viskores::rendering::raytracing::TriangleIntersector> MakeIntersector()
{
  auto coordinates = viskores::cont::make_ArrayHandle({ viskores::Vec3f_32(0.0f, 0.0f, 0.0f),
                                                        viskores::Vec3f_32(1.0f, 0.0f, 0.0f),
                                                        viskores::Vec3f_32(0.0f, 1.0f, 0.0f) });
  auto triangles = viskores::cont::make_ArrayHandle({ viskores::Id4(0, 0, 1, 2) });
  auto intersector = std::make_shared<viskores::rendering::raytracing::TriangleIntersector>();
  intersector->SetData(viskores::cont::CoordinateSystem("coordinates", coordinates), triangles);
  return intersector;
}

void TestTextureCoordinatesAndLookup()
{
  auto intersector = MakeIntersector();
  auto textureValues = viskores::cont::make_ArrayHandle({ viskores::Vec2f_32(0.0f, 0.0f),
                                                          viskores::Vec2f_32(2.0f, 0.0f),
                                                          viskores::Vec2f_32(0.0f, 4.0f) });
  viskores::cont::Field textureField(
    "texture", viskores::cont::Field::Association::Points, textureValues);
  auto textureRanges =
    viskores::cont::make_ArrayHandle({ viskores::Range(0.0, 2.0), viskores::Range(0.0, 4.0) });

  Ray ray;
  InitializeRay(ray);
  intersector->IntersectRays(ray);
  intersector->IntersectionData(ray, textureField, textureRanges);
  VISKORES_TEST_ASSERT(test_equal(ray.TextureR.ReadPortal().Get(0), 0.25f));
  VISKORES_TEST_ASSERT(test_equal(ray.TextureS.ReadPortal().Get(0), 0.25f));

  std::vector<viskores::Vec4f_32> colors(25, viskores::Vec4f_32(0.0f));
  colors[6] = viskores::Vec4f_32(0.2f, 0.4f, 0.6f, 1.0f);
  auto colorMap = viskores::cont::make_ArrayHandle(colors, viskores::CopyFlag::On);

  Ray renderRay;
  InitializeRay(renderRay);
  viskores::rendering::raytracing::RayTracer tracer;
  tracer.AddShapeIntersector(intersector);
  tracer.SetField(textureField, textureRanges);
  tracer.SetColorMap(colorMap, viskores::Id2(5, 5));
  tracer.SetShadingOn(false);
  tracer.Render(renderRay);

  const auto rendered = renderRay.Buffers[0].Buffer.ReadPortal();
  VISKORES_TEST_ASSERT(test_equal(rendered.Get(0), 0.2f));
  VISKORES_TEST_ASSERT(test_equal(rendered.Get(1), 0.4f));
  VISKORES_TEST_ASSERT(test_equal(rendered.Get(2), 0.6f));
  VISKORES_TEST_ASSERT(test_equal(rendered.Get(3), 1.0f));
}

void TestOneDimensionalTexture()
{
  auto intersector = MakeIntersector();
  auto values = viskores::cont::make_ArrayHandle({ 0.0f, 2.0f, 0.0f });
  viskores::cont::Field field("texture", viskores::cont::Field::Association::Points, values);
  auto ranges = viskores::cont::make_ArrayHandle({ viskores::Range(0.0, 2.0) });

  Ray ray;
  InitializeRay(ray);
  intersector->IntersectRays(ray);
  intersector->IntersectionData(ray, field, ranges);
  VISKORES_TEST_ASSERT(test_equal(ray.TextureR.ReadPortal().Get(0), 0.25f));
  VISKORES_TEST_ASSERT(test_equal(ray.TextureS.ReadPortal().Get(0), 0.0f));

  auto colorMap = viskores::cont::make_ArrayHandle({ viskores::Vec4f_32(0.0f),
                                                     viskores::Vec4f_32(0.3f, 0.5f, 0.7f, 1.0f),
                                                     viskores::Vec4f_32(0.0f),
                                                     viskores::Vec4f_32(0.0f),
                                                     viskores::Vec4f_32(0.0f) });
  Ray renderRay;
  InitializeRay(renderRay);
  viskores::rendering::raytracing::RayTracer tracer;
  tracer.AddShapeIntersector(intersector);
  tracer.SetField(field, viskores::Range(0.0, 2.0));
  tracer.SetColorMap(colorMap);
  tracer.SetShadingOn(false);
  tracer.Render(renderRay);

  const auto rendered = renderRay.Buffers[0].Buffer.ReadPortal();
  VISKORES_TEST_ASSERT(test_equal(rendered.Get(0), 0.3f));
  VISKORES_TEST_ASSERT(test_equal(rendered.Get(1), 0.5f));
  VISKORES_TEST_ASSERT(test_equal(rendered.Get(2), 0.7f));
  VISKORES_TEST_ASSERT(test_equal(rendered.Get(3), 1.0f));
}

void RunTests()
{
  TestTextureCoordinatesAndLookup();
  TestOneDimensionalTexture();
}

} // namespace

int UnitTestRayTracingTexture(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunTests, argc, argv);
}
