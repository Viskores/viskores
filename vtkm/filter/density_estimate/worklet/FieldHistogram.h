//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_FieldHistogram_h
#define vtk_m_worklet_FieldHistogram_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/Field.h>

namespace
{
// GCC creates false positive warnings for signed/unsigned char* operations.
// This occurs because the values are implicitly casted up to int's for the
// operation, and than  casted back down to char's when return.
// This causes a false positive warning, even when the values is within
// the value types range
#if defined(VTKM_GCC)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif // gcc
template <typename T>
T compute_delta(T fieldMinValue, T fieldMaxValue, vtkm::Id num)
{
  using VecType = vtkm::VecTraits<T>;
  const T fieldRange = fieldMaxValue - fieldMinValue;
  return fieldRange / static_cast<typename VecType::ComponentType>(num);
}
#if defined(VTKM_GCC)
#pragma GCC diagnostic pop
#endif // gcc
}

namespace vtkm
{
namespace worklet
{

//simple functor that prints basic statistics
class FieldHistogram
{
public:
  // For each value set the bin it should be in
  template <typename FieldType>
  class SetHistogramBin : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn value, FieldOut binIndex);
    using ExecutionSignature = void(_1, _2);
    using InputDomain = _1;

    vtkm::Id numberOfBins;
    FieldType minValue;
    FieldType delta;

    VTKM_CONT
    SetHistogramBin(vtkm::Id numberOfBins0, FieldType minValue0, FieldType delta0)
      : numberOfBins(numberOfBins0)
      , minValue(minValue0)
      , delta(delta0)
    {
    }

    VTKM_EXEC
    void operator()(const FieldType& value, vtkm::Id& binIndex) const
    {
      binIndex = static_cast<vtkm::Id>((value - minValue) / delta);
      if (binIndex < 0)
        binIndex = 0;
      else if (binIndex >= numberOfBins)
        binIndex = numberOfBins - 1;
    }
  };

  // Calculate the adjacent difference between values in ArrayHandle
  class AdjacentDifference : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn inputIndex, WholeArrayIn counts, FieldOut outputCount);
    using ExecutionSignature = void(_1, _2, _3);
    using InputDomain = _1;

    template <typename WholeArrayType>
    VTKM_EXEC void operator()(const vtkm::Id& index,
                              const WholeArrayType& counts,
                              vtkm::Id& difference) const
    {
      if (index == 0)
        difference = counts.Get(index);
      else
        difference = counts.Get(index) - counts.Get(index - 1);
    }
  };

  // Execute the histogram binning filter given data and number of bins
  // Returns:
  // min value of the bins
  // delta/range of each bin
  // number of values in each bin
  template <typename FieldType, typename Storage>
  void Run(vtkm::cont::ArrayHandle<FieldType, Storage> fieldArray,
           vtkm::Id numberOfBins,
           vtkm::Range& rangeOfValues,
           FieldType& binDelta,
           vtkm::cont::ArrayHandle<vtkm::Id>& binArray)
  {
    const vtkm::Vec<FieldType, 2> initValue{ vtkm::cont::ArrayGetValue(0, fieldArray) };

    vtkm::Vec<FieldType, 2> result =
      vtkm::cont::Algorithm::Reduce(fieldArray, initValue, vtkm::MinAndMax<FieldType>());

    this->Run(fieldArray, numberOfBins, result[0], result[1], binDelta, binArray);

    //update the users data
    rangeOfValues = vtkm::Range(result[0], result[1]);
  }

  // Execute the histogram binning filter given data and number of bins, min,
  // max values.
  // Returns:
  // number of values in each bin
  template <typename FieldType, typename Storage>
  void Run(vtkm::cont::ArrayHandle<FieldType, Storage> fieldArray,
           vtkm::Id numberOfBins,
           FieldType fieldMinValue,
           FieldType fieldMaxValue,
           FieldType& binDelta,
           vtkm::cont::ArrayHandle<vtkm::Id>& binArray)
  {
    const vtkm::Id numberOfValues = fieldArray.GetNumberOfValues();

    const FieldType fieldDelta = compute_delta(fieldMinValue, fieldMaxValue, numberOfBins);

    // Worklet fills in the bin belonging to each value
    vtkm::cont::ArrayHandle<vtkm::Id> binIndex;
    binIndex.Allocate(numberOfValues);

    // Worklet to set the bin number for each data value
    SetHistogramBin<FieldType> binWorklet(numberOfBins, fieldMinValue, fieldDelta);
    vtkm::worklet::DispatcherMapField<SetHistogramBin<FieldType>> setHistogramBinDispatcher(
      binWorklet);
    setHistogramBinDispatcher.Invoke(fieldArray, binIndex);

    // Sort the resulting bin array for counting
    vtkm::cont::Algorithm::Sort(binIndex);

    // Get the upper bound of each bin number
    vtkm::cont::ArrayHandle<vtkm::Id> totalCount;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> binCounter(0, 1, numberOfBins);
    vtkm::cont::Algorithm::UpperBounds(binIndex, binCounter, totalCount);

    // Difference between adjacent items is the bin count
    vtkm::worklet::DispatcherMapField<AdjacentDifference> dispatcher;
    dispatcher.Invoke(binCounter, totalCount, binArray);

    //update the users data
    binDelta = fieldDelta;
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_FieldHistogram_h
