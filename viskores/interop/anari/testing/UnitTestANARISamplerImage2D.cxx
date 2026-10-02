//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleGroupVec.h>
#include <viskores/cont/CellSetSingleType.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/io/ImageReaderPNG.h>
#include <viskores/io/VTKDataSetReader.h>

#include "ANARITestCommon.h"

namespace
{

void NoopANARIDeleter(const void*, const void*) {}

template <typename T>
anari_cpp::Array1D ViskoresArray2Anari(anari_cpp::Device d,
                                       const viskores::cont::ArrayHandle<T>& viskoresArray,
                                       viskores::cont::Token& token)
{
  viskores::cont::ArrayHandleBasic<T> basicArray = viskoresArray;
  // It would be possible to set up a better deleter that would manage the Viskores array with a
  // token, but for now just assume the token will be around just long enough.
  return anari_cpp::newArray1D(
    d, basicArray.GetReadPointer(token), NoopANARIDeleter, nullptr, basicArray.GetNumberOfValues());
}

template <typename T>
anari_cpp::Array1D ViskoresArray2Anari(anari_cpp::Device d,
                                       const viskores::cont::UnknownArrayHandle& viskoresArray,
                                       viskores::cont::Token& token)
{
  viskores::cont::ArrayHandle<T> concreteArray;
  viskores::cont::ArrayCopyShallowIfPossible(viskoresArray, concreteArray);
  return ViskoresArray2Anari(d, concreteArray, token);
}

template <typename T>
anari_cpp::Array1D ViskoresArray2Anari(anari_cpp::Device d,
                                       const viskores::cont::Field& viskoresField,
                                       viskores::cont::Token& token)
{
  return ViskoresArray2Anari<T>(d, viskoresField.GetData(), token);
}

template <typename T>
anari_cpp::Array2D ViskoresArray2Anari(anari_cpp::Device d,
                                       const viskores::cont::ArrayHandle<T>& viskoresArray,
                                       viskores::Id2 size,
                                       viskores::cont::Token& token)
{
  viskores::cont::ArrayHandleBasic<T> basicArray = viskoresArray;
  VISKORES_ASSERT(size[0] * size[1] == basicArray.GetNumberOfValues());
  // It would be possible to set up a better deleter that would manage the Viskores array with a
  // token, but for now just assume the token will be around just long enough.
  return anari_cpp::newArray2D(
    d, basicArray.GetReadPointer(token), NoopANARIDeleter, nullptr, size[0], size[1]);
}

template <typename T>
anari_cpp::Array2D ViskoresArray2Anari(anari_cpp::Device d,
                                       const viskores::cont::UnknownArrayHandle& viskoresArray,
                                       viskores::Id2 size,
                                       viskores::cont::Token& token)
{
  viskores::cont::ArrayHandle<T> concreteArray;
  viskores::cont::ArrayCopyShallowIfPossible(viskoresArray, concreteArray);
  return ViskoresArray2Anari(d, concreteArray, size, token);
}

template <typename T>
anari_cpp::Array2D ViskoresArray2Anari(anari_cpp::Device d,
                                       const viskores::cont::Field& viskoresField,
                                       viskores::Id2 size,
                                       viskores::cont::Token& token)
{
  return ViskoresArray2Anari<T>(d, viskoresField.GetData(), size, token);
}

bool HasImage2DSamplerExtension(anari_cpp::Device d)
{
  const char** extensions = nullptr;
  anariGetProperty(d, d, "extension", ANARI_STRING_LIST, &extensions, sizeof(char**), ANARI_WAIT);
  for (int i = 0; extensions != nullptr && extensions[i] != nullptr; ++i)
  {
    if (std::string(extensions[i]) == "KHR_SAMPLER_IMAGE2D" ||
        std::string(extensions[i]) == "ANARI_KHR_SAMPLER_IMAGE2D")
      return true;
  }
  return false;
}

void RenderTests()
{
  auto d = loadANARIDevice();
  if (!HasImage2DSamplerExtension(d))
  {
    VISKORES_TEST_SKIP("ANARI KHR_SAMPLER_IMAGE2D extension not supported by ANARI device.");
  }

  viskores::io::VTKDataSetReader sphereReader(
    viskores::testing::Testing::DataPath("unstructured/sphere.vtk"));
  const auto& sphere = sphereReader.ReadDataSet();
  const auto cells = sphere.GetCellSet().AsCellSet<viskores::cont::CellSetSingleType<>>();
  const auto connectivity = cells.GetConnectivityArray(viskores::TopologyElementTagCell{},
                                                       viskores::TopologyElementTagPoint{});

  viskores::cont::Token token;
  auto vertexPositions =
    ViskoresArray2Anari<viskores::Vec3f_32>(d, sphere.GetCoordinateSystem(), token);
  auto vertexTextureCoordinates =
    ViskoresArray2Anari<viskores::Vec2f_32>(d, sphere.GetPointField("TextureCoordinates"), token);

  viskores::cont::ArrayHandle<viskores::Vec3ui_32> triangleConnections;
  viskores::cont::ArrayCopy(viskores::cont::make_ArrayHandleGroupVec<3>(connectivity),
                            triangleConnections);
  auto triangleIndices = ViskoresArray2Anari(d, triangleConnections, token);

  viskores::io::ImageReaderPNG imageReader(
    viskores::testing::Testing::DataPath("uniform/viskores-icon.png"));
  const auto& image = imageReader.ReadDataSet();

  auto imageArray = ViskoresArray2Anari<viskores::Vec4f_32>(
    d, image.GetPointField(imageReader.GetPointFieldName()), viskores::Id2{ 1024, 1024 }, token);

  auto geometry = anari_cpp::newObject<anari_cpp::Geometry>(d, "triangle");
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.position", vertexPositions);
  anari_cpp::setAndReleaseParameter(d, geometry, "vertex.attribute0", vertexTextureCoordinates);
  anari_cpp::setAndReleaseParameter(d, geometry, "primitive.index", triangleIndices);
  anari_cpp::commitParameters(d, geometry);

  auto sampler = anari_cpp::newObject<anari_cpp::Sampler>(d, "image2D");
  anari_cpp::setAndReleaseParameter(d, sampler, "image", imageArray);
  anari_cpp::setParameter(d, sampler, "filter", "linear");
  anari_cpp::setParameter(d, sampler, "wrapMode", "clampToEdge");
  anari_cpp::setParameter(d, sampler, "inAttribute", "attribute0");
  anari_cpp::commitParameters(d, sampler);

  auto material = anari_cpp::newObject<anari_cpp::Material>(d, "matte");
  anari_cpp::setParameter(d, material, "color", sampler);
  anari_cpp::commitParameters(d, material);
  anari_cpp::release(d, sampler);

  auto surface = anari_cpp::newObject<anari_cpp::Surface>(d);
  anari_cpp::setParameter(d, surface, "geometry", geometry);
  anari_cpp::setParameter(d, surface, "material", material);
  anari_cpp::commitParameters(d, surface);
  anari_cpp::release(d, geometry);
  anari_cpp::release(d, material);

  auto world = anari_cpp::newObject<anari_cpp::World>(d);
  anari_cpp::setParameterArray1D(d, world, "surface", &surface, 1);
  anari_cpp::commitParameters(d, world);
  anari_cpp::release(d, surface);

  renderTestANARIImage(d,
                       world,
                       viskores::Vec3f_32(0.f, -1.1f, 0.f),
                       viskores::Vec3f_32(0.f, 1.f, 0.f),
                       viskores::Vec3f_32(0.f, 0.f, -1.f),
                       "interop/anari/sampler-image2d.png",
                       viskores::Vec2ui_32(512, 512));

  anari_cpp::release(d, world);
  anari_cpp::release(d, d);
}

} // namespace

int UnitTestANARISamplerImage2D(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RenderTests, argc, argv);
}
