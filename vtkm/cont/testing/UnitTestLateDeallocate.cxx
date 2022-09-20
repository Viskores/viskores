//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterTag.h>
#include <vtkm/cont/RuntimeDeviceTracker.h>
#include <vtkm/cont/Token.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

// These should be constructed early in program startup and destroyed late on
// program shutdown. They will likely be destroyed after any device is cleaned up.
struct Data
{
  vtkm::cont::ArrayHandle<vtkm::Id> Array;
  vtkm::cont::DataSet DataSet;

  ~Data() { std::cout << "Destroying global data." << std::endl; }
};
Data Globals;

void AllocateDeviceMemory()
{
  // Load data.
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleIndex(10), Globals.Array);
  Globals.DataSet = vtkm::cont::testing::MakeTestDataSet{}.Make3DExplicitDataSet0();

  vtkm::cont::CellSetExplicit<> cellSet;
  Globals.DataSet.GetCellSet().AsCellSet(cellSet);

  // Put data on devices.
  auto& tracker = vtkm::cont::GetRuntimeDeviceTracker();
  for (vtkm::Int8 deviceIndex = 0; deviceIndex < VTKM_MAX_DEVICE_ADAPTER_ID; ++deviceIndex)
  {
    vtkm::cont::DeviceAdapterId device = vtkm::cont::make_DeviceAdapterId(deviceIndex);
    if (device.IsValueValid() && tracker.CanRunOn(device))
    {
      std::cout << "Loading data on " << device.GetName() << std::endl;

      vtkm::cont::Token token;
      Globals.Array.PrepareForInput(device, token);
      cellSet.PrepareForInput(
        device, vtkm::TopologyElementTagPoint{}, vtkm::TopologyElementTagCell{}, token);
    }
  }
}

} // anonymous namespace

int UnitTestLateDeallocate(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(AllocateDeviceMemory, argc, argv);

  // After this test returns, the global data structures will be deallocated. This will likely
  // happen after all the devices are deallocated. You may get a warning, but you should not
  // get a crash.
}
