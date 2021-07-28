//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingColorTable_h
#define vtk_m_cont_testing_TestingColorTable_h

#include <vtkm/Types.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/ColorTableMap.h>
#include <vtkm/cont/ColorTableSamples.h>
#include <vtkm/cont/testing/Testing.h>

#include <algorithm>
#include <iostream>

namespace vtkm
{
namespace cont
{
namespace testing
{

template <typename DeviceAdapterTag>
class TestingColorTable
{

  template <vtkm::IdComponent N>
  static void CheckColors(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, N>> result,
                          const std::vector<vtkm::Vec<vtkm::UInt8, N>>& expected)
  {
    using Vec = vtkm::Vec<vtkm::UInt8, N>;

    VTKM_TEST_ASSERT(result.GetNumberOfValues() == static_cast<vtkm::Id>(expected.size()));
    auto portal = result.ReadPortal();
    for (vtkm::Id index = 0; index < portal.GetNumberOfValues(); ++index)
    {
      Vec resultValue = portal.Get(index);
      Vec expectedValue = expected[static_cast<std::size_t>(index)];
      VTKM_TEST_ASSERT(
        resultValue == expectedValue, "Expected color ", expectedValue, " but got ", resultValue);
    }
  }

public:
  static void TestConstructors()
  {
    vtkm::Range inValidRange{ 1.0, 0.0 };
    vtkm::Range range{ 0.0, 1.0 };
    vtkm::Vec<float, 3> rgb1{ 0.0f, 0.0f, 0.0f };
    vtkm::Vec<float, 3> rgb2{ 1.0f, 1.0f, 1.0f };
    auto rgbspace = vtkm::ColorSpace::RGB;
    auto hsvspace = vtkm::ColorSpace::HSV;
    auto diverging = vtkm::ColorSpace::Diverging;

    vtkm::cont::ColorTable table(rgbspace);
    VTKM_TEST_ASSERT(table.GetColorSpace() == rgbspace, "color space not saved");
    VTKM_TEST_ASSERT(table.GetRange() == inValidRange, "default range incorrect");

    vtkm::cont::ColorTable tableRGB(range, rgb1, rgb2, hsvspace);
    VTKM_TEST_ASSERT(tableRGB.GetColorSpace() == hsvspace, "color space not saved");
    VTKM_TEST_ASSERT(tableRGB.GetRange() == range, "color range not saved");

    vtkm::Vec<float, 4> rgba1{ 0.0f, 0.0f, 0.0f, 1.0f };
    vtkm::Vec<float, 4> rgba2{ 1.0f, 1.0f, 1.0f, 0.0f };
    vtkm::cont::ColorTable tableRGBA(range, rgba1, rgba2, diverging);
    VTKM_TEST_ASSERT(tableRGBA.GetColorSpace() == diverging, "color space not saved");
    VTKM_TEST_ASSERT(tableRGBA.GetRange() == range, "color range not saved");

    //verify we can store a vector of tables
    std::vector<vtkm::cont::ColorTable> tables;
    tables.push_back(table);
    tables.push_back(tableRGB);
    tables.push_back(tableRGBA);
    tables.push_back(tableRGBA);
    tables.push_back(tableRGB);
    tables.push_back(table);
  }

