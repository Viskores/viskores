//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

// viskores
#include <viskores/cont/testing/Testing.h>

#include "ANARITestCommon.h"

#include <vector>

namespace
{

constexpr viskores::UInt32 ImageSize = 32;
const viskores::Vec4f_32 Background(0.3f, 0.3f, 0.3f, 1.f);

// A red triangle in front of a camera at its default position and direction
// (the origin, looking down -z).
anari_cpp::World CreateTriangleWorld(anari_cpp::Device d)
{
  auto vertices = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 3);
  auto* positions = anari_cpp::map<viskores::Vec3f_32>(d, vertices);
  positions[0] = viskores::Vec3f_32(-1.f, -1.f, -3.f);
  positions[1] = viskores::Vec3f_32(1.f, -1.f, -3.f);
  positions[2] = viskores::Vec3f_32(0.f, 1.f, -3.f);
  anari_cpp::unmap(d, vertices);

  auto geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "triangle");
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.position", vertices);
  anari_cpp::commitParameters(d, geometry);

  auto material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
  anari_cpp::setParameter(d, material, "color", viskores::Vec3f_32(1.f, 0.f, 0.f));
  anari_cpp::commitParameters(d, material);

  auto surface = anari_cpp::newObject<anari_cpp::Surface>(d);
  anari_cpp::setAndReleaseParameter(d, surface, "geometry", geometry);
  anari_cpp::setAndReleaseParameter(d, surface, "material", material);
  anari_cpp::commitParameters(d, surface);

  auto world = anari_cpp::newObject<anari_cpp::World>(d);
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);
  anari_cpp::commitParameters(d, world);
  anari_cpp::release(d, surface);

  return world;
}

std::vector<viskores::Vec4f_32> RenderPixels(anari_cpp::Device d,
                                             anari_cpp::World world,
                                             anari_cpp::Camera camera)
{
  auto renderer = anari_cpp::newObject<anari_cpp::Renderer>(d, "default");
  anari_cpp::setParameter(d, renderer, "background", Background);
  anari_cpp::commitParameters(d, renderer);

  auto frame = anari_cpp::newObject<anari_cpp::Frame>(d);
  anari_cpp::setParameter(d, frame, "size", viskores::Vec2ui_32(ImageSize, ImageSize));
  anari_cpp::setParameter(d, frame, "channel.color", ANARI_FLOAT32_VEC4);
  anari_cpp::setParameter(d, frame, "world", world);
  anari_cpp::setParameter(d, frame, "camera", camera);
  anari_cpp::setParameter(d, frame, "renderer", renderer);
  anari_cpp::commitParameters(d, frame);
  anari_cpp::release(d, renderer);

  anari_cpp::render(d, frame);
  anari_cpp::wait(d, frame);

  const auto fb = anari_cpp::map<viskores::Vec4f_32>(d, frame, "channel.color");
  std::vector<viskores::Vec4f_32> pixels(fb.data, fb.data + fb.width * fb.height);
  anari_cpp::unmap(d, frame, "channel.color");
  anari_cpp::release(d, frame);

  return pixels;
}

// A perspective camera committed with no parameters set renders like one given
// the default position, direction and up vector explicitly.
void DefaultParametersTest()
{
  auto d = loadANARIDevice();
  auto world = CreateTriangleWorld(d);

  // Render with the parameterless camera first, so that it does not reuse the
  // memory of a camera that was given values.
  auto defaultCamera = anari_cpp::newObject<anari_cpp::Camera>(d, "perspective");
  anari_cpp::commitParameters(d, defaultCamera);
  const auto pixels = RenderPixels(d, world, defaultCamera);
  anari_cpp::release(d, defaultCamera);

  auto explicitCamera = anari_cpp::newObject<anari_cpp::Camera>(d, "perspective");
  anari_cpp::setParameter(d, explicitCamera, "position", viskores::Vec3f_32(0.f, 0.f, 0.f));
  anari_cpp::setParameter(d, explicitCamera, "direction", viskores::Vec3f_32(0.f, 0.f, -1.f));
  anari_cpp::setParameter(d, explicitCamera, "up", viskores::Vec3f_32(0.f, 1.f, 0.f));
  anari_cpp::commitParameters(d, explicitCamera);
  const auto expected = RenderPixels(d, world, explicitCamera);
  anari_cpp::release(d, explicitCamera);

  // The triangle must show up for the comparison below to mean anything.
  const std::size_t centerPixel = (ImageSize / 2) * ImageSize + ImageSize / 2;
  VISKORES_TEST_ASSERT(!test_equal(expected[centerPixel], Background),
                       "The triangle does not cover the center of the image");

  VISKORES_TEST_ASSERT(pixels.size() == expected.size(), "Images differ in size");
  for (std::size_t pixelIndex = 0; pixelIndex < pixels.size(); ++pixelIndex)
  {
    VISKORES_TEST_ASSERT(test_equal(pixels[pixelIndex], expected[pixelIndex]),
                         "Pixel ",
                         pixelIndex,
                         " of the perspective camera without parameters is ",
                         pixels[pixelIndex],
                         "; expected ",
                         expected[pixelIndex]);
  }

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARICameraDefaults(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DefaultParametersTest, argc, argv);
}
