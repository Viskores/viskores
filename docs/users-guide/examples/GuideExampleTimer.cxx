//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/filter/field_transform/PointElevation.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void DoTiming()
{
  vtkm::cont::DataSet dataSet =
    vtkm::cont::testing::MakeTestDataSet().Make2DUniformDataSet0();
  ////
  //// BEGIN-EXAMPLE Timer
  ////
  vtkm::filter::field_transform::PointElevation elevationFilter;
  elevationFilter.SetUseCoordinateSystemAsField(true);
  elevationFilter.SetOutputFieldName("elevation");

  vtkm::cont::Timer timer;

  timer.Start();

  vtkm::cont::DataSet result = elevationFilter.Execute(dataSet);

  // This code makes sure data is pulled back to the host in a host/device
  // architecture.
  vtkm::cont::ArrayHandle<vtkm::Float64> outArray;
  result.GetField("elevation").GetData().AsArrayHandle(outArray);
  outArray.SyncControlArray();

  timer.Stop();

  vtkm::Float64 elapsedTime = timer.GetElapsedTime();

  std::cout << "Time to run: " << elapsedTime << std::endl;
  ////
  //// END-EXAMPLE Timer
  ////
}

} // anonymous namespace

int GuideExampleTimer(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(DoTiming, argc, argv);
}