  static void TestLoadPresets()
  {
    vtkm::Range range{ 0.0, 1.0 };
    auto rgbspace = vtkm::ColorSpace::RGB;
    auto hsvspace = vtkm::ColorSpace::HSV;
    auto labspace = vtkm::ColorSpace::Lab;
    auto diverging = vtkm::ColorSpace::Diverging;

    {
      vtkm::cont::ColorTable table(rgbspace);
      VTKM_TEST_ASSERT(table.LoadPreset("Cool to Warm"));
      VTKM_TEST_ASSERT(table.GetColorSpace() == diverging,
                       "color space not switched when loading preset");
      VTKM_TEST_ASSERT(table.GetRange() == range, "color range not correct after loading preset");
      VTKM_TEST_ASSERT(table.GetNumberOfPoints() == 3);

      VTKM_TEST_ASSERT(table.LoadPreset(vtkm::cont::ColorTable::Preset::CoolToWarmExtended));
      VTKM_TEST_ASSERT(table.GetColorSpace() == labspace,
                       "color space not switched when loading preset");
      VTKM_TEST_ASSERT(table.GetRange() == range, "color range not correct after loading preset");
      VTKM_TEST_ASSERT(table.GetNumberOfPoints() == 35);

      table.SetColorSpace(hsvspace);
      VTKM_TEST_ASSERT((table.LoadPreset("no table with this name") == false),
                       "failed to error out on bad preset table name");
      //verify that after a failure we still have the previous preset loaded
      VTKM_TEST_ASSERT(table.GetColorSpace() == hsvspace,
                       "color space not switched when loading preset");
      VTKM_TEST_ASSERT(table.GetRange() == range, "color range not correct after failing preset");
      VTKM_TEST_ASSERT(table.GetNumberOfPoints() == 35);
    }


    //verify that we can get the presets
    std::set<std::string> names = vtkm::cont::ColorTable::GetPresets();
    VTKM_TEST_ASSERT(names.size() == 18, "incorrect number of names in preset set");

    VTKM_TEST_ASSERT(names.count("Inferno") == 1, "names should contain inferno");
    VTKM_TEST_ASSERT(names.count("Black-Body Radiation") == 1,
                     "names should contain black-body radiation");
    VTKM_TEST_ASSERT(names.count("Viridis") == 1, "names should contain viridis");
    VTKM_TEST_ASSERT(names.count("Black - Blue - White") == 1,
                     "names should contain black, blue and white");
    VTKM_TEST_ASSERT(names.count("Blue to Orange") == 1, "names should contain samsel fire");
    VTKM_TEST_ASSERT(names.count("Jet") == 1, "names should contain jet");

    // verify that we can load in all the listed color tables
    for (auto&& name : names)
    {
      vtkm::cont::ColorTable table(name);
      VTKM_TEST_ASSERT(table.GetNumberOfPoints() > 0, "Issue loading preset ", name);
    }

    auto presetEnum = { vtkm::cont::ColorTable::Preset::Default,
                        vtkm::cont::ColorTable::Preset::CoolToWarm,
                        vtkm::cont::ColorTable::Preset::CoolToWarmExtended,
                        vtkm::cont::ColorTable::Preset::Viridis,
                        vtkm::cont::ColorTable::Preset::Inferno,
                        vtkm::cont::ColorTable::Preset::Plasma,
                        vtkm::cont::ColorTable::Preset::BlackBodyRadiation,
                        vtkm::cont::ColorTable::Preset::XRay,
                        vtkm::cont::ColorTable::Preset::Green,
                        vtkm::cont::ColorTable::Preset::BlackBlueWhite,
                        vtkm::cont::ColorTable::Preset::BlueToOrange,
                        vtkm::cont::ColorTable::Preset::GrayToRed,
                        vtkm::cont::ColorTable::Preset::ColdAndHot,
                        vtkm::cont::ColorTable::Preset::BlueGreenOrange,
                        vtkm::cont::ColorTable::Preset::YellowGrayBlue,
                        vtkm::cont::ColorTable::Preset::RainbowUniform,
                        vtkm::cont::ColorTable::Preset::Jet,
                        vtkm::cont::ColorTable::Preset::RainbowDesaturated };
    for (vtkm::cont::ColorTable::Preset preset : presetEnum)
    {
      vtkm::cont::ColorTable table(preset);
      VTKM_TEST_ASSERT(table.GetNumberOfPoints() > 0, "Issue loading preset");
    }
  }

