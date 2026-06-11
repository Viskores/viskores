//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

// viskores
#include <viskores/cont/testing/Testing.h>
#include <viskores/io/EncodePNG.h>

#include "ANARITestCommon.h"

namespace
{

void RenderTests()
{
  // Initialize ANARI /////////////////////////////////////////////////////////

  auto d = loadANARIDevice();

  const char** extensions = nullptr;
  anariGetProperty(d, d, "extension", ANARI_STRING_LIST, &extensions, sizeof(char**), ANARI_WAIT);
  bool conesSupported = false;
  for (int i = 0; extensions[i] != nullptr; ++i)
  {
    if (extensions[i] == std::string("KHR_GEOMETRY_CONE") ||
        extensions[i] == std::string("ANARI_KHR_GEOMETRY_CONE"))
    {
      conesSupported = true;
      break;
    }
  }
  if (!conesSupported)
    VISKORES_TEST_SKIP("ANARI KHR_GEOMETRY_CONE extension not supported by ANARI device.");

  // Create indexed ANARI cone geometry //////////////////////////////////////

  auto indexedVertices = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 4);
  auto* indexedPositions = anari_cpp::map<viskores::Vec3f_32>(d, indexedVertices);
  indexedPositions[0] = viskores::Vec3f_32(-0.8f, -0.35f, -0.25f);
  indexedPositions[1] = viskores::Vec3f_32(0.05f, -0.1f, 0.2f);
  indexedPositions[2] = viskores::Vec3f_32(-0.65f, 0.45f, -0.15f);
  indexedPositions[3] = viskores::Vec3f_32(0.35f, 0.25f, 0.25f);
  anari_cpp::unmap(d, indexedVertices);

  auto indices = anari_cpp::newArray1D(d, ANARI_UINT32_VEC2, 2);
  auto* cones = anari_cpp::map<viskores::Vec2ui_32>(d, indices);
  cones[0] = viskores::Vec2ui_32(0, 1);
  cones[1] = viskores::Vec2ui_32(2, 3);
  anari_cpp::unmap(d, indices);

  auto indexedRadii = anari_cpp::newArray1D(d, ANARI_FLOAT32, 4);
  auto* indexedVertexRadii = anari_cpp::map<viskores::Float32>(d, indexedRadii);
  indexedVertexRadii[0] = 0.22f;
  indexedVertexRadii[1] = 0.08f;
  indexedVertexRadii[2] = 0.17f;
  indexedVertexRadii[3] = 0.f;
  anari_cpp::unmap(d, indexedRadii);

  auto caps = anari_cpp::newArray1D(d, ANARI_UINT8, 4);
  auto* vertexCaps = anari_cpp::map<viskores::UInt8>(d, caps);
  vertexCaps[0] = 1;
  vertexCaps[1] = 1;
  vertexCaps[2] = 1;
  vertexCaps[3] = 0;
  anari_cpp::unmap(d, caps);

  auto primitiveAttributes = anari_cpp::newArray1D(d, ANARI_FLOAT32, 2);
  auto* primitiveAttribute0 = anari_cpp::map<viskores::Float32>(d, primitiveAttributes);
  primitiveAttribute0[0] = 0.f;
  primitiveAttribute0[1] = 1.f;
  anari_cpp::unmap(d, primitiveAttributes);

