//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/filter/image_processing/SSIM.h>

#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/ErrorFilterExecution.h>
#include <viskores/cont/testing/Testing.h>

#include <vector>

namespace
{

viskores::cont::DataSet MakePointDataSet(const std::vector<viskores::Float64>& primary,
                                         const std::vector<viskores::Float64>& secondary)
{
  viskores::cont::DataSet dataSet =
    viskores::cont::DataSetBuilderUniform::Create(viskores::Id2{ 2, 2 });
  dataSet.AddPointField("primary", primary);
  dataSet.AddPointField("secondary", secondary);
  return dataSet;
}

template <typename PrimaryType, typename SecondaryType>
viskores::cont::DataSet MakePointDataSet(const std::vector<PrimaryType>& primary,
                                         const std::vector<SecondaryType>& secondary)
{
  viskores::cont::DataSet dataSet =
    viskores::cont::DataSetBuilderUniform::Create(viskores::Id2{ 2, 2 });
  dataSet.AddPointField("primary", primary);
  dataSet.AddPointField("secondary", secondary);
  return dataSet;
}

viskores::cont::DataSet MakeWholeDataSet(const std::vector<viskores::Float64>& primary,
                                         const std::vector<viskores::Float64>& secondary)
{
  viskores::cont::DataSet dataSet;
  dataSet.AddField(viskores::cont::make_Field(
    "primary", viskores::cont::Field::Association::WholeDataSet, primary, viskores::CopyFlag::On));
  dataSet.AddField(viskores::cont::make_Field("secondary",
                                              viskores::cont::Field::Association::WholeDataSet,
                                              secondary,
                                              viskores::CopyFlag::On));
  return dataSet;
}

viskores::cont::ArrayHandle<viskores::Float64> ReadSSIMValues(
  const viskores::cont::DataSet& dataSet)
{
  VISKORES_TEST_ASSERT(dataSet.HasField("ssim", viskores::cont::Field::Association::Points),
                       "SSIM output field is missing.");

  viskores::cont::ArrayHandle<viskores::Float64> output;
  dataSet.GetField("ssim", viskores::cont::Field::Association::Points)
    .GetData()
    .AsArrayHandle(output);

  return output;
}

void CheckSSIMValues(const viskores::cont::DataSet& dataSet,
                     const std::vector<viskores::Float64>& expected)
{
  auto output = ReadSSIMValues(dataSet);
  VISKORES_TEST_ASSERT(output.GetNumberOfValues() == static_cast<viskores::Id>(expected.size()),
                       "SSIM output field has wrong size.");

  auto portal = output.ReadPortal();
  for (viskores::Id index = 0; index < output.GetNumberOfValues(); ++index)
  {
    const viskores::Float64 actual = portal.Get(index);
    const viskores::Float64 expectedValue = expected[static_cast<std::size_t>(index)];
    VISKORES_TEST_ASSERT(test_equal(actual, expectedValue),
                         "Wrong pointwise SSIM value at index ",
                         index,
                         ". Expected ",
                         expectedValue,
                         " but got ",
                         actual,
                         ".");
  }
}

void CheckAllSSIMValues(const viskores::cont::DataSet& dataSet, viskores::Float64 expected)
{
  auto output = ReadSSIMValues(dataSet);
  std::vector<viskores::Float64> expectedValues(
    static_cast<std::size_t>(output.GetNumberOfValues()), expected);
  CheckSSIMValues(dataSet, expectedValues);
}

void TestMatchingFields()
{
  viskores::filter::image_processing::SSIM filter;
  filter.SetPrimaryField("primary");
  filter.SetSecondaryField("secondary");

  const viskores::cont::DataSet dataSet = MakePointDataSet({ 0, 1, 2, 3 }, { 0, 1, 2, 3 });
  const viskores::cont::DataSet result = filter.Execute(dataSet);

  CheckAllSSIMValues(result, 1.0);
  VISKORES_TEST_ASSERT(test_equal(filter.ComputeMetric(dataSet), 1.0),
                       "Wrong SSIM from ComputeMetric.");
}

void TestConstantOffsetFields()
{
  viskores::filter::image_processing::SSIM filter;
  filter.SetPrimaryField("primary");
  filter.SetSecondaryField("secondary");

  const viskores::cont::DataSet dataSet = MakePointDataSet({ 1, 1, 1, 1 }, { 2, 2, 2, 2 });
  const viskores::cont::DataSet result = filter.Execute(dataSet);

  const viskores::Float64 c1 = 0.01 * 0.01;
  const viskores::Float64 expected = (4.0 + c1) / (5.0 + c1);
  CheckAllSSIMValues(result, expected);
  VISKORES_TEST_ASSERT(test_equal(filter.ComputeMetric(dataSet), expected),
                       "Wrong average SSIM for constant offset fields.");
}

void TestMatchingColorFields()
{
  viskores::filter::image_processing::SSIM filter;
  filter.SetPrimaryField("primary");
  filter.SetSecondaryField("secondary");

  const std::vector<viskores::Vec3f> primary = {
    { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 }, { 9, 10, 11 }
  };
  const std::vector<viskores::Vec3f> secondary = primary;
  const viskores::cont::DataSet dataSet = MakePointDataSet(primary, secondary);
  const viskores::cont::DataSet result = filter.Execute(dataSet);

  CheckAllSSIMValues(result, 1.0);
}

void TestConstantOffsetColorFields()
{
  viskores::filter::image_processing::SSIM filter;
  filter.SetPrimaryField("primary");
  filter.SetSecondaryField("secondary");

  const std::vector<viskores::Vec3f> primary = {
    { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }, { 1, 1, 1 }
  };
  const std::vector<viskores::Vec3f> secondary = {
    { 2, 2, 2 }, { 2, 2, 2 }, { 2, 2, 2 }, { 2, 2, 2 }
  };
  const viskores::cont::DataSet dataSet = MakePointDataSet(primary, secondary);
  const viskores::cont::DataSet result = filter.Execute(dataSet);

  const viskores::Float64 c1 = 0.01 * 0.01;
  const viskores::Float64 expected = (4.0 + c1) / (5.0 + c1);
  CheckAllSSIMValues(result, expected);
}

void TestMatchingRGBA8Fields()
{
  viskores::filter::image_processing::SSIM filter;
  filter.SetPrimaryField("primary");
  filter.SetSecondaryField("secondary");
  filter.SetDynamicRange(255);

  const std::vector<viskores::Vec4ui_8> primary = {
    { 0, 128, 255, 255 }, { 255, 64, 32, 255 }, { 32, 64, 128, 255 }, { 255, 255, 255, 255 }
  };
  const std::vector<viskores::Vec4ui_8> secondary = primary;
  const viskores::cont::DataSet dataSet = MakePointDataSet(primary, secondary);
  const viskores::cont::DataSet result = filter.Execute(dataSet);

  CheckAllSSIMValues(result, 1.0);
}

void TestPatchRadius()
{
  viskores::filter::image_processing::SSIM filter;
  filter.SetPrimaryField("primary");
  filter.SetSecondaryField("secondary");
  filter.SetPatchRadius(0);

  const viskores::cont::DataSet dataSet = MakePointDataSet({ 0, 1, 1, 1 }, { 0, 0, 1, 1 });
  const viskores::cont::DataSet result = filter.Execute(dataSet);

  const viskores::Float64 c1 = 0.01 * 0.01;
  const viskores::Float64 mismatch = c1 / (1.0 + c1);
  CheckSSIMValues(result, { 1.0, mismatch, 1.0, 1.0 });
  VISKORES_TEST_ASSERT(test_equal(filter.ComputeMetric(dataSet), (3.0 + mismatch) / 4.0),
                       "Wrong average SSIM for radius-0 fields.");
}

void TestNonPointFields()
{
  viskores::filter::image_processing::SSIM filter;
  filter.SetPrimaryField("primary", viskores::cont::Field::Association::WholeDataSet);
  filter.SetSecondaryField("secondary", viskores::cont::Field::Association::WholeDataSet);

  bool threw = false;
  try
  {
    filter.Execute(MakeWholeDataSet({ 0, 1, 2, 3 }, { 0, 1, 2 }));
  }
  catch (const viskores::cont::ErrorFilterExecution&)
  {
    threw = true;
  }
  VISKORES_TEST_ASSERT(threw, "Non-point SSIM fields should throw.");
}

void TestMismatchedColorComponents()
{
  viskores::filter::image_processing::SSIM filter;
  filter.SetPrimaryField("primary");
  filter.SetSecondaryField("secondary");

  const std::vector<viskores::Vec3f> primary = {
    { 0, 1, 2 }, { 3, 4, 5 }, { 6, 7, 8 }, { 9, 10, 11 }
  };
  const std::vector<viskores::Vec4f> secondary = {
    { 0, 1, 2, 3 }, { 4, 5, 6, 7 }, { 8, 9, 10, 11 }, { 12, 13, 14, 15 }
  };

  bool threw = false;
  try
  {
    filter.Execute(MakePointDataSet(primary, secondary));
  }
  catch (const viskores::cont::ErrorFilterExecution&)
  {
    threw = true;
  }
  VISKORES_TEST_ASSERT(threw, "SSIM fields with mismatched component counts should throw.");
}

void TestNegativePatchRadius()
{
  viskores::filter::image_processing::SSIM filter;
  filter.SetPrimaryField("primary");
  filter.SetSecondaryField("secondary");
  filter.SetPatchRadius(-1);

  bool threw = false;
  try
  {
    filter.Execute(MakePointDataSet({ 0, 1, 2, 3 }, { 0, 1, 2, 3 }));
  }
  catch (const viskores::cont::ErrorFilterExecution&)
  {
    threw = true;
  }
  VISKORES_TEST_ASSERT(threw, "Negative SSIM patch radii should throw.");
}

void TestSSIM()
{
  TestMatchingFields();
  TestConstantOffsetFields();
  TestMatchingColorFields();
  TestConstantOffsetColorFields();
  TestMatchingRGBA8Fields();
  TestPatchRadius();
  TestNonPointFields();
  TestMismatchedColorComponents();
  TestNegativePatchRadius();
}

} // anonymous namespace

int UnitTestSSIMFilter(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestSSIM, argc, argv);
}