  static void TestClamping()
  {
    std::cout << "Test Clamping" << std::endl;

    vtkm::Range range{ 0.0, 1.0 };
    vtkm::Vec<float, 3> rgb1{ 0.0f, 1.0f, 0.0f };
    vtkm::Vec<float, 3> rgb2{ 1.0f, 0.0f, 1.0f };
    auto rgbspace = vtkm::ColorSpace::RGB;

    vtkm::cont::ColorTable table(range, rgb1, rgb2, rgbspace);
    VTKM_TEST_ASSERT(table.GetClamping() == true, "clamping not setup properly");

    auto field = vtkm::cont::make_ArrayHandle({ -1, 0, 1, 2 });

    vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> colors;
    const bool ran = vtkm::cont::ColorTableMap(field, table, colors);
    VTKM_TEST_ASSERT(ran, "color table failed to execute");

    //verify that we clamp the values to the expected range
    CheckColors(colors, { { 0, 255, 0 }, { 0, 255, 0 }, { 255, 0, 255 }, { 255, 0, 255 } });
  }

  static void TestRangeColors()
  {
    std::cout << "Test default ranges" << std::endl;

    vtkm::Range range{ -1.0, 2.0 };
    vtkm::Vec<float, 3> rgb1{ 0.0f, 1.0f, 0.0f };
    vtkm::Vec<float, 3> rgb2{ 1.0f, 0.0f, 1.0f };
    auto rgbspace = vtkm::ColorSpace::RGB;

    vtkm::cont::ColorTable table(range, rgb1, rgb2, rgbspace);
    table.SetClampingOff();
    VTKM_TEST_ASSERT(table.GetClamping() == false, "clamping not setup properly");

    auto field = vtkm::cont::make_ArrayHandle({ -2, -1, 2, 3 });

    vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> colors;
    const bool ran = vtkm::cont::ColorTableMap(field, table, colors);
    VTKM_TEST_ASSERT(ran, "color table failed to execute");

    //verify that both the above and below range colors are used,
    //and that the default value of both is 0,0,0
    CheckColors(colors, { { 0, 0, 0 }, { 0, 255, 0 }, { 255, 0, 255 }, { 0, 0, 0 } });


    std::cout << "Test specified ranges" << std::endl;
    //verify that we can specify custom above and below range colors
    table.SetAboveRangeColor(vtkm::Vec<float, 3>{ 1.0f, 0.0f, 0.0f }); //red
    table.SetBelowRangeColor(vtkm::Vec<float, 3>{ 0.0f, 0.0f, 1.0f }); //green
    const bool ran2 = vtkm::cont::ColorTableMap(field, table, colors);
    VTKM_TEST_ASSERT(ran2, "color table failed to execute");
    CheckColors(colors, { { 0, 0, 255 }, { 0, 255, 0 }, { 255, 0, 255 }, { 255, 0, 0 } });
  }

  static void TestRescaleRange()
  {
    std::cout << "Test Rescale Range" << std::endl;
    vtkm::Range range{ -100.0, 100.0 };

    //implement a blue2yellow color table
    vtkm::Vec<float, 3> rgb1{ 0.0f, 0.0f, 1.0f };
    vtkm::Vec<float, 3> rgb2{ 1.0f, 1.0f, 0.0f };
    auto lab = vtkm::ColorSpace::Lab;

    vtkm::cont::ColorTable table(range, rgb1, rgb2, lab);
    table.AddPoint(0.0, vtkm::Vec<float, 3>{ 0.5f, 0.5f, 0.5f });
    VTKM_TEST_ASSERT(table.GetRange() == range, "custom range not saved");

    vtkm::cont::ColorTable newTable = table.MakeDeepCopy();
    VTKM_TEST_ASSERT(newTable.GetRange() == range, "custom range not saved");

    vtkm::Range normalizedRange{ 0.0, 50.0 };
    newTable.RescaleToRange(normalizedRange);
    VTKM_TEST_ASSERT(table.GetRange() == range, "deep copy not working properly");
    VTKM_TEST_ASSERT(newTable.GetRange() == normalizedRange, "rescale of range failed");
    VTKM_TEST_ASSERT(newTable.GetNumberOfPoints() == 3,
                     "rescaled has incorrect number of control points");

    //Verify that the rescaled color table generates correct colors
    auto field = vtkm::cont::make_ArrayHandle({ 0, 10, 20, 30, 40, 50 });

    vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> colors;
    const bool ran = vtkm::cont::ColorTableMap(field, newTable, colors);
    VTKM_TEST_ASSERT(ran, "color table failed to execute");

    //values confirmed with ParaView 5.4
    CheckColors(colors,
                { { 0, 0, 255 },
                  { 105, 69, 204 },
                  { 126, 109, 153 },
                  { 156, 151, 117 },
                  { 207, 202, 87 },
                  { 255, 255, 0 } });
  }

