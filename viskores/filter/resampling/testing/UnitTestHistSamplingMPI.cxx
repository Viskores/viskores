//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/ArrayHandleCounting.h>
#include <viskores/cont/EnvironmentTracker.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/entity_extraction/ThresholdPoints.h>
#include <viskores/filter/resampling/HistSampling.h>
#include <viskores/filter/resampling/testing/TestingHistSampling.h>
#include <viskores/thirdparty/diy/environment.h>

namespace
{

void TestHistSamplingUsesGlobalHistogramAcrossRanks()
{
  auto communicator = viskores::cont::EnvironmentTracker::GetCommunicator();
  viskores::cont::PartitionedDataSet input;

  if (communicator.rank() == 0)
  {
    input.AppendPartition(viskores::filter::resampling::testing::MakePointCloudPartition(10, 9, 0));
  }

  if (communicator.size() == 1 || communicator.rank() == 1)
  {
    input.AppendPartition(
      viskores::filter::resampling::testing::MakePointCloudPartition(90, 0, 10));
  }

  viskores::filter::resampling::HistSampling histSample;
  histSample.SetNumberOfBins(2);
  histSample.SetSampleFraction(0.2f);
  histSample.SetActiveField("scalarField", viskores::cont::Field::Association::Points);
  auto output = histSample.Execute(input);
  VISKORES_TEST_ASSERT(output.GetNumberOfPartitions() == input.GetNumberOfPartitions());

  if (communicator.rank() == 0)
  {
    const auto& sampledPartition = output.GetPartition(0);
    viskores::filter::entity_extraction::ThresholdPoints selectGloballyRareValues;
    selectGloballyRareValues.SetActiveField("scalarField");
    selectGloballyRareValues.SetCompactPoints(true);
    selectGloballyRareValues.SetThresholdBelow(0.5);
    auto globallyRareValues = selectGloballyRareValues.Execute(sampledPartition);

    VISKORES_TEST_ASSERT(
      test_equal_ArrayHandles(globallyRareValues.GetField("pointId").GetData(),
                              viskores::cont::make_ArrayHandleCounting(
                                viskores::Id{ 0 }, viskores::Id{ 1 }, viskores::Id{ 9 })));
  }
}

void TestHistSamplingGloballyEmptyInput()
{
  viskores::cont::PartitionedDataSet input;

  viskores::filter::resampling::HistSampling histSample;
  histSample.SetActiveField("scalarField", viskores::cont::Field::Association::Points);
  auto output = histSample.Execute(input);

  VISKORES_TEST_ASSERT(output.GetNumberOfPartitions() == 0);
}

void TestHistSamplingMPI()
{
  TestHistSamplingUsesGlobalHistogramAcrossRanks();
  TestHistSamplingGloballyEmptyInput();
}

} // anonymous namespace

int UnitTestHistSamplingMPI(int argc, char* argv[])
{
  viskoresdiy::mpi::environment environment(argc, argv);
  return viskores::cont::testing::Testing::Run(TestHistSamplingMPI, argc, argv);
}
