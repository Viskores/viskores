//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include <viskores/worklet/MaskSelectTemplate.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayHandleIndex.h>
#include <viskores/cont/ArrayHandleTransform.h>
#include <viskores/cont/Invoker.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

constexpr viskores::Id ARRAY_SIZE = 20;

////
//// BEGIN-EXAMPLE MaskSelectWorklet
////
struct NearestFibonacci : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut);
  using ExecutionSignature = void(_1);

  ////
  //// BEGIN-EXAMPLE DeclareMask
  ////
  using MaskType = viskores::worklet::MaskSelect;
  ////
  //// END-EXAMPLE DeclareMask
  ////

  template<typename T>
  VISKORES_EXEC void operator()(T& targetNumber) const
  {
    T beforeLast = T(0);
    T last = T(1);
    while (last < targetNumber)
    {
      T next = beforeLast + last;
      beforeLast = last;
      last = next;
    }

    targetNumber =
      ((targetNumber - beforeLast) < (last - targetNumber)) ? beforeLast : last;
  }
};
////
//// END-EXAMPLE MaskSelectWorklet
////

////
//// BEGIN-EXAMPLE MaskSelectRun
////
struct MaskExactFibonacci
{
  template<typename T>
  VISKORES_EXEC T operator()(T x) const
  {
    return (x > 3) ? 1 : 0;
  }
};

/// Later in the filter...

//// PAUSE-EXAMPLE
struct DoRun
{
  viskores::cont::Invoker Invoke = viskores::cont::Invoker{};

  void operator()() const
  {
    viskores::cont::ArrayHandle<viskores::Id> dataArray;
    viskores::cont::ArrayCopy(viskores::cont::ArrayHandleIndex(ARRAY_SIZE), dataArray);
    viskores::cont::ArrayHandle<viskores::Id> outputArray;

    //// RESUME-EXAMPLE
    ////
    //// BEGIN-EXAMPLE ConstructMaskForInvoke
    ////
    viskores::worklet::MaskSelectTemplate mask{
      viskores::cont::make_ArrayHandleTransform(dataArray, MaskExactFibonacci{})
    };
    this->Invoke(NearestFibonacci{}, mask, dataArray);
    ////
    //// END-EXAMPLE ConstructMaskForInvoke
    ////
    ////
    //// END-EXAMPLE MaskSelectRun
    ////
    viskores::cont::printSummary_ArrayHandle(dataArray, std::cout, true);
    VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
      dataArray,
      viskores::cont::make_ArrayHandle(
        { 0, 1, 2, 3, 5, 5, 5, 8, 8, 8, 8, 13, 13, 13, 13, 13, 13, 21, 21, 21 })));
  }
};

void Run()
{
  DoRun{}();
}

} // anonymous namespace

int GuideExampleMaskSelect(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}