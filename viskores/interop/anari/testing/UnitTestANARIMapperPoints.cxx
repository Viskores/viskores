//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


// viskores::anari
#include <viskores/interop/anari/ANARIMapperPoints.h>
// viskores
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/io/EncodePNG.h>
#include <viskores/source/Tangle.h>
// std
#include <cstdlib>
#include <vector>

#include "ANARITestCommon.h"

namespace
{

// Four points used by two triangle cells, with a scalar and a vector point
// field and a cell field. Two cells and four points means a cell field cannot
// be mistaken for one value per point.
viskores::cont::DataSet MakePointsDataSet()
{
  const std::vector<viskores::Vec3f_32> coords = {
    { 0.f, 0.f, 0.f }, { 1.f, 0.f, 0.f }, { 1.f, 1.f, 0.f }, { 0.f, 1.f, 0.f }
  };
  const std::vector<viskores::UInt8> shapes = { viskores::CELL_SHAPE_TRIANGLE,
                                                viskores::CELL_SHAPE_TRIANGLE };
  const std::vector<viskores::IdComponent> numIndices = { 3, 3 };
  const std::vector<viskores::Id> connectivity = { 0, 1, 2, 0, 2, 3 };

  auto dataSet =
    viskores::cont::DataSetBuilderExplicit::Create(coords, shapes, numIndices, connectivity);
  dataSet.AddPointField("pointValues", std::vector<viskores::Float32>{ 1.f, 2.f, 3.f, 4.f });
  dataSet.AddPointField(
    "pointVectors",
    std::vector<viskores::Vec3f_32>{
      { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f, 1.f } });
  dataSet.AddCellField("cellValues", std::vector<viskores::Float32>{ 10.f, 20.f });
  return dataSet;
}

// Render a small frame of `surface` looking down -Z at `center`, and return the
// pixel at the middle of the image and the one in a corner.
std::pair<viskores::Vec4f_32, viskores::Vec4f_32> RenderSurface(anari_cpp::Device d,
                                                                anari_cpp::Surface surface,
                                                                const viskores::Vec3f_32& center)
{
  auto world = anari_cpp::newObject<anari_cpp::World>(d);
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);
  anari_cpp::commitParameters(d, world);

  auto renderer = anari_cpp::newObject<anari_cpp::Renderer>(d, "default");
  anari_cpp::setParameter(d, renderer, "background", viskores::Vec4f_32(0.f, 0.f, 0.f, 1.f));
  anari_cpp::commitParameters(d, renderer);

  auto camera = anari_cpp::newObject<anari_cpp::Camera>(d, "perspective");
  anari_cpp::setParameter(d, camera, "position", center + viskores::Vec3f_32(0.f, 0.f, 10.f));
  anari_cpp::setParameter(d, camera, "direction", viskores::Vec3f_32(0.f, 0.f, -1.f));
  anari_cpp::setParameter(d, camera, "up", viskores::Vec3f_32(0.f, 1.f, 0.f));
  anari_cpp::commitParameters(d, camera);

  constexpr viskores::UInt32 size = 64;
  auto frame = anari_cpp::newObject<anari_cpp::Frame>(d);
  anari_cpp::setParameter(d, frame, "size", viskores::Vec2ui_32(size, size));
  anari_cpp::setParameter(d, frame, "channel.color", ANARI_FLOAT32_VEC4);
  anari_cpp::setParameter(d, frame, "world", world);
  anari_cpp::setParameter(d, frame, "camera", camera);
  anari_cpp::setParameter(d, frame, "renderer", renderer);
  anari_cpp::commitParameters(d, frame);

  anari_cpp::render(d, frame);
  anari_cpp::wait(d, frame);

  const auto fb = anari_cpp::map<viskores::Vec4f_32>(d, frame, "channel.color");
  VISKORES_TEST_ASSERT(fb.data != nullptr && fb.width == size && fb.height == size,
                       "Could not map the frame");
  const auto middle = fb.data[(size / 2) * size + size / 2];
  const auto corner = fb.data[0];
  anari_cpp::unmap(d, frame, "channel.color");

  anari_cpp::release(d, frame);
  anari_cpp::release(d, camera);
  anari_cpp::release(d, renderer);
  anari_cpp::release(d, world);

  return { middle, corner };
}