  static void TestAddPoints()
  {
    std::cout << "Test Add Points" << std::endl;

    vtkm::Range range{ -20, 20.0 };
    auto rgbspace = vtkm::ColorSpace::RGB;

    vtkm::cont::ColorTable table(rgbspace);
    table.AddPoint(-10.0, vtkm::Vec<float, 3>{ 0.0f, 1.0f, 1.0f });
    table.AddPoint(-20.0, vtkm::Vec<float, 3>{ 1.0f, 1.0f, 1.0f });
    table.AddPoint(20.0, vtkm::Vec<float, 3>{ 0.0f, 0.0f, 0.0f });
    table.AddPoint(0.0, vtkm::Vec<float, 3>{ 0.0f, 0.0f, 1.0f });

    VTKM_TEST_ASSERT(table.GetRange() == range, "adding points to make range expand properly");
    VTKM_TEST_ASSERT(table.GetNumberOfPoints() == 4,
                     "adding points caused number of control points to be wrong");

    vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> colors;
    auto field = vtkm::cont::make_ArrayHandle({ 10.0f, -5.0f, -15.0f });
    const bool ran = vtkm::cont::ColorTableMap(field, table, colors);
    VTKM_TEST_ASSERT(ran, "color table failed to execute");

    CheckColors(colors, { { 0, 0, 128 }, { 0, 128, 255 }, { 128, 255, 255 } });
  }

  static void TestAddSegments()
  {
    std::cout << "Test Add Segments" << std::endl;

    vtkm::Range range{ 0.0, 50.0 };
    auto diverging = vtkm::ColorSpace::Diverging;

    vtkm::cont::ColorTable table(vtkm::cont::ColorTable::Preset::CoolToWarm);
    VTKM_TEST_ASSERT(table.GetColorSpace() == diverging,
                     "color space not switched when loading preset");


    //Opacity Ramp from 0 to 1
    table.AddSegmentAlpha(0.0, 0.0f, 1.0, 1.0f);
    VTKM_TEST_ASSERT(table.GetNumberOfPointsAlpha() == 2, "incorrect number of alpha points");

    table.RescaleToRange(range);

    //Verify that the opacity points have moved
    vtkm::Vec<double, 4> opacityData;
    table.GetPointAlpha(1, opacityData);
    VTKM_TEST_ASSERT(test_equal(opacityData[0], range.Max), "rescale to range failed on opacity");
    VTKM_TEST_ASSERT(opacityData[1] == 1.0, "rescale changed opacity values");
    VTKM_TEST_ASSERT(opacityData[2] == 0.5, "rescale modified mid/sharp of opacity");
    VTKM_TEST_ASSERT(opacityData[3] == 0.0, "rescale modified mid/sharp of opacity");


    vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> colors;
    auto field = vtkm::cont::make_ArrayHandle({ 0, 10, 20, 30, 40, 50 });
    const bool ran = vtkm::cont::ColorTableMap(field, table, colors);
    VTKM_TEST_ASSERT(ran, "color table failed to execute");

    //values confirmed with ParaView 5.4
    CheckColors(colors,
                { { 59, 76, 192, 0 },
                  { 124, 159, 249, 51 },
                  { 192, 212, 245, 102 },
                  { 242, 203, 183, 153 },
                  { 238, 133, 104, 204 },
                  { 180, 4, 38, 255 } });
  }

