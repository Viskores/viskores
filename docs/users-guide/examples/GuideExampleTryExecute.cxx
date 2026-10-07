//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/DeviceAdapterAlgorithm.h>
#include <viskores/cont/Timer.h>
#include <viskores/cont/TryExecute.h>

#include <viskores/cont/testing/Testing.h>

namespace TryExecuteExample
{

////
//// BEGIN-EXAMPLE ArrayAverageImpl
////
template<typename T, typename Storage, typename Device>
VISKORES_CONT T ArrayAverage(const viskores::cont::ArrayHandle<T, Storage>& array,
                             Device device)
{
  // Specialize a timer for this specific device.
  viskores::cont::Timer timer;
  timer.Reset(device);

  // Call reduce on this specific device.
  timer.Start();
  T sum = viskores::cont::Algorithm::Reduce(device, array, T(0));
  timer.Stop();

  std::cout << "Elapsed reduction time: " << timer.GetElapsedTime() << std::endl;

  return sum / T(array.GetNumberOfValues());
}
////
//// END-EXAMPLE ArrayAverageImpl
////

////
//// BEGIN-EXAMPLE ArrayAverageTryExecute
////
namespace detail
{

struct ArrayAverageFunctor
{
  template<typename Device, typename T, typename Storage>
  VISKORES_CONT bool operator()(Device device,
                                const viskores::cont::ArrayHandle<T, Storage>& inArray,
                                T& outValue) const
  {
    // Call the version of ArrayAverage that takes a DeviceAdapter.
    outValue = ArrayAverage(inArray, device);

    return true;
  }
};

} // namespace detail

template<typename T, typename Storage>
VISKORES_CONT T ArrayAverage(const viskores::cont::ArrayHandle<T, Storage>& array)
{
  T outValue;

  bool foundAverage =
    viskores::cont::TryExecute(detail::ArrayAverageFunctor{}, array, outValue);

  if (!foundAverage)
  {
    throw viskores::cont::ErrorExecution("Could not compute array average.");
  }

  return outValue;
}
////
//// END-EXAMPLE ArrayAverageTryExecute
////

void Run()
{
  static const viskores::Id ARRAY_SIZE = 11;

  std::cout << "Running average on " << ARRAY_SIZE << " indices" << std::endl;
  viskores::Id average = ArrayAverage(viskores::cont::ArrayHandleIndex(ARRAY_SIZE));
  VISKORES_TEST_ASSERT(average == (ARRAY_SIZE - 1) / 2, "Bad average");
}

} // namespace TryExecuteExample

int GuideExampleTryExecute(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TryExecuteExample::Run, argc, argv);
}