// Only fields with one value per point become vertex attributes. A cell field
// is skipped, and every attribute that is attached gets its name.
void FieldTests()
{
  auto d = loadANARIDevice();
  auto dataSet = MakePointsDataSet();

  viskores::interop::anari::ANARIActor actor(dataSet, "cellValues", "pointValues", "pointVectors");
  viskores::interop::anari::ANARIMapperPoints mapper(d, actor);
  mapper.GetANARIGeometry();

  const auto& params = mapper.GetANARIPointsParameters();
  VISKORES_TEST_ASSERT(params.NumPrimitives == 4, "Expected 4 spheres");
  VISKORES_TEST_ASSERT(params.Radius > 0.f, "Expected a positive sphere radius");

  const auto positions = ReadANARIArray<viskores::Vec3f_32>(d, params.Vertex.Position, 4);
  const auto coords = dataSet.GetCoordinateSystem().GetDataAsMultiplexer().ReadPortal();
  for (viskores::Id pointIndex = 0; pointIndex < 4; ++pointIndex)
  {
    VISKORES_TEST_ASSERT(
      test_equal(positions[static_cast<std::size_t>(pointIndex)], coords.Get(pointIndex)),
      "vertex.position does not match the coordinates");
  }

  VISKORES_TEST_ASSERT(params.Vertex.Attribute[0] == nullptr,
                       "A cell field was sent as a vertex attribute");
  VISKORES_TEST_ASSERT(params.Vertex.AttributeName[0].empty(),
                       "A skipped cell field still has an attribute name");

  const auto values = ReadANARIArray<viskores::Float32>(d, params.Vertex.Attribute[1], 4);
  VISKORES_TEST_ASSERT(values == std::vector<viskores::Float32>({ 1.f, 2.f, 3.f, 4.f }),
                       "vertex.attribute1 does not hold the point field");
  VISKORES_TEST_ASSERT(params.Vertex.AttributeName[1] == "pointValues", "Wrong attribute 1 name");

  const auto vectors = ReadANARIArray<viskores::Vec3f_32>(d, params.Vertex.Attribute[2], 4);
  VISKORES_TEST_ASSERT(test_equal(vectors[3], viskores::Vec3f_32(1.f, 1.f, 1.f)),
                       "vertex.attribute2 does not hold the vector point field");
  VISKORES_TEST_ASSERT(params.Vertex.AttributeName[2] == "pointVectors", "Wrong attribute 2 name");

  // Render once so the device reads every array; only a crash or error fails here.
  RenderSurface(d, mapper.GetANARISurface(), viskores::Vec3f_32(0.5f, 0.5f, 0.f));

  anari_cpp::release(d, d);
}

// A lone point has bounds of zero size; it must still get a visible sphere.
void SinglePointTests()
{
  auto d = loadANARIDevice();

  const viskores::Vec3f_32 point(1.f, 2.f, 3.f);
  auto dataSet = viskores::cont::DataSetBuilderExplicit::Create(
    std::vector<viskores::Vec3f_32>{ point },
    std::vector<viskores::UInt8>{ viskores::CELL_SHAPE_VERTEX },
    std::vector<viskores::IdComponent>{ 1 },
    std::vector<viskores::Id>{ 0 });

  viskores::interop::anari::ANARIActor actor(dataSet);
  viskores::interop::anari::ANARIMapperPoints mapper(d, actor);
  mapper.GetANARIGeometry();

  const auto& params = mapper.GetANARIPointsParameters();
  VISKORES_TEST_ASSERT(params.NumPrimitives == 1, "Expected 1 sphere");
  VISKORES_TEST_ASSERT(params.Radius > 0.f, "A single point got a sphere radius of 0");

  const auto pixels = RenderSurface(d, mapper.GetANARISurface(), point);
  VISKORES_TEST_ASSERT(!test_equal(pixels.first, pixels.second),
                       "The sphere for a single point is not visible: middle pixel ",
                       pixels.first,
                       " matches the background ",
                       pixels.second);

  anari_cpp::release(d, d);
}

void RenderTests()
{
  // Initialize ANARI /////////////////////////////////////////////////////////

  auto d = loadANARIDevice();

  // Create Viskores datasets /////////////////////////////////////////////////////

  viskores::source::Tangle source;
  source.SetPointDimensions({ 32 });
  auto tangle = source.Execute();

  auto& tangle_field = tangle.GetField("tangle");
  viskores::Range range;
  tangle_field.GetRange(&range);
  const auto isovalue = range.Center();

  viskores::filter::contour::Contour contourFilter;
  contourFilter.SetIsoValue(isovalue);
  contourFilter.SetActiveField("tangle");
  auto tangleIso = contourFilter.Execute(tangle);

  // Map data to ANARI objects ////////////////////////////////////////////////

  auto world = anari_cpp::newObject<anari_cpp::World>(d);

  viskores::interop::anari::ANARIActor actor(
    tangleIso.GetCellSet(), tangleIso.GetCoordinateSystem(), tangleIso.GetField("tangle"));

  viskores::interop::anari::ANARIMapperPoints mIso(d, actor);
  setColorMap(d, mIso);

  auto surface = mIso.GetANARISurface();
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);

  anari_cpp::commitParameters(d, world);

  // Render a frame ///////////////////////////////////////////////////////////

  renderTestANARIImage(d,
                       world,
                       viskores::Vec3f_32(-0.05, 1.43, 1.87),
                       viskores::Vec3f_32(0.32, -0.53, -0.79),
                       viskores::Vec3f_32(-0.20, -0.85, 0.49),
                       "interop/anari/points.png");

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

void PointsTests()
{
  FieldTests();
  SinglePointTests();
  RenderTests();
}

} // namespace

int UnitTestANARIMapperPoints(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(PointsTests, argc, argv);
}
