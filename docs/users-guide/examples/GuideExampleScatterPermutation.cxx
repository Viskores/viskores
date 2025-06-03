//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include <viskores/cont/ArrayHandleCounting.h>

#include <viskores/worklet/ScatterPermutation.h>
#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/Invoker.h>

#include <viskores/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE ScatterPermutation
////
struct ReverseArrayWorklet : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputArray, FieldOut outputArray);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  using ArrayStorageTag =
    typename viskores::cont::ArrayHandleCounting<viskores::Id>::StorageTag;
  using ScatterType = viskores::worklet::ScatterPermutation<ArrayStorageTag>;

  VISKORES_CONT
  static ScatterType MakeScatter(viskores::Id arraySize)
  {
    return ScatterType(
      viskores::cont::ArrayHandleCounting<viskores::Id>(arraySize - 1, -1, arraySize));
  }

  template<typename FieldType>
  VISKORES_EXEC void operator()(FieldType inputArrayField,
                                FieldType& outputArrayField) const
  {
    outputArrayField = inputArrayField;
  }
};

//
// Later in the associated Filter class...
//

//// PAUSE-EXAMPLE
struct DemoReverseArray
{
  viskores::cont::Invoker Invoke;

  template<typename T, typename Storage>
  VISKORES_CONT viskores::cont::ArrayHandle<T> Run(
    const viskores::cont::ArrayHandle<T, Storage>& inputField)
  {
    //// RESUME-EXAMPLE
    viskores::cont::ArrayHandle<T> outputField;
    this->Invoke(ReverseArrayWorklet{},
                 ReverseArrayWorklet::MakeScatter(inputField.GetNumberOfValues()),
                 inputField,
                 outputField);
    ////
    //// END-EXAMPLE ScatterPermutation
    ////

    return outputField;
  }
};

void Run()
{
  std::cout << "Testing scatter permutation." << std::endl;
  viskores::cont::ArrayHandleCounting<viskores::Float32> inputArray(-2.5f, 0.1f, 51);
  viskores::cont::ArrayHandleCounting<viskores::Float32> resultArray(2.5f, -0.1f, 51);

  VISKORES_TEST_ASSERT(inputArray.GetNumberOfValues() == 51,
                       "Unexpected number of input points.");

  viskores::cont::ArrayHandle<viskores::Float32> reversedArray =
    DemoReverseArray().Run(inputArray);

  VISKORES_TEST_ASSERT(inputArray.GetNumberOfValues() ==
                         reversedArray.GetNumberOfValues(),
                       "Permutation array has wrong size.");
  auto portalScatter = reversedArray.ReadPortal();
  auto portalAccepted = resultArray.ReadPortal();

  for (viskores::Id index = 0; index < portalScatter.GetNumberOfValues(); index++)
  {
    VISKORES_TEST_ASSERT(test_equal(portalScatter.Get(index), portalAccepted.Get(index)),
                         "Permutation array has wrong value.");
  }
}

} // anonymous namespace

int GuideExampleScatterPermutation(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Run, argc, argv);
}
