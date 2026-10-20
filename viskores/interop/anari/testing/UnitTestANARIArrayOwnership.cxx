//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

// viskores::anari
#include <viskores/interop/anari/ANARIMapperGlyphs.h>
#include <viskores/interop/anari/ANARIMapperPoints.h>
#include <viskores/interop/anari/ANARIMapperTriangles.h>
#include <viskores/interop/anari/ANARIMapperVolume.h>
// viskores
#include <viskores/cont/ArrayHandleBasic.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/vector_analysis/Gradient.h>
#include <viskores/source/Tangle.h>
// std
#include <algorithm>
#include <vector>

#include "ANARITestCommon.h"

namespace
{

// Host memory whose release the test can observe ////////////////////////////

viskores::IdComponent NumTrackedBuffersFreed = 0;

template <typename T>
void TrackedDeleter(void* memory)
{
  ++NumTrackedBuffersFreed;
  delete[] static_cast<T*>(memory);
}

template <typename T>
viskores::cont::ArrayHandleBasic<T> MakeTrackedArray(const std::vector<T>& values)
{
  T* memory = new T[values.size()];
  std::copy(values.begin(), values.end(), memory);
  return viskores::cont::ArrayHandleBasic<T>(
    memory, static_cast<viskores::Id>(values.size()), TrackedDeleter<T>);
}

std::vector<viskores::Float32> MakeRamp(std::size_t size)
{
  std::vector<viskores::Float32> values(size);
  for (std::size_t valueIndex = 0; valueIndex < size; ++valueIndex)
  {
    values[valueIndex] = static_cast<viskores::Float32>(valueIndex);
  }
  return values;
}

// Render a small frame of `world` so the device reads every array it holds.
void RenderFrame(anari_cpp::Device d, anari_cpp::World world)
{
  auto renderer = anari_cpp::newObject<anari_cpp::Renderer>(d, "default");
  anari_cpp::setParameter(d, renderer, "background", viskores::Vec4f_32(0.3f, 0.3f, 0.3f, 1.f));
  anari_cpp::commitParameters(d, renderer);

  auto camera = anari_cpp::newObject<anari_cpp::Camera>(d, "perspective");
  anari_cpp::setParameter(d, camera, "position", viskores::Vec3f_32(0.5f, 0.5f, 3.f));
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
  anari_cpp::map<viskores::Vec4f_32>(d, frame, "channel.color");
  anari_cpp::unmap(d, frame, "channel.color");

  anari_cpp::release(d, frame);
  anari_cpp::release(d, camera);
  anari_cpp::release(d, renderer);
}

// Helper tests ///////////////////////////////////////////////////////////////

void CheckArrayContents(anari_cpp::Device d,
                        ANARIArray array,
                        const std::vector<viskores::Float32>& expected)
{
  const auto* values = static_cast<const viskores::Float32*>(anariMapArray(d, array));
  VISKORES_TEST_ASSERT(values != nullptr, "Could not map the ANARI array");
  VISKORES_TEST_ASSERT(std::equal(expected.begin(), expected.end(), values),
                       "ANARI array does not hold the Viskores array's values");
  anariUnmapArray(d, array);
}

void MakeANARIArrayTests()
{
  auto d = loadANARIDevice();
  const auto values = MakeRamp(8);

  NumTrackedBuffersFreed = 0;
  {
    auto array = viskores::interop::anari::MakeANARIArray1D(d, MakeTrackedArray(values));
    VISKORES_TEST_ASSERT(array != nullptr, "No 1D array created");
    VISKORES_TEST_ASSERT(NumTrackedBuffersFreed == 0,
                         "Host memory freed while its ANARI 1D array is alive");
    CheckArrayContents(d, array, values);
    anari_cpp::release(d, array);
    VISKORES_TEST_ASSERT(NumTrackedBuffersFreed == 1,
                         "Host memory not freed when its ANARI 1D array was released");
  }

  NumTrackedBuffersFreed = 0;
  {
    auto array = viskores::interop::anari::MakeANARIArray3D(d, MakeTrackedArray(values), 2, 2, 2);
    VISKORES_TEST_ASSERT(array != nullptr, "No 3D array created");
    VISKORES_TEST_ASSERT(NumTrackedBuffersFreed == 0,
                         "Host memory freed while its ANARI 3D array is alive");
    CheckArrayContents(d, array, values);
    anari_cpp::release(d, array);
    VISKORES_TEST_ASSERT(NumTrackedBuffersFreed == 1,
                         "Host memory not freed when its ANARI 3D array was released");
  }

  VISKORES_TEST_ASSERT(viskores::interop::anari::MakeANARIArray1D(
                         d, viskores::cont::ArrayHandle<viskores::Float32>{}) == nullptr,
                       "An ANARI array was created for an empty Viskores array");

  anari_cpp::release(d, d);
}

// Mapper tests ///////////////////////////////////////////////////////////////

// Retain a structured volume mapper's spatial field, destroy the mapper and its
// input, then render a world that uses the retained field.
void RetainedSpatialFieldTests()
{
  auto d = loadANARIDevice();

  NumTrackedBuffersFreed = 0;
  anari_cpp::SpatialField field = nullptr;
  {
    const viskores::Id3 dims(3, 3, 3);
    auto dataSet = viskores::cont::DataSetBuilderUniform::Create(dims);
    dataSet.AddPointField("values", MakeTrackedArray(MakeRamp(27)));
    viskores::interop::anari::ANARIActor actor(
      dataSet.GetCellSet(), dataSet.GetCoordinateSystem(), dataSet.GetField("values"));

    viskores::interop::anari::ANARIMapperVolume mVol(d, actor);
    field = mVol.GetANARISpatialField();
    VISKORES_TEST_ASSERT(field != nullptr, "No spatial field created");
    anari_cpp::retain(d, field);
  }

  VISKORES_TEST_ASSERT(NumTrackedBuffersFreed == 0,
                       "Volume data freed while the application retains the spatial field");

  auto volume = anari_cpp::newObject<anari_cpp::Volume>(d, "transferFunction1D");
  anari_cpp::setParameter(d, volume, "value", field);
  anari_cpp::setParameter(d, volume, "valueRange", viskores::Vec2f_32(0.f, 26.f));
  std::vector<viskores::Vec4f_32> colors = { { 0.f, 0.f, 1.f, 0.f }, { 1.f, 0.f, 0.f, 1.f } };
  anari_cpp::setParameterArray1D(d, volume, "color", colors.data(), colors.size());
  anari_cpp::commitParameters(d, volume);

  auto world = anari_cpp::newObject<anari_cpp::World>(d);
  anari_cpp::setParameterArray1D(d, world, "volume", &volume, 1);
  anari_cpp::commitParameters(d, world);

  RenderFrame(d, world);

  anari_cpp::release(d, world);
  anari_cpp::release(d, volume);
  anari_cpp::release(d, field);

  VISKORES_TEST_ASSERT(NumTrackedBuffersFreed == 1,
                       "Volume data not freed once the device released the spatial field");

  anari_cpp::release(d, d);
}

// Retain a mapper's geometry, destroy the mapper, then render a world that uses
// the retained geometry. The mapper owns copies of the data here, so a
// use-after-free shows up only under a memory checker (ASan or Valgrind).
template <typename MapperType>
void CheckRetainedGeometry(anari_cpp::Device d,
                           const viskores::interop::anari::ANARIActor& actor,
                           const char* mapperName)
{
  anari_cpp::Geometry geometry = nullptr;
  {
    MapperType mapper(d, actor);
    geometry = mapper.GetANARIGeometry();
    VISKORES_TEST_ASSERT(geometry != nullptr, "No geometry created by the ", mapperName, " mapper");
    anari_cpp::retain(d, geometry);
  }

  auto material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
  anari_cpp::commitParameters(d, material);

  auto surface = anari_cpp::newObject<anari_cpp::Surface>(d);
  anari_cpp::setParameter(d, surface, "geometry", geometry);
  anari_cpp::setParameter(d, surface, "material", material);
  anari_cpp::commitParameters(d, surface);

  auto world = anari_cpp::newObject<anari_cpp::World>(d);
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);
  anari_cpp::commitParameters(d, world);

