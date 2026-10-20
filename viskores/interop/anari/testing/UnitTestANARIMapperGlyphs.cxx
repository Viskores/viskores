//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


// viskores::anari
#include <viskores/interop/anari/ANARIMapperGlyphs.h>
// viskores
#include <viskores/Math.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/vector_analysis/Gradient.h>
#include <viskores/io/EncodePNG.h>
#include <viskores/source/Tangle.h>
// std
#include <cstdlib>
#include <vector>

#include "ANARITestCommon.h"

namespace
{

// Changing `SetOffsetGlyphs` after the surface exists moves the glyph vertices
// without a new actor. This only reads the arrays, so it runs even when the
// device has no cones.
void OffsetToggleTests()
{
  auto d = loadANARIDevice();

  // Two vertex cells one unit apart, each with a vector along +Z. The glyph
  // size is the bounds diagonal / 300.
  const std::vector<viskores::Vec3f_32> coords = { { 0.f, 0.f, 0.f }, { 1.f, 0.f, 0.f } };
  auto dataSet = viskores::cont::DataSetBuilderExplicit::Create(
    coords,
    std::vector<viskores::UInt8>{ viskores::CELL_SHAPE_VERTEX, viskores::CELL_SHAPE_VERTEX },
    std::vector<viskores::IdComponent>{ 1, 1 },
    std::vector<viskores::Id>{ 0, 1 });
  dataSet.AddPointField("vectors",
                        std::vector<viskores::Vec3f_32>{ { 0.f, 0.f, 1.f }, { 0.f, 0.f, 1.f } });

  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("vectors"));
  viskores::interop::anari::ANARIMapperGlyphs mapper(d, actor);
  mapper.GetANARISurface();

  const auto& params = mapper.GetANARIGlyphsParameters();
  const viskores::Float32 size = 1.f / 300.f;
  const auto checkFirstGlyph = [&](viskores::Float32 tipZ, viskores::Float32 tailZ)
  {
    VISKORES_TEST_ASSERT(params.NumPrimitives == 2, "Expected 2 glyphs");
    // Each glyph is two cones, so 4 vertices per glyph.
    const std::size_t numVertices = 4 * 2;
    const auto vertices =
      ReadANARIArray<viskores::Vec3f_32>(d, params.Vertex.Position, numVertices);
    VISKORES_TEST_ASSERT(test_equal(vertices[0], viskores::Vec3f_32(0.f, 0.f, tipZ)),
                         "Wrong first vertex of glyph 0: ",
                         vertices[0]);
    VISKORES_TEST_ASSERT(test_equal(vertices[3], viskores::Vec3f_32(0.f, 0.f, tailZ)),
                         "Wrong last vertex of glyph 0: ",
                         vertices[3]);
  };

  checkFirstGlyph(size, -size);

  mapper.SetOffsetGlyphs(true);
  checkFirstGlyph(0.f, -2.f * size);

  mapper.SetOffsetGlyphs(false);
  checkFirstGlyph(size, -size);

  anari_cpp::release(d, d);
}

// Explicit vertex cells, one per point.
viskores::cont::DataSet MakeVertexDataSet(const std::vector<viskores::Vec3f_32>& coords)
{
  const auto numPoints = coords.size();
  std::vector<viskores::Id> connectivity(numPoints);
  for (std::size_t i = 0; i < numPoints; ++i)
  {
    connectivity[i] = static_cast<viskores::Id>(i);
  }
  return viskores::cont::DataSetBuilderExplicit::Create(
    coords,
    std::vector<viskores::UInt8>(numPoints, viskores::CELL_SHAPE_VERTEX),
    std::vector<viskores::IdComponent>(numPoints, 1),
    connectivity);
}

