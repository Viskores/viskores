//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/field_transform/RGBToLab.h>

#include <vector>

namespace
{

const std::vector<viskores::Vec3f> ExpectedLabColors = {
  { 0.0f, 0.0f, 0.0f },
  { 100.0f, 0.0f, 0.0f },
  { 53.240794f, 80.092460f, 67.203197f },
  { 87.734722f, -86.182716f, 83.179321f },
  { 32.297011f, 79.187520f, -107.860162f },
};

void ValidateLabField(const viskores::cont::DataSet& result,
                      const std::string& fieldName,
                      viskores::cont::Field::Association association,
                      const std::vector<viskores::Vec3f>& expected)
{
  VISKORES_TEST_ASSERT(result.HasField(fieldName, association), "Output field missing.");

  viskores::cont::ArrayHandle<viskores::Vec3f> lab;
  result.GetField(fieldName, association).GetData().AsArrayHandle(lab);

  VISKORES_TEST_ASSERT(lab.GetNumberOfValues() == static_cast<viskores::Id>(expected.size()),
                       "Unexpected output field size.");

  auto portal = lab.ReadPortal();
  for (viskores::Id index = 0; index < lab.GetNumberOfValues(); ++index)
  {
    auto compute = portal.Get(index);
    auto expect = expected[static_cast<std::size_t>(index)];
    VISKORES_TEST_ASSERT(
      test_equal(compute, expect, 0.01), "Unexpected Lab color ", compute, " != ", expect);
  }
}

void TestRGBToLabFromBytes()
{
  std::cout << "Check byte colors" << std::endl;

  std::vector<viskores::Vec3ui_8> rgb = { { 0, 0, 0 },   { 255, 255, 255 }, { 255, 0, 0 },
                                          { 0, 255, 0 }, { 0, 0, 255 },     { 128, 128, 128 } };
  std::vector<viskores::Vec3f> expected = ExpectedLabColors;
  expected.push_back({ 53.585016f, 0.0f, 0.0f });

  viskores::cont::DataSet dataSet;
  dataSet.AddPointField("rgb", rgb);

  viskores::filter::field_transform::RGBToLab filter;
  filter.SetActiveField("rgb", viskores::cont::Field::Association::Points);
  filter.SetOutputFieldName("lab");

  ValidateLabField(
    filter.Execute(dataSet), "lab", viskores::cont::Field::Association::Points, expected);
}

void TestRGBToLabFromFloats()
{
  std::cout << "Check float colors" << std::endl;

  std::vector<viskores::Vec3f_32> rgb = {
    { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.5f, 0.5f, 0.5f },
  };
  std::vector<viskores::Vec3f> expected = ExpectedLabColors;
  expected.push_back({ 53.388967f, 0.0f, 0.0f });

  viskores::cont::DataSet dataSet;
  dataSet.AddCellField("rgb", rgb);

  viskores::filter::field_transform::RGBToLab filter;
  filter.SetActiveField("rgb", viskores::cont::Field::Association::Cells);

  ValidateLabField(
    filter.Execute(dataSet), "rgb_lab", viskores::cont::Field::Association::Cells, expected);
}

void TestRGBToLab()
{
  TestRGBToLabFromBytes();
  TestRGBToLabFromFloats();
}

} // anonymous namespace

int UnitTestRGBToLab(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestRGBToLab, argc, argv);
}
