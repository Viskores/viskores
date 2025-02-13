//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <viskores/Math.h>
#include <viskores/Range.h>
#include <viskores/StaticAssert.h>

#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandle.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/ArrayRangeCompute.h>

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/cont/testing/Testing.h>

struct SimpleHistogramStruct
{
  ////
  //// BEGIN-EXAMPLE SimpleHistogram
  ////
  struct CountBins : viskores::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn data, AtomicArrayInOut histogramBins);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;

    viskores::Range HistogramRange;
    viskores::Id NumberOfBins;

    VISKORES_CONT
    CountBins(const viskores::Range& histogramRange, viskores::Id& numBins)
      : HistogramRange(histogramRange)
      , NumberOfBins(numBins)
    {
    }

    template<typename T, typename AtomicArrayType>
    VISKORES_EXEC void operator()(T value, const AtomicArrayType& histogramBins) const
    {
      viskores::Float64 interp =
        (value - this->HistogramRange.Min) / this->HistogramRange.Length();
      viskores::Id bin = static_cast<viskores::Id>(interp * this->NumberOfBins);
      if (bin < 0)
      {
        bin = 0;
      }
      if (bin >= this->NumberOfBins)
      {
        bin = this->NumberOfBins - 1;
      }

      histogramBins.Add(bin, 1);
    }
  };
  ////
  //// END-EXAMPLE SimpleHistogram
  ////

  template<typename InputArray>
  VISKORES_CONT static viskores::cont::ArrayHandle<viskores::Int32> Run(
    const InputArray& input,
    viskores::Id numberOfBins)
  {
    VISKORES_IS_ARRAY_HANDLE(InputArray);

    // Histograms only work on scalar values
    using ValueType = typename InputArray::ValueType;
    VISKORES_STATIC_ASSERT_MSG(
      (std::is_same<typename viskores::VecTraits<ValueType>::HasMultipleComponents,
                    viskores::VecTraitsTagSingleComponent>::value),
      "Histiogram input not a scalar value.");

    viskores::Range range = viskores::cont::ArrayRangeCompute(input).ReadPortal().Get(0);

    // Initialize histogram to 0
    viskores::cont::ArrayHandle<viskores::Int32> histogram;
    viskores::cont::Algorithm::Copy(
      viskores::cont::ArrayHandleConstant<viskores::Int32>(0, numberOfBins), histogram);

    CountBins histogramWorklet(range, numberOfBins);

    viskores::cont::Invoker invoker;
    invoker(histogramWorklet, input, histogram);

    return histogram;
  }
};

VISKORES_CONT
static inline void TrySimpleHistogram()
{
  std::cout << "Try Simple Histogram" << std::endl;

  static const viskores::Id ARRAY_SIZE = 100;
  viskores::cont::ArrayHandle<viskores::Float32> inputArray;
  inputArray.Allocate(ARRAY_SIZE);
  SetPortal(inputArray.WritePortal());

  viskores::cont::ArrayHandle<viskores::Int32> histogram =
    SimpleHistogramStruct::Run(inputArray, ARRAY_SIZE / 2);

  VISKORES_TEST_ASSERT(histogram.GetNumberOfValues() == ARRAY_SIZE / 2,
                       "Bad array size");
  for (viskores::Id index = 0; index < histogram.GetNumberOfValues(); ++index)
  {
    viskores::Int32 binSize = histogram.ReadPortal().Get(index);
    VISKORES_TEST_ASSERT(binSize == 2, "Bad bin size.");
  }
}

int GuideExampleSimpleHistogram(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TrySimpleHistogram, argc, argv);
}
