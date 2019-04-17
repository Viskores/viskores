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
#include <vtkm/cont/ArrayHandleDiscard.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/cont/serial/internal/ArrayManagerExecutionSerial.h>
#include <vtkm/cont/serial/internal/DeviceAdapterAlgorithmSerial.h>
#include <vtkm/cont/serial/internal/DeviceAdapterTagSerial.h>

#include <vtkm/cont/testing/Testing.h>

#include <vtkm/BinaryOperators.h>

#include <algorithm>

namespace UnitTestArrayHandleDiscardDetail
{

template <typename ValueType>
struct Test
{
  static constexpr vtkm::Id ARRAY_SIZE = 100;
  static constexpr vtkm::Id NUM_KEYS = 3;

  using DeviceTag = vtkm::cont::DeviceAdapterTagSerial;
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>;
  using Handle = vtkm::cont::ArrayHandle<ValueType>;
  using DiscardHandle = vtkm::cont::ArrayHandleDiscard<ValueType>;
  using OutputPortal = typename Handle::PortalControl;
  using ReduceOp = vtkm::Add;

  // Test discard arrays by using the ReduceByKey algorithm. Two regular arrays
  // handles are provided as inputs, but the keys_output array is a discard
  // array handle. The values_output array should still be populated correctly.
  void TestReduceByKey()
  {
    // The reduction operator:
    ReduceOp op;

    // Prepare inputs / reference data:
    ValueType keyData[ARRAY_SIZE];
    ValueType valueData[ARRAY_SIZE];
    ValueType refData[NUM_KEYS];
    std::fill(refData, refData + NUM_KEYS, ValueType(0));
    for (vtkm::Id i = 0; i < ARRAY_SIZE; ++i)
    {
      const vtkm::Id key = i % NUM_KEYS;
      keyData[i] = static_cast<ValueType>(key);
      valueData[i] = static_cast<ValueType>(i * 2);
      refData[key] = op(refData[key], valueData[i]);
    }

    // Prepare array handles:
    Handle keys = vtkm::cont::make_ArrayHandle(keyData, ARRAY_SIZE);
    Handle values = vtkm::cont::make_ArrayHandle(valueData, ARRAY_SIZE);
    DiscardHandle output_keys;
    Handle output_values;

    Algorithm::SortByKey(keys, values);
    Algorithm::ReduceByKey(keys, values, output_keys, output_values, op);

    OutputPortal outputs = output_values.GetPortalControl();

    VTKM_TEST_ASSERT(outputs.GetNumberOfValues() == NUM_KEYS,
                     "Unexpected number of output values from ReduceByKey.");

    for (vtkm::Id i = 0; i < NUM_KEYS; ++i)
    {
      VTKM_TEST_ASSERT(outputs.Get(i) == refData[i], "Unexpected output value after ReduceByKey.");
    }
  }

  void TestPrepareExceptions()
  {
    DiscardHandle handle;
    handle.Allocate(50);

    try
    {
      handle.PrepareForInput(DeviceTag());
    }
    catch (vtkm::cont::ErrorBadValue&)
    {
      // Expected failure.
    }

    try
    {
      handle.PrepareForInPlace(DeviceTag());
    }
    catch (vtkm::cont::ErrorBadValue&)
    {
      // Expected failure.
    }

    // Shouldn't fail:
    handle.PrepareForOutput(ARRAY_SIZE, DeviceTag());
  }

  void operator()()
  {
    TestReduceByKey();
    TestPrepareExceptions();
  }
};

void TestArrayHandleDiscard()
{
  Test<vtkm::UInt8>()();
  Test<vtkm::Int16>()();
  Test<vtkm::Int32>()();
  Test<vtkm::Int64>()();
  Test<vtkm::Float32>()();
  Test<vtkm::Float64>()();
}

} // end namespace UnitTestArrayHandleDiscardDetail

int UnitTestArrayHandleDiscard(int argc, char* argv[])
{
  using namespace UnitTestArrayHandleDiscardDetail;
  return vtkm::cont::testing::Testing::Run(TestArrayHandleDiscard, argc, argv);
}
