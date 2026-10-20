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

namespace
{

constexpr viskores::Float32 LowValue = 1e7f;
constexpr viskores::Float32 HighValue = 2e7f;

// Two squares side by side in the z = 0 plane, each made of two triangles.
// Every vertex of the left square has attribute0 = LowValue, and every
// vertex of the right square has attribute0 = HighValue.
anari_cpp::Geometry CreateTwoSquares(anari_cpp::Device d)
{
  auto positionArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 12);
  auto* positions = anari_cpp::map<viskores::Vec3f_32>(d, positionArray);
  auto attributeArray = anari_cpp::newArray1D(d, ANARI_FLOAT32, 12);
  auto* attributes = anari_cpp::map<viskores::Float32>(d, attributeArray);
  for (int square = 0; square < 2; ++square)
  {
    const viskores::Float32 x0 = (square == 0) ? -1.f : 0.1f;
    const viskores::Float32 x1 = x0 + 0.9f;
    const viskores::Float32 value = (square == 0) ? LowValue : HighValue;
    viskores::Vec3f_32* p = positions + 6 * square;
    p[0] = viskores::Vec3f_32(x0, -0.5f, 0.f);
    p[1] = viskores::Vec3f_32(x1, -0.5f, 0.f);
    p[2] = viskores::Vec3f_32(x1, 0.5f, 0.f);
    p[3] = viskores::Vec3f_32(x0, -0.5f, 0.f);
    p[4] = viskores::Vec3f_32(x1, 0.5f, 0.f);
    p[5] = viskores::Vec3f_32(x0, 0.5f, 0.f);
    for (int i = 0; i < 6; ++i)
    {
      attributes[6 * square + i] = value;
    }
  }
  anari_cpp::unmap(d, positionArray);
  anari_cpp::unmap(d, attributeArray);

  auto geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "triangle");
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.position", positionArray);
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.attribute0", attributeArray);
  anari_cpp::commitParameters(d, geometry);
  return geometry;
}

// A two-color red-to-blue color map on attribute0, with the given sampler
// transform.
anari_cpp::Material CreateMaterial(anari_cpp::Device d,
                                   const anari_cpp::mat4& inTransform,
                                   const viskores::Vec4f_32& inOffset)
{
  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC4, 2);
  auto* colors = anari_cpp::map<viskores::Vec4f_32>(d, colorArray);
  colors[0] = viskores::Vec4f_32(1.f, 0.f, 0.f, 1.f);
  colors[1] = viskores::Vec4f_32(0.f, 0.f, 1.f, 1.f);
  anari_cpp::unmap(d, colorArray);

  auto sampler = anari_cpp::newObject<anari_cpp::Sampler>(d, "image1D");
  anari_cpp::setAndReleaseParameter(d, sampler, "image", colorArray);
  anari_cpp::setParameter(d, sampler, "filter", "nearest");
  anari_cpp::setParameter(d, sampler, "wrapMode", "clampToEdge");
  anari_cpp::setParameter(d, sampler, "inAttribute", "attribute0");
  anari_cpp::setParameter(d, sampler, "inTransform", inTransform);
  anari_cpp::setParameter(d, sampler, "inOffset", inOffset);
  anari_cpp::commitParameters(d, sampler);

  auto material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
  anari_cpp::setAndReleaseParameter(d, material, "color", sampler);
  anari_cpp::commitParameters(d, material);
  return material;
}

anari_cpp::World CreateWorld(anari_cpp::Device d,
                             const anari_cpp::mat4& inTransform,
                             const viskores::Vec4f_32& inOffset)
{
  auto surface = anari_cpp::newObject<anari_cpp::Surface>(d);
  anari_cpp::setAndReleaseParameter(d, surface, "geometry", CreateTwoSquares(d));
  anari_cpp::setAndReleaseParameter(
    d, surface, "material", CreateMaterial(d, inTransform, inOffset));
  anari_cpp::commitParameters(d, surface);

  auto world = anari_cpp::newObject<anari_cpp::World>(d);
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);
  anari_cpp::commitParameters(d, world);
  anari_cpp::release(d, surface);
  return world;
}

