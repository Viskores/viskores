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
    const auto vertices = ReadANARIArray<viskores::Vec3f_32>(d, params.Vertex.Position, 8);
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
  RenderTests();
}

} // namespace

int UnitTestANARIMapperGlyphs(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(GlyphsTests, argc, argv);
}