// A vector too short to normalize gives a zero-size glyph at its point, not
// NaN or infinite vertices.
void ZeroVectorTests()
{
  auto d = loadANARIDevice();

  const std::vector<viskores::Vec3f_32> coords = { { 0.f, 0.f, 0.f },
                                                   { 1.f, 0.f, 0.f },
                                                   { 2.f, 0.f, 0.f } };
  auto dataSet = MakeVertexDataSet(coords);
  // A zero vector, a normal one, and one whose squared length underflows.
  dataSet.AddPointField(
    "vectors",
    std::vector<viskores::Vec3f_32>{ { 0.f, 0.f, 0.f }, { 0.f, 0.f, 1.f }, { 1e-30f, 0.f, 0.f } });

  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("vectors"));
  viskores::interop::anari::ANARIMapperGlyphs mapper(d, actor);
  mapper.GetANARISurface();

  const auto& params = mapper.GetANARIGlyphsParameters();
  VISKORES_TEST_ASSERT(params.NumPrimitives == 3, "Expected 3 glyphs");
  const std::size_t numVertices = 4 * 3;
  const auto vertices = ReadANARIArray<viskores::Vec3f_32>(d, params.Vertex.Position, numVertices);
  const auto radii = ReadANARIArray<viskores::Float32>(d, params.Vertex.Radius, numVertices);

  for (std::size_t i = 0; i < numVertices; ++i)
  {
    for (viskores::IdComponent c = 0; c < 3; ++c)
    {
      VISKORES_TEST_ASSERT(
        viskores::IsFinite(vertices[i][c]), "vertex.position ", i, " is not finite: ", vertices[i]);
    }
    VISKORES_TEST_ASSERT(
      viskores::IsFinite(radii[i]), "vertex.radius ", i, " is not finite: ", radii[i]);
  }

  // The degenerate glyphs collapse onto their points with zero radius.
  for (std::size_t glyph : { std::size_t{ 0 }, std::size_t{ 2 } })
  {
    for (std::size_t v = 0; v < 4; ++v)
    {
      const auto i = 4 * glyph + v;
      VISKORES_TEST_ASSERT(test_equal(vertices[i], coords[glyph]),
                           "Degenerate glyph ",
                           glyph,
                           " vertex ",
                           v,
                           " is not at its point: ",
                           vertices[i]);
      VISKORES_TEST_ASSERT(radii[i] == 0.f, "Degenerate glyph ", glyph, " has a non-zero radius");
    }
  }

  // The glyph with a real vector is unchanged.
  const viskores::Float32 size = 2.f / 300.f;
  VISKORES_TEST_ASSERT(test_equal(vertices[4], viskores::Vec3f_32(1.f, 0.f, size)),
                       "Wrong first vertex of glyph 1: ",
                       vertices[4]);
  VISKORES_TEST_ASSERT(test_equal(radii[4], size / 8), "Wrong first radius of glyph 1");

  anari_cpp::release(d, d);
}

void CheckRejected(viskores::interop::anari::ANARIMapperGlyphs& mapper, const char* what)
{
  const auto& params = mapper.GetANARIGlyphsParameters();
  VISKORES_TEST_ASSERT(mapper.GroupIsEmpty(), "Group not empty for ", what);
  VISKORES_TEST_ASSERT(params.Vertex.Position == nullptr, "vertex.position for ", what);
  VISKORES_TEST_ASSERT(params.Vertex.Radius == nullptr, "vertex.radius for ", what);
  VISKORES_TEST_ASSERT(params.NumPrimitives == 0, "Primitives counted for ", what);
  // Building the rest of the ANARI objects must not fail either.
  mapper.GetANARIGroup();
}

// A field that is not a 3-component vector gives an empty group, not an
// exception.
void UnsupportedFieldTests()
{
  auto d = loadANARIDevice();

  auto dataSet = MakeVertexDataSet({ { 0.f, 0.f, 0.f }, { 1.f, 0.f, 0.f } });
  dataSet.AddPointField("scalars", std::vector<viskores::Float32>{ 1.f, 2.f });
  dataSet.AddPointField("vec2", std::vector<viskores::Vec2f_32>{ { 1.f, 0.f }, { 0.f, 1.f } });
  dataSet.AddPointField("vectors",
                        std::vector<viskores::Vec3f_32>{ { 0.f, 0.f, 1.f }, { 0.f, 0.f, 1.f } });

  auto makeActor = [&](const char* fieldName)
  {
    return viskores::interop::anari::ANARIActor(
      dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField(fieldName));
  };

  {
    viskores::interop::anari::ANARIMapperGlyphs mapper(d, makeActor("scalars"));
    mapper.GetANARISurface();
    CheckRejected(mapper, "a scalar field");
  }

  {
    viskores::interop::anari::ANARIMapperGlyphs mapper(d, makeActor("vec2"));
    mapper.GetANARISurface();
    CheckRejected(mapper, "a Vec2 field");
  }

  // Switching from a valid actor to a scalar field drops the old glyphs.
  {
    viskores::interop::anari::ANARIMapperGlyphs mapper(d, makeActor("vectors"));
    mapper.GetANARISurface();
    VISKORES_TEST_ASSERT(!mapper.GroupIsEmpty(), "A Vec3 field was rejected");
    mapper.SetActor(makeActor("scalars"));
    CheckRejected(mapper, "a scalar field after a valid one");
  }

  anari_cpp::release(d, d);
}