  RenderFrame(d, world);

  anari_cpp::release(d, world);
  anari_cpp::release(d, surface);
  anari_cpp::release(d, material);
  anari_cpp::release(d, geometry);
}

void RetainedGeometryTests()
{
  auto d = loadANARIDevice();

  viskores::source::Tangle source;
  source.SetPointDimensions({ 8 });
  auto tangle = source.Execute();

  viskores::Range range;
  tangle.GetField("tangle").GetRange(&range);
  viskores::filter::contour::Contour contourFilter;
  contourFilter.SetIsoValue(range.Center());
  contourFilter.SetActiveField("tangle");
  auto tangleIso = contourFilter.Execute(tangle);

  viskores::interop::anari::ANARIActor isoActor(
    tangleIso.GetCellSet(), tangleIso.GetCoordinateSystem(), tangleIso.GetField("tangle"));
  CheckRetainedGeometry<viskores::interop::anari::ANARIMapperTriangles>(d, isoActor, "triangles");
  CheckRetainedGeometry<viskores::interop::anari::ANARIMapperPoints>(d, isoActor, "points");

  viskores::filter::vector_analysis::Gradient gradientFilter;
  gradientFilter.SetActiveField("tangle");
  gradientFilter.SetOutputFieldName("Gradient");
  auto tangleGrad = gradientFilter.Execute(tangle);
  viskores::interop::anari::ANARIActor gradActor(
    tangleGrad.GetCellSet(), tangleGrad.GetCoordinateSystem(), tangleGrad.GetField("Gradient"));
  CheckRetainedGeometry<viskores::interop::anari::ANARIMapperGlyphs>(d, gradActor, "glyphs");

  anari_cpp::release(d, d);
}

void ArrayOwnershipTests()
{
  MakeANARIArrayTests();
  RetainedSpatialFieldTests();
  RetainedGeometryTests();
}

} // namespace

int UnitTestANARIArrayOwnership(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(ArrayOwnershipTests, argc, argv);
}
