//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandleSOAStride.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

viskores::cont::ArrayHandle<viskores::Vec3f> GetColors()
{
  return viskores::cont::make_ArrayHandle<viskores::Vec3f>(
    { { 0.f, 0.5f, 1.f }, { 1.f, 0.5f, 0.f } });
}

viskores::cont::ArrayHandle<viskores::FloatDefault> GetOpacity()
{
  return viskores::cont::make_ArrayHandle<viskores::FloatDefault>({ 0.5f, 1.0f });
}

void CheckArray(const viskores::cont::UnknownArrayHandle& array)
{
  array.PrintSummary(std::cout);

  VISKORES_TEST_ASSERT(
    test_equal_ArrayHandles(array,
                            viskores::cont::make_ArrayHandle<viskores::Vec4f_64>(
                              { { 0.0, 0.5, 1.0, 0.5 }, { 1.0, 0.5, 0.0, 1.0 } })));
}

void MakeArrayHandleSOAStride()
{
  ////
  //// BEGIN-EXAMPLE MakeArrayHandleSOAStride
  ////
  viskores::cont::ArrayHandle<viskores::Vec3f> colors = GetColors();
  viskores::cont::ArrayHandle<viskores::FloatDefault> opacities = GetOpacity();

  viskores::cont::ArrayHandleSOAStride<viskores::Vec4f> rgba =
    viskores::cont::make_ArrayHandleSOAStride(
      viskores::cont::ArrayExtractComponent(colors, 0),
      viskores::cont::ArrayExtractComponent(colors, 1),
      viskores::cont::ArrayExtractComponent(colors, 2),
      viskores::cont::ArrayExtractComponent(opacities, 0));
  ////
  //// END-EXAMPLE MakeArrayHandleSOAStride
  ////
  CheckArray(rgba);
}

void Test()
{
  MakeArrayHandleSOAStride();
}

} // anonymous namespace

int GuideExampleArrayHandleSOAStride(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