  static void TestRemovePoints()
  {
    std::cout << "Test Remove Points" << std::endl;

    auto hsv = vtkm::ColorSpace::HSV;

    vtkm::cont::ColorTable table(hsv);
    //implement Blue to Red Rainbow color table
    table.AddSegment(0,
                     vtkm::Vec<float, 3>{ 0.0f, 0.0f, 1.0f },
                     1., //second points color should be replaced by following segment
                     vtkm::Vec<float, 3>{ 1.0f, 0.0f, 0.0f });

    table.AddPoint(-10.0, vtkm::Vec<float, 3>{ 0.0f, 1.0f, 1.0f });
    table.AddPoint(-20.0, vtkm::Vec<float, 3>{ 1.0f, 1.0f, 1.0f });
    table.AddPoint(20.0, vtkm::Vec<float, 3>{ 1.0f, 0.0f, 0.0f });

    VTKM_TEST_ASSERT(table.RemovePoint(-10.) == true, "failed to remove a existing point");
    VTKM_TEST_ASSERT(table.RemovePoint(-20.) == true, "failed to remove a existing point");
    VTKM_TEST_ASSERT(table.RemovePoint(20.) == true, "failed to remove a existing point");
    VTKM_TEST_ASSERT(table.RemovePoint(20.) == false, "can't remove a point that doesn't exist");

    VTKM_TEST_ASSERT((table.GetRange() == vtkm::Range{ 0.0, 1.0 }),
                     "removing points didn't update range");
    table.RescaleToRange(vtkm::Range{ 0.0, 50.0 });

    vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> colors;
    auto field = vtkm::cont::make_ArrayHandle({ 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f });
    const bool ran = vtkm::cont::ColorTableMap(field, table, colors);
    VTKM_TEST_ASSERT(ran, "color table failed to execute");

    //values confirmed with ParaView 5.4
    CheckColors(colors,
                { { 0, 0, 255 },
                  { 0, 204, 255 },
                  { 0, 255, 102 },
                  { 102, 255, 0 },
                  { 255, 204, 0 },
                  { 255, 0, 0 } });

    std::cout << "  Change Color Space" << std::endl;
    vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> colors_rgb;
    table.SetColorSpace(vtkm::ColorSpace::RGB);
    vtkm::cont::ColorTableMap(field, table, colors_rgb);

    CheckColors(colors_rgb,
                { { 0, 0, 255 },
                  { 51, 0, 204 },
                  { 102, 0, 153 },
                  { 153, 0, 102 },
                  { 204, 0, 51 },
                  { 255, 0, 0 } });
  }

  static void TestOpacityOnlyPoints()
  {
    std::cout << "Test Opacity Only Points" << std::endl;

    auto hsv = vtkm::ColorSpace::HSV;

    vtkm::cont::ColorTable table(hsv);
    //implement only a color table
    table.AddPointAlpha(0.0, 0.0f, 0.75f, 0.25f);
    table.AddPointAlpha(1.0, 1.0f);

    table.AddPointAlpha(10.0, 0.5f, 0.5f, 0.0f);
    table.AddPointAlpha(-10.0, 0.0f);
    table.AddPointAlpha(-20.0, 1.0f);
    table.AddPointAlpha(20.0, 0.5f);

    VTKM_TEST_ASSERT(table.RemovePointAlpha(10.) == true, "failed to remove a existing point");
    VTKM_TEST_ASSERT(table.RemovePointAlpha(-10.) == true, "failed to remove a existing point");
    VTKM_TEST_ASSERT(table.RemovePointAlpha(-20.) == true, "failed to remove a existing point");
    VTKM_TEST_ASSERT(table.RemovePointAlpha(20.) == true, "failed to remove a existing point");
    VTKM_TEST_ASSERT(table.RemovePointAlpha(20.) == false,
                     "can't remove a point that doesn't exist");

    VTKM_TEST_ASSERT((table.GetRange() == vtkm::Range{ 0.0, 1.0 }),
                     "removing points didn't update range");
    table.RescaleToRange(vtkm::Range{ 0.0, 50.0 });

    vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> colors;
    auto field = vtkm::cont::make_ArrayHandle({ 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 50.0f });
    const bool ran = vtkm::cont::ColorTableMap(field, table, colors);
    VTKM_TEST_ASSERT(ran, "color table failed to execute");

    //values confirmed with ParaView 5.4
    CheckColors(colors,
                { { 0, 0, 0, 0 },
                  { 0, 0, 0, 1 },
                  { 0, 0, 0, 11 },
                  { 0, 0, 0, 52 },
                  { 0, 0, 0, 203 },
                  { 0, 0, 0, 255 } });
  }