  auto indexedGeometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "cone");
  anari_cpp::setAndReleaseParameter(d, indexedGeometry, "vertex.position", indexedVertices);
  anari_cpp::setAndReleaseParameter(d, indexedGeometry, "primitive.index", indices);
  anari_cpp::setAndReleaseParameter(d, indexedGeometry, "vertex.radius", indexedRadii);
  anari_cpp::setAndReleaseParameter(d, indexedGeometry, "vertex.cap", caps);
  anari_cpp::setAndReleaseParameter(
    d, indexedGeometry, "primitive.attribute0", primitiveAttributes);
  anari_cpp::setParameter(d, indexedGeometry, "caps", "none");
  anari_cpp::commitParameters(d, indexedGeometry);

  // Create non-indexed ANARI cone geometry //////////////////////////////////

  auto vertices = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC3, 4);
  auto* positions = anari_cpp::map<viskores::Vec3f_32>(d, vertices);
  positions[0] = viskores::Vec3f_32(0.18f, -0.62f, -0.25f);
  positions[1] = viskores::Vec3f_32(0.9f, -0.28f, 0.25f);
  positions[2] = viskores::Vec3f_32(0.12f, 0.55f, 0.18f);
  positions[3] = viskores::Vec3f_32(0.92f, 0.6f, -0.18f);
  anari_cpp::unmap(d, vertices);

  auto radii = anari_cpp::newArray1D(d, ANARI_FLOAT32, 4);
  auto* vertexRadii = anari_cpp::map<viskores::Float32>(d, radii);
  vertexRadii[0] = 0.f;
  vertexRadii[1] = 0.14f;
  vertexRadii[2] = 0.1f;
  vertexRadii[3] = 0.22f;
  anari_cpp::unmap(d, radii);

  auto vertexAttributes = anari_cpp::newArray1D(d, ANARI_FLOAT32, 4);
  auto* vertexAttribute0 = anari_cpp::map<viskores::Float32>(d, vertexAttributes);
  vertexAttribute0[0] = 0.f;
  vertexAttribute0[1] = 1.f;
  vertexAttribute0[2] = 0.25f;
  vertexAttribute0[3] = 0.75f;
  anari_cpp::unmap(d, vertexAttributes);

  auto geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "cone");
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.position", vertices);
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.radius", radii);
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.attribute0", vertexAttributes);
  anari_cpp::setParameter(d, geometry, "caps", "both");
  anari_cpp::commitParameters(d, geometry);

  // Assemble ANARI scene /////////////////////////////////////////////////////

  auto colorArray = anari_cpp::newArray1D(d, ANARI_FLOAT32_VEC4, 4);
  auto* colors = anari_cpp::map<viskores::Vec4f_32>(d, colorArray);
  colors[0] = viskores::Vec4f_32(0.95f, 0.15f, 0.1f, 1.f);
  colors[1] = viskores::Vec4f_32(0.95f, 0.85f, 0.1f, 1.f);
  colors[2] = viskores::Vec4f_32(0.1f, 0.55f, 0.95f, 1.f);
  colors[3] = viskores::Vec4f_32(0.15f, 0.75f, 0.25f, 1.f);
  anari_cpp::unmap(d, colorArray);

  auto sampler = anari_cpp::newObject<anari_cpp::Sampler>(d, "image1D");
  anari_cpp::setAndReleaseParameter(d, sampler, "image", colorArray);
  anari_cpp::setParameter(d, sampler, "filter", "nearest");
  anari_cpp::setParameter(d, sampler, "wrapMode", "clampToEdge");
  anari_cpp::setParameter(d, sampler, "inAttribute", "attribute0");
  anari_cpp::commitParameters(d, sampler);

  auto material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
  anari_cpp::setParameter(d, material, "color", sampler);
  anari_cpp::commitParameters(d, material);
  anari_cpp::release(d, sampler);

  auto indexedSurface = anari_cpp::newObject<anari_cpp::Surface>(d);
  anari_cpp::setParameter(d, indexedSurface, "geometry", indexedGeometry);
  anari_cpp::setParameter(d, indexedSurface, "material", material);
  anari_cpp::commitParameters(d, indexedSurface);
  anari_cpp::release(d, indexedGeometry);

  auto surface = anari_cpp::newObject<anari_cpp::Surface>(d);
  anari_cpp::setParameter(d, surface, "geometry", geometry);
  anari_cpp::setParameter(d, surface, "material", material);
  anari_cpp::commitParameters(d, surface);
  anari_cpp::release(d, geometry);
  anari_cpp::release(d, material);

  anari_cpp::Surface surfaces[] = { indexedSurface, surface };
  auto world = anari_cpp::newObject<anari_cpp::World>(d);
  anari_cpp::setParameterArray1D(d, world, "surface", surfaces, 2);
  anari_cpp::commitParameters(d, world);
  anari_cpp::release(d, indexedSurface);
  anari_cpp::release(d, surface);

  // Render a frame ///////////////////////////////////////////////////////////

  renderTestANARIImage(d,
                       world,
                       viskores::Vec3f_32(2.f, -2.2f, 1.3f),
                       viskores::Vec3f_32(-2.f, 2.2f, -1.3f),
                       viskores::Vec3f_32(0.f, 0.f, 1.f),
                       "interop/anari/cone.png",
                       viskores::Vec2ui_32(512, 512));

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARIGeometryCone(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
