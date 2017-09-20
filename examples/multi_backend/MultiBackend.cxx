//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <iostream>

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/RuntimeDeviceInformation.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/TryExecute.h>
#include <vtkm/cont/cuda/DeviceAdapterCuda.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

using FloatVec3 = vtkm::Vec<vtkm::Float32, 3>;
using Uint8Vec4 = vtkm::Vec<vtkm::UInt8, 4>;

struct GenerateSurfaceWorklet : public vtkm::worklet::WorkletMapField
{
  vtkm::Float32 t;
  GenerateSurfaceWorklet(vtkm::Float32 st)
    : t(st)
  {
  }

  typedef void ControlSignature(FieldIn<>, FieldOut<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2, _3);

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Vec<T, 3>& input,
                            vtkm::Vec<T, 3>& output,
                            vtkm::Vec<vtkm::UInt8, 4>& color) const
  {
    output[0] = input[0];
    output[1] = 0.25f * vtkm::Sin(input[0] * 10.f + t) * vtkm::Cos(input[2] * 10.f + t);
    output[2] = input[2];

    color[0] = 0;
    color[1] = static_cast<vtkm::UInt8>(160 + (96 * vtkm::Sin(input[0] * 10.f + t)));
    color[2] = static_cast<vtkm::UInt8>(160 + (96 * vtkm::Cos(input[2] * 5.f + t)));
    color[3] = 255;
  }
};

struct RunGenerateSurfaceWorklet
{
  template <typename DeviceAdapterTag>
  bool operator()(DeviceAdapterTag) const
  {
    //At this point we know we have runtime support
    using DeviceTraits = vtkm::cont::DeviceAdapterTraits<DeviceAdapterTag>;

    using DispatcherType =
      vtkm::worklet::DispatcherMapField<GenerateSurfaceWorklet, DeviceAdapterTag>;

    std::cout << "Running a worklet on device adapter: " << DeviceTraits::GetName() << std::endl;

    GenerateSurfaceWorklet worklet(0.05f);
    DispatcherType(worklet).Invoke(this->In, this->Out, this->Color);

    return true;
  }

  vtkm::cont::ArrayHandle<FloatVec3> In;
  vtkm::cont::ArrayHandle<FloatVec3> Out;
  vtkm::cont::ArrayHandle<Uint8Vec4> Color;
};

template <typename T>
std::vector<vtkm::Vec<T, 3>> make_testData(int size)
{
  std::vector<vtkm::Vec<T, 3>> data;
  data.reserve(static_cast<std::size_t>(size * size));
  for (int i = 0; i < size; ++i)
  {
    for (int j = 0; j < size; ++j)
    {
      data.push_back(vtkm::Vec<T, 3>(
        2.f * static_cast<T>(i / size) - 1.f, 0.f, 2.f * static_cast<T>(j / size) - 1.f));
    }
  }
  return data;
}

//This is the list of devices to compile in support for. The order of the
//devices determines the runtime preference.
struct DevicesToTry : vtkm::ListTagBase<vtkm::cont::DeviceAdapterTagCuda,
                                        vtkm::cont::DeviceAdapterTagTBB,
                                        vtkm::cont::DeviceAdapterTagSerial>
{
};

int main(int, char**)
{
  std::vector<FloatVec3> data = make_testData<vtkm::Float32>(1024);

  //make array handles for the data

  // TryExecutes takes a functor and a list of devices. It then tries to run
  // the functor for each device (in the order given in the list) until the
  // execution succeeds. This allows you to compile in support for multiple
  // devices which have runtime requirements ( GPU / HW Accelerator ) and
  // correctly choose the best device at runtime.
  //
  // The functor parentheses operator should take exactly one argument, which is
  // the DeviceAdapterTag to use. The functor should return true if the execution
  // succeeds.
  //
  // This function also optionally takes a vtkm::cont::RuntimeDeviceTracker, which
  // will monitor for certain failures across calls to TryExecute and skip trying
  // devices with a history of failure.
  RunGenerateSurfaceWorklet task;
  task.In = vtkm::cont::make_ArrayHandle(data);
  vtkm::cont::TryExecute(task, DevicesToTry());
}
