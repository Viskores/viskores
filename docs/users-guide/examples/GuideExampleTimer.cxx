//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Timer.h>

#include <viskores/filter/field_transform/PointElevation.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

void DoTiming()
{
  viskores::cont::DataSet dataSet =
    viskores::cont::testing::MakeTestDataSet().Make2DUniformDataSet0();
  ////
  //// BEGIN-EXAMPLE Timer
  ////
  viskores::filter::field_transform::PointElevation elevationFilter;
  elevationFilter.SetUseCoordinateSystemAsField(true);
  elevationFilter.SetOutputFieldName("elevation");

  viskores::cont::Timer timer;

  timer.Start();

  viskores::cont::DataSet result = elevationFilter.Execute(dataSet);

  // This code makes sure data is pulled back to the host in a host/device
  // architecture.
  viskores::cont::ArrayHandle<viskores::Float64> outArray;
  result.GetField("elevation").GetData().AsArrayHandle(outArray);
  outArray.SyncControlArray();

  timer.Stop();

  viskores::Float64 elapsedTime = timer.GetElapsedTime();

  std::cout << "Time to run: " << elapsedTime << std::endl;
  ////
  //// END-EXAMPLE Timer
  ////
}

} // anonymous namespace

int GuideExampleTimer(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoTiming, argc, argv);
}