// Render the squares head-on and return the pixels at the middle of the left
// and right squares.
std::pair<viskores::Vec4f_32, viskores::Vec4f_32> RenderSquares(anari_cpp::Device d,
                                                                anari_cpp::World world)
{
  auto renderer = anari_cpp::newObject<anari_cpp::Renderer>(d, "default");
  anari_cpp::setParameter(d, renderer, "background", viskores::Vec4f_32(0.f, 0.f, 0.f, 1.f));
  anari_cpp::commitParameters(d, renderer);

  auto camera = anari_cpp::newObject<anari_cpp::Camera>(d, "perspective");
  anari_cpp::setParameter(d, camera, "aspect", 1.f);
  anari_cpp::setParameter(d, camera, "position", viskores::Vec3f_32(0.f, 0.f, 3.f));
  anari_cpp::setParameter(d, camera, "direction", viskores::Vec3f_32(0.f, 0.f, -1.f));
  anari_cpp::setParameter(d, camera, "up", viskores::Vec3f_32(0.f, 1.f, 0.f));
  anari_cpp::commitParameters(d, camera);

  const viskores::Vec2ui_32 frameSize(64, 64);
  auto frame = anari_cpp::newObject<anari_cpp::Frame>(d);
  anari_cpp::setParameter(d, frame, "size", frameSize);
  anari_cpp::setParameter(d, frame, "channel.color", ANARI_FLOAT32_VEC4);
  anari_cpp::setParameter(d, frame, "world", world);
  anari_cpp::setParameter(d, frame, "camera", camera);
  anari_cpp::setParameter(d, frame, "renderer", renderer);
  anari_cpp::commitParameters(d, frame);
  anari_cpp::release(d, camera);
  anari_cpp::release(d, renderer);

  anari_cpp::render(d, frame);
  anari_cpp::wait(d, frame);
  const auto pixels = anari_cpp::map<viskores::Vec4f_32>(d, frame, "channel.color");
  // The square centers are at x = -/+0.55, about a third of the way from the
  // middle of the image to its edge (the default 60 degree field of view
  // spans x = -/+1.73 at distance 3).
  const std::size_t row = pixels.height / 2;
  const std::size_t offset = pixels.width / 6;
  const viskores::Vec4f_32 left = pixels.data[row * pixels.width + pixels.width / 2 - offset];
  const viskores::Vec4f_32 right = pixels.data[row * pixels.width + pixels.width / 2 + offset];
  anari_cpp::unmap(d, frame, "channel.color");
  anari_cpp::release(d, frame);
  return { left, right };
}

void CheckColorMapEnds(anari_cpp::Device d,
                       const anari_cpp::mat4& inTransform,
                       const viskores::Vec4f_32& inOffset,
                       const char* description)
{
  auto world = CreateWorld(d, inTransform, inOffset);
  const auto pixels = RenderSquares(d, world);
  anari_cpp::release(d, world);

  const viskores::Vec4f_32& low = pixels.first;
  const viskores::Vec4f_32& high = pixels.second;
  VISKORES_TEST_ASSERT(low[0] > 0.1f && low[2] < 0.5f * low[0],
                       description,
                       ": the lowest value ",
                       LowValue,
                       " is not colored by the first color map entry (red); pixel ",
                       low);
  VISKORES_TEST_ASSERT(high[2] > 0.1f && high[0] < 0.5f * high[2],
                       description,
                       ": the highest value ",
                       HighValue,
                       " is not colored by the last color map entry (blue); pixel ",
                       high);
}

// A sampler inTransform that maps a wide value range, [1e7, 2e7], onto
// [0, 1]. Its scale, 1e-7, is below Float32 epsilon.
void WideRangeTests(anari_cpp::Device d)
{
  const viskores::Float32 scale = 1.f / (HighValue - LowValue);

  // (value - LowValue) * scale, all in the matrix.
  CheckColorMapEnds(
    d,
    anari_cpp::mul(anari_cpp::scaling_matrix(anari_cpp::float3(scale)),
                   anari_cpp::translation_matrix(anari_cpp::float3(-LowValue, 0.f, 0.f))),
    viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f),
    "Translation in inTransform");

  // value * scale + inOffset, with the offset added after the transform.
  CheckColorMapEnds(d,
                    anari_cpp::scaling_matrix(anari_cpp::float3(scale)),
                    viskores::Vec4f_32(-LowValue * scale, 0.f, 0.f, 0.f),
                    "Translation in inOffset");
}

void RenderTests()
{
  auto d = loadANARIDevice();
  WideRangeTests(d);
  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARISamplerImage1D(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
