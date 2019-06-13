//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleStreaming.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherStreamingMapField.h>

#include <vector>

namespace vtkm
{
namespace worklet
{
class SineWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1, WorkIndex);

  template <typename T>
  VTKM_EXEC T operator()(T x, vtkm::Id& index) const
  {
    return (static_cast<T>(index) + vtkm::Sin(x));
  }
};
}
}

// Utility method to print input, output, and reference arrays
template <class T1, class T2, class T3>
void compareArrays(T1& a1, T2& a2, T3& a3, char const* text)
{
  for (vtkm::Id i = 0; i < a1.GetNumberOfValues(); ++i)
  {
    std::cout << a1.GetPortalConstControl().Get(i) << " " << a2.GetPortalConstControl().Get(i)
              << " " << a3.GetPortalConstControl().Get(i) << std::endl;
    VTKM_TEST_ASSERT(
      test_equal(a2.GetPortalConstControl().Get(i), a3.GetPortalConstControl().Get(i), 0.01f),
      text);
  }
}

void TestStreamingSine()
{
  // Test the streaming worklet
  std::cout << "Testing streaming worklet:" << std::endl;

  const vtkm::Id N = 25;
  const vtkm::Id NBlocks = 4;
  vtkm::cont::ArrayHandle<vtkm::Float32> input, output, reference, summation;
  std::vector<vtkm::Float32> data(N), test(N);
  vtkm::Float32 testSum = 0.0f;
  for (vtkm::UInt32 i = 0; i < N; i++)
  {
    data[i] = static_cast<vtkm::Float32>(i);
    test[i] = static_cast<vtkm::Float32>(i) + static_cast<vtkm::Float32>(vtkm::Sin(data[i]));
    testSum += test[i];
  }
  input = vtkm::cont::make_ArrayHandle(data);

  using DeviceAlgorithms = vtkm::cont::Algorithm;
  vtkm::worklet::SineWorklet sineWorklet;
  vtkm::worklet::DispatcherStreamingMapField<vtkm::worklet::SineWorklet> dispatcher(sineWorklet);
  dispatcher.SetNumberOfBlocks(NBlocks);
  dispatcher.Invoke(input, output);

  reference = vtkm::cont::make_ArrayHandle(test);
  compareArrays(input, output, reference, "Wrong result for streaming sine worklet");

  vtkm::Float32 referenceSum, streamSum;

  // Test the streaming exclusive scan
  std::cout << "Testing streaming exclusive scan: " << std::endl;
  referenceSum = DeviceAlgorithms::ScanExclusive(input, summation);
  streamSum = DeviceAlgorithms::StreamingScanExclusive(4, input, output);
  VTKM_TEST_ASSERT(test_equal(streamSum, referenceSum, 0.01f),
                   "Wrong sum for streaming exclusive scan");
  compareArrays(input, output, summation, "Wrong result for streaming exclusive scan");

  // Test the streaming exclusive scan with binary operator
  std::cout << "Testing streaming exnclusive scan with binary operator: " << std::endl;
  vtkm::Float32 initValue = 0.0;
  referenceSum = DeviceAlgorithms::ScanExclusive(input, summation, vtkm::Maximum(), initValue);
  streamSum =
    DeviceAlgorithms::StreamingScanExclusive(4, input, output, vtkm::Maximum(), initValue);
  VTKM_TEST_ASSERT(test_equal(streamSum, referenceSum, 0.01f),
                   "Wrong sum for streaming exclusive scan with binary operator");
  compareArrays(
    input, output, summation, "Wrong result for streaming exclusive scan with binary operator");

  // Test the streaming reduce
  std::cout << "Testing streaming reduce: " << std::endl;
  referenceSum = DeviceAlgorithms::Reduce(input, 0.0f);
  streamSum = DeviceAlgorithms::StreamingReduce(4, input, 0.0f);
  std::cout << "Result: " << streamSum << " " << referenceSum << std::endl;
  VTKM_TEST_ASSERT(test_equal(streamSum, referenceSum, 0.01f), "Wrong sum for streaming reduce");

  // Test the streaming reduce with binary operator
  std::cout << "Testing streaming reduce with binary operator: " << std::endl;
  referenceSum = DeviceAlgorithms::Reduce(input, 0.0f, vtkm::Maximum());
  streamSum = DeviceAlgorithms::StreamingReduce(4, input, 0.0f, vtkm::Maximum());
  std::cout << "Result: " << streamSum << " " << referenceSum << std::endl;
  VTKM_TEST_ASSERT(test_equal(streamSum, referenceSum, 0.01f),
                   "Wrong sum for streaming reduce with binary operator");
}

int UnitTestStreamingSine(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestStreamingSine, argc, argv);
}
