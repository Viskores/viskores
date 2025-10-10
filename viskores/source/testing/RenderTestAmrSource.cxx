//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/source/Amr.h>

#include <viskores/rendering/testing/RenderTest.h>
#include <viskores/rendering/testing/Testing.h>

namespace
{

void TestAmr()
{
  viskores::source::Amr amrSource;
  amrSource.SetDimension(3);
  amrSource.SetCellsPerDimension(6);
  amrSource.SetNumberOfLevels(3);
  viskores::cont::PartitionedDataSet amr = amrSource.Execute();

  // amr.PrintSummary(std::cout);

  viskores::rendering::testing::RenderTestOptions options;
  options.Mapper = viskores::rendering::testing::MapperType::Volume;
  options.ColorTable =
    viskores::cont::ColorTable{ viskores::cont::ColorTable::Preset::BlueToOrange };
  options.ColorTable.AddPointAlpha(0.0, 0.0);
  options.ColorTable.AddSegmentAlpha(0.25, 0.0, 1.0, 1.0);

  viskores::rendering::testing::RenderTest(amr, "RTData", "source/amr.png", options);
}

} // anonymous namespace

int RenderTestAmrSource(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestAmr, argc, argv);
}
