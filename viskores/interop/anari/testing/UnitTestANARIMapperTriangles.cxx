//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================


// viskores::anari
#include <viskores/interop/anari/ANARIMapperTriangles.h>
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

// A flat mesh of a triangle, a quad and another triangle. The quad becomes two
// triangles, so the mapper sees 4 triangles from 3 cells.
//
//   2 +-----+ 4
//    /|     |\   cell 0: triangle 0 1 2
//   / |  1  | \  cell 1: quad 1 3 4 2
//  / 0|     |2 \ cell 2: triangle 3 5 4
// +---+-----+---+
// 0   1     3   5
//
viskores::cont::DataSet MakeMixedCellDataSet()
{
  const std::vector<viskores::Vec3f_32> coords = {
    { 0.f, 0.f, 0.f }, { 1.f, 0.f, 0.f }, { 1.f, 1.f, 0.f },
    { 2.f, 0.f, 0.f }, { 2.f, 1.f, 0.f }, { 3.f, 0.f, 0.f },
  };
  const std::vector<viskores::UInt8> shapes = { viskores::CELL_SHAPE_TRIANGLE,
                                                viskores::CELL_SHAPE_QUAD,
                                                viskores::CELL_SHAPE_TRIANGLE };
  const std::vector<viskores::IdComponent> numIndices = { 3, 4, 3 };
  const std::vector<viskores::Id> connectivity = { 0, 1, 2, 1, 3, 4, 2, 3, 5, 4 };

  auto dataSet =
    viskores::cont::DataSetBuilderExplicit::Create(coords, shapes, numIndices, connectivity);
  dataSet.AddPointField("pointValues",
                        std::vector<viskores::Float32>{ 0.f, 1.5f, 3.f, 4.5f, 6.f, 7.5f });
  dataSet.AddCellField("cellValues", std::vector<viskores::Float32>{ 10.f, 20.f, 30.f });
  return dataSet;
}

// The cell of `MakeMixedCellDataSet` that a triangle came from, worked out from
// its points: only cell 0 uses point 0 and only cell 2 uses point 5.
viskores::Id CellOfTriangle(const viskores::Vec3ui_32& triangle)
{
  const auto uses = [&](viskores::UInt32 point)
  { return triangle[0] == point || triangle[1] == point || triangle[2] == point; };
  return uses(0) ? 0 : (uses(5) ? 2 : 1);
}

// Check that `index` holds 4 triangles over 6 points and that `cellAttribute`
// holds the cell value of each triangle's cell.
void CheckCellAttribute(anari_cpp::Device d,
                        const viskores::interop::anari::TrianglesParameters& params,
                        ANARIArray cellAttribute)
{
  const std::vector<viskores::Float32> cellValues = { 10.f, 20.f, 30.f };

  VISKORES_TEST_ASSERT(params.NumPrimitives == 4, "Expected 4 triangles");
  const auto triangles = ReadANARIArray<viskores::Vec3ui_32>(d, params.Primitive.Index, 4);
  const auto values = ReadANARIArray<viskores::Float32>(d, cellAttribute, 4);

  for (std::size_t triIndex = 0; triIndex < triangles.size(); ++triIndex)
  {
    const auto& triangle = triangles[triIndex];
    VISKORES_TEST_ASSERT(triangle[0] < 6 && triangle[1] < 6 && triangle[2] < 6,
                         "Triangle index out of range");
    const auto expected = cellValues[static_cast<std::size_t>(CellOfTriangle(triangle))];
    VISKORES_TEST_ASSERT(values[triIndex] == expected,
                         "Triangle ",
                         triIndex,
                         " has cell value ",
                         values[triIndex],
                         ", expected ",
                         expected);
  }
}

void CheckPositions(anari_cpp::Device d,
                    const viskores::interop::anari::TrianglesParameters& params,
                    const viskores::cont::DataSet& dataSet)
{
  const auto positions = ReadANARIArray<viskores::Vec3f_32>(d, params.Vertex.Position, 6);
  const auto coords = dataSet.GetCoordinateSystem().GetDataAsMultiplexer().ReadPortal();
  for (viskores::Id pointIndex = 0; pointIndex < 6; ++pointIndex)
  {
    VISKORES_TEST_ASSERT(
      test_equal(positions[static_cast<std::size_t>(pointIndex)], coords.Get(pointIndex)),
      "vertex.position does not match the coordinates");
  }
}

// Render a small frame of `surface` so the device reads every array it holds.
void RenderSurface(anari_cpp::Device d, anari_cpp::Surface surface)
{
  auto world = anari_cpp::newObject<anari_cpp::World>(d);
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);
  anari_cpp::commitParameters(d, world);

  auto renderer = anari_cpp::newObject<anari_cpp::Renderer>(d, "default");
  anari_cpp::commitParameters(d, renderer);

  auto camera = anari_cpp::newObject<anari_cpp::Camera>(d, "perspective");
  anari_cpp::setParameter(d, camera, "position", viskores::Vec3f_32(1.5f, 0.5f, 4.f));
  anari_cpp::setParameter(d, camera, "direction", viskores::Vec3f_32(0.f, 0.f, -1.f));
  anari_cpp::setParameter(d, camera, "up", viskores::Vec3f_32(0.f, 1.f, 0.f));
  anari_cpp::commitParameters(d, camera);

  auto frame = anari_cpp::newObject<anari_cpp::Frame>(d);
  anari_cpp::setParameter(d, frame, "size", viskores::Vec2ui_32(64, 64));
  anari_cpp::setParameter(d, frame, "channel.color", ANARI_FLOAT32_VEC4);
  anari_cpp::setParameter(d, frame, "world", world);
  anari_cpp::setParameter(d, frame, "camera", camera);
  anari_cpp::setParameter(d, frame, "renderer", renderer);
  anari_cpp::commitParameters(d, frame);

  anari_cpp::render(d, frame);
  anari_cpp::wait(d, frame);

  anari_cpp::release(d, frame);
  anari_cpp::release(d, camera);
  anari_cpp::release(d, renderer);
  anari_cpp::release(d, world);
}