// A single point has zero-size bounds, which must not give zero-size glyphs.
void SinglePointTests()
{
  auto d = loadANARIDevice();

  auto dataSet = MakeVertexDataSet({ { 1.f, 2.f, 3.f } });
  dataSet.AddPointField("vectors", std::vector<viskores::Vec3f_32>{ { 0.f, 0.f, 1.f } });

  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("vectors"));
  viskores::interop::anari::ANARIMapperGlyphs mapper(d, actor);
  mapper.GetANARISurface();

  const auto& params = mapper.GetANARIGlyphsParameters();
  VISKORES_TEST_ASSERT(params.NumPrimitives == 1, "Expected 1 glyph");
  const auto vertices = ReadANARIArray<viskores::Vec3f_32>(d, params.Vertex.Position, 4);
  const auto radii = ReadANARIArray<viskores::Float32>(d, params.Vertex.Radius, 4);
  VISKORES_TEST_ASSERT(vertices[0][2] > 3.f, "A single point got a zero-length glyph");
  VISKORES_TEST_ASSERT(radii[0] > 0.f, "A single point got a zero-radius glyph");

  anari_cpp::release(d, d);
}

void RenderTests()
{
  // Initialize ANARI /////////////////////////////////////////////////////////

  auto d = loadANARIDevice();

  // Check for the KHR_GEOMETRY_CONE extension.

  const char** extensions = nullptr;
  anariGetProperty(d, d, "extension", ANARI_STRING_LIST, &extensions, sizeof(char**), ANARI_WAIT);
  bool conesSupported = false;
  for (int i = 0; extensions[i] != nullptr; ++i)
  {
    if (extensions[i] == std::string("ANARI_KHR_GEOMETRY_CONE"))
    {
      conesSupported = true;
      break;
    }
  }
  if (!conesSupported)
  {
    VISKORES_TEST_SKIP("ANARI KHR_GEOMETRY_CONE extension not supported by ANARI device.");
  }

  // Create Viskores datasets /////////////////////////////////////////////////////

  viskores::source::Tangle source;
  source.SetPointDimensions({ 32 });
  auto tangle = source.Execute();

  viskores::filter::vector_analysis::Gradient gradientFilter;
  gradientFilter.SetActiveField("tangle");
  gradientFilter.SetOutputFieldName("Gradient");
  auto tangleGrad = gradientFilter.Execute(tangle);

  // Map data to ANARI objects ////////////////////////////////////////////////

  auto world = anari_cpp::newObject<anari_cpp::World>(d);

  viskores::interop::anari::ANARIActor actor(
    tangleGrad.GetCellSet(), tangleGrad.GetCoordinateSystem(), tangleGrad.GetField("Gradient"));

  viskores::interop::anari::ANARIMapperGlyphs mGlyphs(d, actor);

  auto surface = mGlyphs.GetANARISurface();
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);
  anari_cpp::commitParameters(d, world);

  // Render a frame ///////////////////////////////////////////////////////////

  renderTestANARIImage(d,
                       world,
                       viskores::Vec3f_32(0.5f, 1.f, 0.6f),
                       viskores::Vec3f_32(0.f, -1.f, 0.f),
                       viskores::Vec3f_32(0.f, 0.f, 1.f),
                       "interop/anari/glyphs.png");

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

void GlyphsTests()
{
  OffsetToggleTests();
  ZeroVectorTests();
  UnsupportedFieldTests();
  SinglePointTests();
  RenderTests();
}

} // namespace

int UnitTestANARIMapperGlyphs(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(GlyphsTests, argc, argv);
}
