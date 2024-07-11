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

#include <vtkm/Math.h>
#include <vtkm/Range.h>
#include <vtkm/StaticAssert.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayRangeCompute.h>

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/testing/Testing.h>

struct SimpleHistogramStruct
{
  ////
  //// BEGIN-EXAMPLE SimpleHistogram
  ////
  struct CountBins : vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn data, AtomicArrayInOut histogramBins);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;

    vtkm::Range HistogramRange;
    vtkm::Id NumberOfBins;

    VTKM_CONT
    CountBins(const vtkm::Range& histogramRange, vtkm::Id& numBins)
      : HistogramRange(histogramRange)
      , NumberOfBins(numBins)
    {
    }

    template<typename T, typename AtomicArrayType>
    VTKM_EXEC void operator()(T value, const AtomicArrayType& histogramBins) const
    {
      vtkm::Float64 interp =
        (value - this->HistogramRange.Min) / this->HistogramRange.Length();
      vtkm::Id bin = static_cast<vtkm::Id>(interp * this->NumberOfBins);
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
  VTKM_CONT static vtkm::cont::ArrayHandle<vtkm::Int32> Run(const InputArray& input,
                                                            vtkm::Id numberOfBins)
  {
    VTKM_IS_ARRAY_HANDLE(InputArray);

    // Histograms only work on scalar values
    using ValueType = typename InputArray::ValueType;
    VTKM_STATIC_ASSERT_MSG(
      (std::is_same<typename vtkm::VecTraits<ValueType>::HasMultipleComponents,
                    vtkm::VecTraitsTagSingleComponent>::value),
      "Histiogram input not a scalar value.");

    vtkm::Range range = vtkm::cont::ArrayRangeCompute(input).ReadPortal().Get(0);

    // Initialize histogram to 0
    vtkm::cont::ArrayHandle<vtkm::Int32> histogram;
    vtkm::cont::Algorithm::Copy(
      vtkm::cont::ArrayHandleConstant<vtkm::Int32>(0, numberOfBins), histogram);

    CountBins histogramWorklet(range, numberOfBins);

    vtkm::cont::Invoker invoker;
    invoker(histogramWorklet, input, histogram);

    return histogram;
  }
};

VTKM_CONT
static inline void TrySimpleHistogram()
{
  std::cout << "Try Simple Histogram" << std::endl;

  static const vtkm::Id ARRAY_SIZE = 100;
  vtkm::cont::ArrayHandle<vtkm::Float32> inputArray;
  inputArray.Allocate(ARRAY_SIZE);
  SetPortal(inputArray.WritePortal());

  vtkm::cont::ArrayHandle<vtkm::Int32> histogram =
    SimpleHistogramStruct::Run(inputArray, ARRAY_SIZE / 2);

  VTKM_TEST_ASSERT(histogram.GetNumberOfValues() == ARRAY_SIZE / 2, "Bad array size");
  for (vtkm::Id index = 0; index < histogram.GetNumberOfValues(); ++index)
  {
    vtkm::Int32 binSize = histogram.ReadPortal().Get(index);
    VTKM_TEST_ASSERT(binSize == 2, "Bad bin size.");
  }
}

int GuideExampleSimpleHistogram(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TrySimpleHistogram, argc, argv);
}
