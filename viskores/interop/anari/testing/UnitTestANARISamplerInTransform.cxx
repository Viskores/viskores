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

#include <cstddef>
#include <string>
#include <utility>

// The device colors a surface through a sampler by finding the field values
// that the sampler's inTransform and inOffset map to texture coordinates 0
// and 1. These tests check that inverse for wide value ranges and per axis.

namespace
{

constexpr viskores::Float32 LowValue = 1e7f;
constexpr viskores::Float32 HighValue = 2e7f;

const viskores::Vec4f_32 Red(1.f, 0.f, 0.f, 1.f);
const viskores::Vec4f_32 Green(0.f, 1.f, 0.f, 1.f);
const viskores::Vec4f_32 Blue(0.f, 0.f, 1.f, 1.f);
const viskores::Vec4f_32 White(1.f, 1.f, 1.f, 1.f);

// Two squares side by side in the z = 0 plane, each made of two triangles.
// Every vertex of the left square has attribute0 = leftValue, and every
// vertex of the right square has attribute0 = rightValue. With
// numComponents = 1 the attribute is the first component only.
anari_cpp::Geometry CreateTwoSquares(anari_cpp::Device d,
                                     viskores::IdComponent numComponents,
                                     const viskores::Vec2f_32& leftValue,
                                     const viskores::Vec2f_32& rightValue)
{
  constexpr viskores::IdComponent verticesPerSquare = 6;
  auto positionArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 2 * verticesPerSquare);
  auto* positions = anari_cpp::map<viskores::Vec3f_32>(d, positionArray);
  auto attributeArray = anari_cpp::newArray1D(
    d, (numComponents == 1) ? ANARI_FLOAT32 : ANARI_FLOAT32_VEC2, 2 * verticesPerSquare);
  auto* attributes = anari_cpp::map<viskores::Float32>(d, attributeArray);
  for (viskores::IdComponent square = 0; square < 2; ++square)
  {
    const viskores::Float32 leftX = (square == 0) ? -1.f : 0.1f;
    const viskores::Float32 rightX = leftX + 0.9f;
    const viskores::Vec2f_32& value = (square == 0) ? leftValue : rightValue;
    viskores::Vec3f_32* squarePositions = positions + verticesPerSquare * square;
    squarePositions[0] = viskores::Vec3f_32(leftX, -0.5f, 0.f);
    squarePositions[1] = viskores::Vec3f_32(rightX, -0.5f, 0.f);
    squarePositions[2] = viskores::Vec3f_32(rightX, 0.5f, 0.f);
    squarePositions[3] = viskores::Vec3f_32(leftX, -0.5f, 0.f);
    squarePositions[4] = viskores::Vec3f_32(rightX, 0.5f, 0.f);
    squarePositions[5] = viskores::Vec3f_32(leftX, 0.5f, 0.f);
    for (viskores::IdComponent vertex = 0; vertex < verticesPerSquare; ++vertex)
    {
      for (viskores::IdComponent component = 0; component < numComponents; ++component)
      {
        attributes[(verticesPerSquare * square + vertex) * numComponents + component] =
          value[component];
      }
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

// A nearest-filtered sampler on attribute0 with the given transform. The
// array type is kept so the `image` parameter gets the array's ANARI type.
template <typename ImageArray>
anari_cpp::Sampler CreateSampler(anari_cpp::Device d,
                                 const char* subtype,
                                 ImageArray colorArray,
                                 const anari_cpp::mat4& inTransform,
                                 const viskores::Vec4f_32& inOffset)
{
  auto sampler = anari_cpp::newObject<anari_cpp::Sampler>(d, subtype);
  anari_cpp::setAndReleaseParameter(d, sampler, "image", colorArray);
  anari_cpp::setParameter(d, sampler, "filter", "nearest");
  anari_cpp::setParameter(d, sampler, "wrapMode", "clampToEdge");
  anari_cpp::setParameter(d, sampler, "inAttribute", "attribute0");
  anari_cpp::setParameter(d, sampler, "inTransform", inTransform);
  anari_cpp::setParameter(d, sampler, "inOffset", inOffset);
  anari_cpp::commitParameters(d, sampler);
  return sampler;
}

anari_cpp::World CreateWorld(anari_cpp::Device d,
                             anari_cpp::Geometry geometry,
                             anari_cpp::Sampler sampler)
{
  auto material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
  anari_cpp::setAndReleaseParameter(d, material, "color", sampler);
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

// Whether a shaded pixel has the hue of `expected`, one of the pure colors
// above: its lit channels are bright and its unlit channels dark.
bool HasColor(const viskores::Vec4f_32& pixel, const viskores::Vec4f_32& expected)
{
  viskores::Float32 litMin = 1.f;
  viskores::Float32 unlitMax = 0.f;
  for (viskores::IdComponent channel = 0; channel < 3; ++channel)
  {
    if (expected[channel] > 0.f)
    {
      litMin = viskores::Min(litMin, pixel[channel]);
    }
    else
    {
      unlitMax = viskores::Max(unlitMax, pixel[channel]);
    }
  }
  return (litMin > 0.1f) && (unlitMax < 0.5f * litMin);
}

void CheckSquareColors(anari_cpp::Device d,
                       anari_cpp::World world,
                       const viskores::Vec4f_32& expectedLeft,
                       const viskores::Vec4f_32& expectedRight,
                       const std::string& description)
{
  const auto pixels = RenderSquares(d, world);
  VISKORES_TEST_ASSERT(HasColor(pixels.first, expectedLeft),
                       description,
                       ": left square pixel ",
                       pixels.first,
                       " does not have the color ",
                       expectedLeft);
  VISKORES_TEST_ASSERT(HasColor(pixels.second, expectedRight),
                       description,
                       ": right square pixel ",
                       pixels.second,
                       " does not have the color ",
                       expectedRight);
}

// An image1D sampler whose inTransform maps a wide value range,
// [LowValue, HighValue], onto [0, 1]. Its scale, 1e-7, is below Float32
// epsilon. The lowest and highest values get the two ends of a red-to-blue
// color map.
void CheckWideRange(anari_cpp::Device d,
                    const anari_cpp::mat4& inTransform,
                    const viskores::Vec4f_32& inOffset,
                    const std::string& description)
{
  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC4, 2);
  auto* colors = anari_cpp::map<viskores::Vec4f_32>(d, colorArray);
  colors[0] = Red;
  colors[1] = Blue;
  anari_cpp::unmap(d, colorArray);

  auto world = CreateWorld(
    d,
    CreateTwoSquares(d, 1, viskores::Vec2f_32(LowValue, 0.f), viskores::Vec2f_32(HighValue, 0.f)),
    CreateSampler(d, "image1D", colorArray, inTransform, inOffset));
  CheckSquareColors(d, world, Red, Blue, description);
  anari_cpp::release(d, world);
}

void WideRangeTests(anari_cpp::Device d)
{
  const viskores::Float32 scale = 1.f / (HighValue - LowValue);

  // (value - LowValue) * scale, all in the matrix.
  CheckWideRange(
    d,
    anari_cpp::mul(anari_cpp::scaling_matrix(anari_cpp::float3(scale)),
                   anari_cpp::translation_matrix(anari_cpp::float3(-LowValue, 0.f, 0.f))),
    viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f),
    "image1D, translation in inTransform");

  // value * scale + inOffset, with the offset added after the transform.
  CheckWideRange(d,
                 anari_cpp::scaling_matrix(anari_cpp::float3(scale)),
                 viskores::Vec4f_32(-LowValue * scale, 0.f, 0.f, 0.f),
                 "image1D, translation in inOffset");
}

// An image2D sampler whose two axes have different value ranges: [0, 1e7]
// for the first and [0, 1] for the second. Each square's values lie outside
// both ranges, so they clamp to a corner of a 2x2 color image.
void PerAxisTests(anari_cpp::Device d)
{
  // Texel (x, y) is at y * 2 + x.
  auto colorArray = anari_cpp::newArray2D(d, ANARI_FLOAT32_VEC4, 2, 2);
  auto* colors = anari_cpp::map<viskores::Vec4f_32>(d, colorArray);
  colors[0] = Red;
  colors[1] = Blue;
  colors[2] = Green;
  colors[3] = White;
  anari_cpp::unmap(d, colorArray);

  const anari_cpp::mat4 inTransform =
    anari_cpp::scaling_matrix(anari_cpp::float3(1.f / LowValue, 1.f, 1.f));

  // Left: below the first range, above the second -> texel (0, 1), green.
  // Right: above the first range, below the second -> texel (1, 0), blue.
  // With the first axis's range used for the second axis, 2 would fall
  // inside [0, 1e7] near its start, and the left square would be red.
  auto world = CreateWorld(
    d,
    CreateTwoSquares(d, 2, viskores::Vec2f_32(-LowValue, 2.f), viskores::Vec2f_32(HighValue, -1.f)),
    CreateSampler(d, "image2D", colorArray, inTransform, viskores::Vec4f_32(0.f, 0.f, 0.f, 0.f)));
  CheckSquareColors(d, world, Green, Blue, "image2D, separate range per axis");
  anari_cpp::release(d, world);
}

void RenderTests()
{
  auto d = loadANARIDevice();
  WideRangeTests(d);
  PerAxisTests(d);
  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARISamplerInTransform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