// A cell field becomes one primitive attribute value per triangle, taken from
// the triangle's cell.
void CellFieldTests()
{
  auto d = loadANARIDevice();
  auto dataSet = MakeMixedCellDataSet();

  viskores::interop::anari::ANARIActor actor(
    dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("cellValues"));
  viskores::interop::anari::ANARIMapperTriangles mapper(d, actor);
  mapper.GetANARIGeometry();

  const auto& params = mapper.GetANARITrianglesParameters();
  VISKORES_TEST_ASSERT(params.Vertex.Attribute[0] == nullptr,
                       "A cell field was sent as a vertex attribute");
  VISKORES_TEST_ASSERT(params.AttributeName[0] == "cellValues", "Wrong attribute name");
  CheckPositions(d, params, dataSet);
  CheckCellAttribute(d, params, params.Primitive.Attribute[0]);

  RenderSurface(d, mapper.GetANARISurface());

  anari_cpp::release(d, d);
}

// Point and cell fields on one actor go to vertex and primitive attributes
// respectively, each in the slot of its actor field.
void MixedFieldTests()
{
  auto d = loadANARIDevice();
  auto dataSet = MakeMixedCellDataSet();

  viskores::interop::anari::ANARIActor actor(dataSet, "pointValues", "cellValues");
  viskores::interop::anari::ANARIMapperTriangles mapper(d, actor);
  mapper.SetCalculateNormals(true);
  mapper.GetANARIGeometry();

  const auto& params = mapper.GetANARITrianglesParameters();
  VISKORES_TEST_ASSERT(params.Primitive.Attribute[0] == nullptr,
                       "A point field was sent as a primitive attribute");
  VISKORES_TEST_ASSERT(params.Vertex.Attribute[1] == nullptr,
                       "A cell field was sent as a vertex attribute");
  VISKORES_TEST_ASSERT(params.AttributeName[0] == "pointValues", "Wrong attribute 0 name");
  VISKORES_TEST_ASSERT(params.AttributeName[1] == "cellValues", "Wrong attribute 1 name");

  const auto pointValues = ReadANARIArray<viskores::Float32>(d, params.Vertex.Attribute[0], 6);
  const std::vector<viskores::Float32> expectedPointValues = { 0.f, 1.5f, 3.f, 4.5f, 6.f, 7.5f };
  VISKORES_TEST_ASSERT(pointValues == expectedPointValues,
                       "vertex.attribute0 does not hold the point field");
  VISKORES_TEST_ASSERT(params.Vertex.Normal != nullptr, "No vertex normals");

  CheckPositions(d, params, dataSet);
  CheckCellAttribute(d, params, params.Primitive.Attribute[1]);

  RenderSurface(d, mapper.GetANARISurface());

  anari_cpp::release(d, d);
}

// Changing `SetCalculateNormals` after the surface exists adds or removes
// `vertex.normal` without a new actor.
void NormalsToggleTests()
{
  auto d = loadANARIDevice();
  auto dataSet = MakeMixedCellDataSet();

  viskores::interop::anari::ANARIActor actor(dataSet, "pointValues");
  viskores::interop::anari::ANARIMapperTriangles mapper(d, actor);
  auto surface = mapper.GetANARISurface();

  const auto& params = mapper.GetANARITrianglesParameters();
  VISKORES_TEST_ASSERT(params.Vertex.Normal == nullptr, "Normals made without being asked for");

  mapper.SetCalculateNormals(true);
  VISKORES_TEST_ASSERT(params.Vertex.Normal != nullptr,
                       "Enabling normals after the surface exists did not add them");
  const std::size_t numPoints = 6;
  const auto normals = ReadANARIArray<viskores::Vec3f_32>(d, params.Vertex.Normal, numPoints);
  for (const auto& normal : normals)
  {
    VISKORES_TEST_ASSERT(test_equal(viskores::Abs(normal[2]), 1.f),
                         "The normals of a flat mesh in z = 0 should point along z");
  }
  CheckPositions(d, params, dataSet);
  RenderSurface(d, surface);

  mapper.SetCalculateNormals(false);
  VISKORES_TEST_ASSERT(params.Vertex.Normal == nullptr,
                       "Disabling normals after the surface exists did not remove them");
  CheckPositions(d, params, dataSet);
  RenderSurface(d, surface);

  // SetActor builds the arrays before any geometry exists; enabling normals
  // then must still give the geometry normals once it is made.
  viskores::interop::anari::ANARIMapperTriangles lateMapper(d);
  lateMapper.SetActor(actor);
  lateMapper.SetCalculateNormals(true);
  lateMapper.GetANARIGeometry();
  VISKORES_TEST_ASSERT(lateMapper.GetANARITrianglesParameters().Vertex.Normal != nullptr,
                       "Enabling normals before the geometry exists did not add them");

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

  viskores::interop::anari::ANARIMapperTriangles mIso(d, actor);
  mIso.SetCalculateNormals(true);
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
                       "interop/anari/isosurface.png");

  // Cleanup //////////////////////////////////////////////////////////////////

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

void TrianglesTests()
{
  CellFieldTests();
  MixedFieldTests();
  NormalsToggleTests();
  RenderTests();
}

} // namespace

int UnitTestANARIMapperTriangles(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TrianglesTests, argc, argv);
}
