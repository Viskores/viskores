//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_FieldHistogram_h
#define vtk_m_worklet_FieldHistogram_h

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DeviceAdapter.h>
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
  typedef vtkm::VecTraits<T> VecType;
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
    typedef void ControlSignature(FieldIn<> value, FieldOut<> binIndex);
    typedef void ExecutionSignature(_1, _2);
    typedef _1 InputDomain;

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
    typedef void ControlSignature(FieldIn<IdType> inputIndex,
                                  WholeArrayIn<IdType> counts,
                                  FieldOut<IdType> outputCount);
    typedef void ExecutionSignature(_1, _2, _3);
    typedef _1 InputDomain;

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
  template <typename FieldType, typename Storage, typename DeviceAdapter>
  void Run(vtkm::cont::ArrayHandle<FieldType, Storage> fieldArray,
           vtkm::Id numberOfBins,
           vtkm::Range& rangeOfValues,
           FieldType& binDelta,
           vtkm::cont::ArrayHandle<vtkm::Id>& binArray,
           DeviceAdapter vtkmNotUsed(device))
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;

    //todo: need to have a signature that can use an input range so we can
    //leverage fields that have already computed there range

    const vtkm::Id numberOfValues = fieldArray.GetNumberOfValues();

    const vtkm::Vec<FieldType, 2> initValue(fieldArray.GetPortalConstControl().Get(0));

    vtkm::Vec<FieldType, 2> result =
      DeviceAlgorithms::Reduce(fieldArray, initValue, vtkm::MinAndMax<FieldType>());

    const FieldType& fieldMinValue = result[0];
    const FieldType& fieldMaxValue = result[1];
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
    DeviceAlgorithms::Sort(binIndex);

    // Get the upper bound of each bin number
    vtkm::cont::ArrayHandle<vtkm::Id> totalCount;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> binCounter(0, 1, numberOfBins);
    DeviceAlgorithms::UpperBounds(binIndex, binCounter, totalCount);

    // Difference between adjacent items is the bin count
    vtkm::worklet::DispatcherMapField<AdjacentDifference, DeviceAdapter> dispatcher;
    dispatcher.Invoke(binCounter, totalCount, binArray);

    //update the users data
    rangeOfValues = vtkm::Range(fieldMinValue, fieldMaxValue);
    binDelta = fieldDelta;
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_FieldHistogram_h
