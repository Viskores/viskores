//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ArrayHandleStreaming.h>
#include <vtkm/worklet/DispatcherStreamingMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>

#include <vtkm/cont/testing/Testing.h>

#include <vector>

namespace vtkm
{
namespace worklet
{
class SineWorklet : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<>, FieldOut<>);
  typedef _2 ExecutionSignature(_1, WorkIndex);

  template<typename T>
  VTKM_EXEC_EXPORT
  T operator()(T x, vtkm::Id& index) const {
    return (static_cast<T>(index) + vtkm::Sin(x));
  }
};
}
}

void TestStreamingSine()
{
  std::cout << "Testing Streaming Sine" << std::endl;

  const vtkm::Id N = 25;  const vtkm::Id NBlocks = 4;
  vtkm::cont::ArrayHandle<vtkm::Float32> input, output;
  std::vector<vtkm::Float32> data(N), test(N);
  for (vtkm::UInt32 i=0; i<N; i++)
  {
    data[i] = static_cast<vtkm::Float32>(i);
    test[i] = static_cast<vtkm::Float32>(i) + static_cast<vtkm::Float32>(vtkm::Sin(data[i]));
  }
  input = vtkm::cont::make_ArrayHandle(data);

  vtkm::worklet::SineWorklet sineWorklet;
  vtkm::worklet::DispatcherStreamingMapField<vtkm::worklet::SineWorklet> 
      dispatcher(sineWorklet);
  dispatcher.SetNumberOfBlocks(NBlocks);
  
  dispatcher.Invoke(input, output);

  std::cout << "Output size: " << output.GetNumberOfValues() << std::endl;
  for (vtkm::UInt32 i = 0; i < output.GetNumberOfValues(); ++i)
  {
    std::cout << input.GetPortalConstControl().Get(i) << " " 
              << output.GetPortalConstControl().Get(i) << " "
              << test[i] << std::endl;
    VTKM_TEST_ASSERT(
         test_equal(output.GetPortalConstControl().Get(i), test[i], 0.01f), 
                    "Wrong result for streaming sine worklet");
  }

}

int UnitTestStreamingSine(int, char *[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamingSine);
}
