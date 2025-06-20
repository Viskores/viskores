//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/Invoker.h>

#include <viskores/worklet/ScatterUniform.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE ScatterUniform
////
struct InterleaveArrays : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3, VisitIndex);
  using InputDomain = _1;

  using ScatterType = viskores::worklet::ScatterUniform<2>;

  template<typename T>
  VISKORES_EXEC void operator()(const T& input0,
                                const T& input1,
                                T& output,
                                viskores::IdComponent visitIndex) const
  {
    if (visitIndex == 0)
    {
      output = input0;
    }
    else // visitIndex == 1
    {
      output = input1;
    }
  }
};
////
//// END-EXAMPLE ScatterUniform
////

void Run()
{
  std::cout << "Trying scatter uniform with array interleave." << std::endl;

  static const viskores::Id ARRAY_SIZE = 10;
  static const viskores::Id value0 = 8;
  static const viskores::Id value1 = 42;

  viskores::cont::Invoker invoke;

  viskores::cont::ArrayHandle<viskores::Id> outArray;

  invoke(InterleaveArrays{},
         viskores::cont::make_ArrayHandleConstant(value0, ARRAY_SIZE),
         viskores::cont::make_ArrayHandleConstant(value1, ARRAY_SIZE),
         outArray);

  viskores::cont::printSummary_ArrayHandle(outArray, std::cout);
  std::cout << std::endl;
  VISKORES_TEST_ASSERT(outArray.GetNumberOfValues() == ARRAY_SIZE * 2,
                       "Wrong sized array.");
  for (viskores::Id index = 0; index < ARRAY_SIZE; index++)
  {
    viskores::Id v0 = outArray.ReadPortal().Get(2 * index + 0);
    VISKORES_TEST_ASSERT(v0 == value0, "Bad value in array.");
    viskores::Id v1 = outArray.ReadPortal().Get(2 * index + 1);
    VISKORES_TEST_ASSERT(v1 == value1, "Bad value in array.");
  }
}

} // anonymous namespace

int GuideExampleScatterUniform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
