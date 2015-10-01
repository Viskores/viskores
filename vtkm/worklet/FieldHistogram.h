//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/cont/Field.h>

#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

namespace vtkm {
namespace worklet {

//simple functor that prints basic statistics
template<typename FieldType, typename DeviceAdapter>
class FieldHistogram
{
public:

  struct minFunctor
  {
    VTKM_EXEC_EXPORT
    FieldType operator()(const FieldType &x, const FieldType &y) const {
      return Min(x, y);
    }
  };

  struct maxFunctor
  {
    VTKM_EXEC_EXPORT
    FieldType operator()(const FieldType& x, const FieldType& y) const {
      return Max(x, y);
    }
  };

  // For each value set the bin it should be in
  class SetHistogramBin : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<> value,
                                  FieldOut<> binIndex);
    typedef void ExecutionSignature(_1,_2);
    typedef _1 InputDomain;
  
    vtkm::Id numberOfBins;
    FieldType minValue;
    FieldType delta;
  
    VTKM_CONT_EXPORT
    SetHistogramBin(
          vtkm::Id numberOfBins0,
          FieldType minValue0,
          FieldType delta0) :
                  numberOfBins(numberOfBins0),
                  minValue(minValue0),
                  delta(delta0) {}
  
    VTKM_EXEC_EXPORT
    void operator()(const FieldType &value, vtkm::Id &binIndex) const
    {
      binIndex = static_cast<vtkm::Id>((value - minValue) / delta);
      if (binIndex < 0)
        binIndex = 0;
      if (binIndex >= numberOfBins)
        binIndex = numberOfBins - 1;
    }
  };

  // Calculate the adjacent difference between values in ArrayHandle
  class AdjacentDifference : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<IdType> inputIndex,
                                  FieldOut<IdType> outputCount);
    typedef void ExecutionSignature(_1,_2);
    typedef _1 InputDomain;

    typedef typename vtkm::cont::ArrayHandle<vtkm::Id>::ExecutionTypes<DeviceAdapter>::PortalConst IdPortalType;
    IdPortalType totalCountArray;

    VTKM_CONT_EXPORT
    AdjacentDifference(IdPortalType totalCount) :
                       totalCountArray(totalCount) { }

    VTKM_EXEC_EXPORT
    void operator()(const vtkm::Id &index, vtkm::Id & difference) const
    {
      if (index == 0)
        difference = this->totalCountArray.Get(index);
      else
        difference = this->totalCountArray.Get(index) - this->totalCountArray.Get(index - 1);
    }
  };

  // Execute the histogram binning filter given data and number of bins
  void Run(vtkm::cont::ArrayHandle<FieldType> fieldArray, 
           vtkm::Id numberOfBins, 
           FieldType* minValue, 
           FieldType* delta, 
           vtkm::cont::ArrayHandle<vtkm::Id> binArray)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;
    typedef typename vtkm::cont::ArrayHandle<FieldType>::PortalConstControl FieldPortal;

    vtkm::Id numberOfValues = fieldArray.GetNumberOfValues();

    vtkm::cont::ArrayHandle<FieldType> tempArray;
    DeviceAlgorithms::Copy(fieldArray, tempArray);

    FieldPortal tempPortal = tempArray.GetPortalConstControl();
    FieldType initValue = tempPortal.Get(0);
    *minValue = DeviceAlgorithms::Reduce(fieldArray, initValue, minFunctor());
    FieldType maxValue = DeviceAlgorithms::Reduce(fieldArray, initValue, maxFunctor());
    FieldType range = maxValue - *minValue;
    *delta = range / static_cast<FieldType>(numberOfBins);

    // Worklet fills in the bin belonging to each value
    vtkm::cont::ArrayHandle<vtkm::Id> binIndex;
    binIndex.Allocate(numberOfValues);

    // Worklet to set the bin number for each data value
    vtkm::worklet::DispatcherMapField<SetHistogramBin>
      setHistogramBinDispatcher(SetHistogramBin(numberOfBins, *minValue, *delta));
    setHistogramBinDispatcher.Invoke(fieldArray, binIndex);

    // Sort the resulting bin array for counting
    DeviceAlgorithms::Sort(binIndex);

    // Get the upper bound of each bin number
    vtkm::cont::ArrayHandle<vtkm::Id> totalCount;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> binCounter(0, 1, numberOfBins);
    DeviceAlgorithms::UpperBounds(binIndex, binCounter, totalCount);

    // Difference between adjacent items is the bin count
    vtkm::worklet::DispatcherMapField<AdjacentDifference>
      adjacentDifferenceDispatcher(AdjacentDifference(totalCount.PrepareForInput(DeviceAdapter())));
    adjacentDifferenceDispatcher.Invoke(binCounter, binArray);
  }
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_FieldHistogram_h
