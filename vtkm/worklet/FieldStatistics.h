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

#ifndef vtk_m_worklet_FieldStatistics_h
#define vtk_m_worklet_FieldStatistics_h

#include <vtkm/Math.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/cont/Field.h>

#include <stdio.h>

#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

typedef VTKM_DEFAULT_DEVICE_ADAPTER_TAG DeviceAdapter;

namespace vtkm {
namespace worklet {

//simple functor that prints basic statistics
template<typename FieldType, typename DeviceAdapter>
class FieldStatistics
{
public:

  // For moments readability
  static const vtkm::Id FIRST = 0;
  static const vtkm::Id SECOND = 1;
  static const vtkm::Id THIRD = 2;
  static const vtkm::Id FOURTH = 3;
  static const vtkm::Id NUM_POWERS = 4;

  static const FieldType THREE = static_cast<FieldType>(3.0);
  static const FieldType FOUR  = static_cast<FieldType>(4.0);

  struct StatInfo
  {
    FieldType minimum;
    FieldType maximum;
    FieldType median;
    FieldType mean;
    FieldType variance;
    FieldType stddev;
    FieldType skewness;
    FieldType kurtosis;
    FieldType rawMoment[4];
    FieldType centralMoment[4];
  };

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

  class CalculatePowers : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<> value,
                                  FieldOut<> pow1Array,
                                  FieldOut<> pow2Array,
                                  FieldOut<> pow3Array,
                                  FieldOut<> pow4Array);
    typedef void ExecutionSignature(_1,_2,_3,_4,_5);
    typedef _1 InputDomain;
  
    vtkm::Id numPowers;

    VTKM_CONT_EXPORT
    CalculatePowers(vtkm::Id num) : numPowers(num) {}
  
    VTKM_EXEC_EXPORT
    void operator()(const FieldType& value, 
                    FieldType &pow1,
                    FieldType &pow2,
                    FieldType &pow3,
                    FieldType &pow4) const
    {
      pow1 = value;
      pow2 = pow1 * value;
      pow3 = pow2 * value;
      pow4 = pow3 * value;
    }
  };

  class SubtractConst : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<> value,
                                  FieldOut<> diff);
    typedef _2 ExecutionSignature(_1);
    typedef _1 InputDomain;
  
    FieldType constant;
  
    VTKM_CONT_EXPORT
    SubtractConst(const FieldType& constant0) : constant(constant0) {}
  
    VTKM_CONT_EXPORT
    FieldType operator()(const FieldType& value) const
    {
      return(value - constant);
    }
  };

  void Run(vtkm::cont::ArrayHandle<FieldType> fieldArray,
           StatInfo& statinfo)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithms;
    typedef typename vtkm::cont::ArrayHandle<FieldType>::PortalControl FieldPortal;

    // Copy original data to array for sorting
    vtkm::cont::ArrayHandle<FieldType> tempArray;
    DeviceAlgorithms::Copy(fieldArray, tempArray);
    DeviceAlgorithms::Sort(tempArray);

    FieldPortal tempPortal = tempArray.GetPortalControl();
    int dataSize = tempPortal.GetNumberOfValues();
    FieldType numValues = static_cast<FieldType>(dataSize);

    // Median
    statinfo.median = tempPortal.Get(dataSize / 2);

    // Minimum and maximum
    FieldType initValue = tempPortal.Get(0);
    statinfo.minimum = DeviceAlgorithms::Reduce(fieldArray, initValue, minFunctor());
    statinfo.maximum = DeviceAlgorithms::Reduce(fieldArray, initValue, maxFunctor());

    // Mean
    FieldType sum = DeviceAlgorithms::ScanInclusive(fieldArray, tempArray);
    statinfo.mean = sum / numValues;
    statinfo.rawMoment[FIRST] = sum / numValues;

    // Create the power sum vector for each value
    vtkm::cont::ArrayHandle<FieldType> pow1Array, pow2Array, pow3Array, pow4Array;
    pow1Array.Allocate(dataSize);
    pow2Array.Allocate(dataSize);
    pow3Array.Allocate(dataSize);
    pow4Array.Allocate(dataSize);

    // Raw moments via Worklet
    vtkm::worklet::DispatcherMapField<CalculatePowers> 
            calculatePowersDispatcher(CalculatePowers(4));
    calculatePowersDispatcher.Invoke(fieldArray, pow1Array, pow2Array, pow3Array, pow4Array);

    // Accumulate the results using ScanInclusive
    statinfo.rawMoment[FIRST] = DeviceAlgorithms::ScanInclusive(pow1Array, pow1Array) / numValues;
    statinfo.rawMoment[SECOND] = DeviceAlgorithms::ScanInclusive(pow2Array, pow2Array) / numValues;
    statinfo.rawMoment[THIRD] = DeviceAlgorithms::ScanInclusive(pow3Array, pow3Array) / numValues;
    statinfo.rawMoment[FOURTH] = DeviceAlgorithms::ScanInclusive(pow4Array, pow4Array) / numValues;

    // Subtract the mean from every value and leave in tempArray
    vtkm::worklet::DispatcherMapField<SubtractConst>
            subtractConstDispatcher(SubtractConst(statinfo.mean));
    subtractConstDispatcher.Invoke(fieldArray, tempArray);

    // Calculate sums of powers on the (value - mean) array
    calculatePowersDispatcher.Invoke(tempArray, pow1Array, pow2Array, pow3Array, pow4Array);

    // Accumulate the results using ScanInclusive
    statinfo.centralMoment[FIRST] = DeviceAlgorithms::ScanInclusive(pow1Array, pow1Array) / numValues;
    statinfo.centralMoment[SECOND] = DeviceAlgorithms::ScanInclusive(pow2Array, pow2Array) / numValues;
    statinfo.centralMoment[THIRD] = DeviceAlgorithms::ScanInclusive(pow3Array, pow3Array) / numValues;
    statinfo.centralMoment[FOURTH] = DeviceAlgorithms::ScanInclusive(pow4Array, pow4Array) / numValues;

    // Statistics from the moments
    statinfo.variance = statinfo.centralMoment[SECOND];
    statinfo.stddev = Sqrt(statinfo.variance);
    statinfo.skewness = statinfo.centralMoment[THIRD] / Pow(statinfo.stddev, THREE);
    statinfo.kurtosis = statinfo.centralMoment[FOURTH] / Pow(statinfo.stddev, FOUR);
  };
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_FieldStatistics_h