  static void TestWorkletTransport()
  {
    std::cout << "Test Worklet Transport" << std::endl;

    using namespace vtkm::worklet::colorconversion;

    vtkm::cont::ColorTable table(vtkm::cont::ColorTable::Preset::Green);
    VTKM_TEST_ASSERT((table.GetRange() == vtkm::Range{ 0.0, 1.0 }),
                     "loading linear green table failed with wrong range");
    VTKM_TEST_ASSERT((table.GetNumberOfPoints() == 21),
                     "loading linear green table failed with number of control points");

    auto samples = vtkm::cont::make_ArrayHandle({ 0.0, 0.5, 1.0 });

    vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> colors;
    vtkm::cont::Invoker invoke;
    invoke(TransferFunction{}, samples, table, colors);

    CheckColors(colors, { { 14, 28, 31, 255 }, { 21, 150, 21, 255 }, { 255, 251, 230, 255 } });
  }

  static void TestSampling()
  {
    std::cout << "Test Sampling" << std::endl;

    vtkm::cont::ColorTable table(vtkm::cont::ColorTable::Preset::Green);
    VTKM_TEST_ASSERT((table.GetRange() == vtkm::Range{ 0.0, 1.0 }),
                     "loading linear green table failed with wrong range");
    VTKM_TEST_ASSERT((table.GetNumberOfPoints() == 21),
                     "loading linear green table failed with number of control points");

    vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> colors;
    constexpr vtkm::Id nvals = 3;
    table.Sample(nvals, colors);

    CheckColors(colors, { { 14, 28, 31, 255 }, { 21, 150, 21, 255 }, { 255, 251, 230, 255 } });
  }

  static void TestLookupTable()
  {
    std::cout << "Test Lookup Table" << std::endl;

    //build a color table with clamping off and verify that sampling works
    vtkm::Range range{ 0.0, 50.0 };
    vtkm::cont::ColorTable table(vtkm::cont::ColorTable::Preset::CoolToWarm);
    table.RescaleToRange(range);
    table.SetClampingOff();
    table.SetAboveRangeColor(vtkm::Vec<float, 3>{ 1.0f, 0.0f, 0.0f }); //red
    table.SetBelowRangeColor(vtkm::Vec<float, 3>{ 0.0f, 0.0f, 1.0f }); //green

    vtkm::cont::ColorTableSamplesRGB samples;
    table.Sample(256, samples);
    VTKM_TEST_ASSERT((samples.Samples.GetNumberOfValues() == 260), "invalid sample length");

    vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> colors;
    auto field = vtkm::cont::make_ArrayHandle({ -1, 0, 10, 20, 30, 40, 50, 60 });
    const bool ran = vtkm::cont::ColorTableMap(field, samples, colors);
    VTKM_TEST_ASSERT(ran, "color table failed to execute");

    //values confirmed with ParaView 5.4
    CheckColors(colors,
                { { 0, 0, 255 },
                  { 59, 76, 192 },
                  { 122, 157, 248 },
                  { 191, 211, 246 },
                  { 241, 204, 184 },
                  { 238, 134, 105 },
                  { 180, 4, 38 },
                  { 255, 0, 0 } });
  }

  struct TestAll
  {
    VTKM_CONT void operator()() const
    {
      TestConstructors();
      TestLoadPresets();
      TestClamping();
      TestRangeColors();

      TestRescaleRange(); //uses Lab
      TestAddPoints();    //uses RGB
      TestAddSegments();  //uses Diverging && opacity
      TestRemovePoints(); //use HSV

      TestOpacityOnlyPoints();

      TestWorkletTransport();
      TestSampling();
      TestLookupTable();
    }
  };

  static int Run(int argc, char* argv[])
  {
    //We need to verify the color table runs on this specific device
    //so we need to force our single device
    vtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return vtkm::cont::testing::Testing::Run(TestAll(), argc, argv);
  }
};
}
}
}
#endif
