//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_FieldStatistics_h
#define vtk_m_worklet_FieldStatistics_h

#include <vtkm/Math.h>
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/Field.h>

#include <stdio.h>

namespace vtkm
{
namespace worklet
{

//simple functor that prints basic statistics
template <typename FieldType>
class FieldStatistics
{
public:
  // For moments readability
  static constexpr vtkm::Id FIRST = 0;
  static constexpr vtkm::Id SECOND = 1;
  static constexpr vtkm::Id THIRD = 2;
  static constexpr vtkm::Id FOURTH = 3;
  static constexpr vtkm::Id NUM_POWERS = 4;

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

  class CalculatePowers : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn value,
                                  FieldOut pow1Array,
                                  FieldOut pow2Array,
                                  FieldOut pow3Array,
                                  FieldOut pow4Array);
    using ExecutionSignature = void(_1, _2, _3, _4, _5);
    using InputDomain = _1;

    vtkm::Id numPowers;

    VTKM_CONT
    CalculatePowers(vtkm::Id num)
      : numPowers(num)
    {
    }

    VTKM_EXEC
    void operator()(const FieldType& value,
                    FieldType& pow1,
                    FieldType& pow2,
                    FieldType& pow3,
                    FieldType& pow4) const
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
    using ControlSignature = void(FieldIn value, FieldOut diff);
    using ExecutionSignature = _2(_1);
    using InputDomain = _1;

    FieldType constant;

    VTKM_CONT
    SubtractConst(const FieldType& constant0)
      : constant(constant0)
    {
    }

    VTKM_EXEC
    FieldType operator()(const FieldType& value) const { return (value - constant); }
  };

  template <typename Storage>
  void Run(vtkm::cont::ArrayHandle<FieldType, Storage> fieldArray, StatInfo& statinfo)
  {
    using DeviceAlgorithms = vtkm::cont::Algorithm;

    // Copy original data to array for sorting
    vtkm::cont::ArrayHandle<FieldType> tempArray;
    DeviceAlgorithms::Copy(fieldArray, tempArray);
    DeviceAlgorithms::Sort(tempArray);

    vtkm::Id dataSize = tempArray.GetNumberOfValues();
    FieldType numValues = static_cast<FieldType>(dataSize);
    const auto firstAndMedian = vtkm::cont::ArrayGetValues({ 0, dataSize / 2 }, tempArray);

    // Median
    statinfo.median = firstAndMedian[1];

    // Minimum and maximum
    const vtkm::Vec<FieldType, 2> initValue(firstAndMedian[0]);
    vtkm::Vec<FieldType, 2> result =
      DeviceAlgorithms::Reduce(fieldArray, initValue, vtkm::MinAndMax<FieldType>());
    statinfo.minimum = result[0];
    statinfo.maximum = result[1];

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
    vtkm::worklet::DispatcherMapField<CalculatePowers> calculatePowersDispatcher(
      CalculatePowers(4));
    calculatePowersDispatcher.Invoke(fieldArray, pow1Array, pow2Array, pow3Array, pow4Array);

    // Accumulate the results using ScanInclusive
    statinfo.rawMoment[FIRST] = DeviceAlgorithms::ScanInclusive(pow1Array, pow1Array) / numValues;
    statinfo.rawMoment[SECOND] = DeviceAlgorithms::ScanInclusive(pow2Array, pow2Array) / numValues;
    statinfo.rawMoment[THIRD] = DeviceAlgorithms::ScanInclusive(pow3Array, pow3Array) / numValues;
    statinfo.rawMoment[FOURTH] = DeviceAlgorithms::ScanInclusive(pow4Array, pow4Array) / numValues;

    // Subtract the mean from every value and leave in tempArray
    vtkm::worklet::DispatcherMapField<SubtractConst> subtractConstDispatcher(
      SubtractConst(statinfo.mean));
    subtractConstDispatcher.Invoke(fieldArray, tempArray);

    // Calculate sums of powers on the (value - mean) array
    calculatePowersDispatcher.Invoke(tempArray, pow1Array, pow2Array, pow3Array, pow4Array);

    // Accumulate the results using ScanInclusive
    statinfo.centralMoment[FIRST] =
      DeviceAlgorithms::ScanInclusive(pow1Array, pow1Array) / numValues;
    statinfo.centralMoment[SECOND] =
      DeviceAlgorithms::ScanInclusive(pow2Array, pow2Array) / numValues;
    statinfo.centralMoment[THIRD] =
      DeviceAlgorithms::ScanInclusive(pow3Array, pow3Array) / numValues;
    statinfo.centralMoment[FOURTH] =
      DeviceAlgorithms::ScanInclusive(pow4Array, pow4Array) / numValues;

    // Statistics from the moments
    statinfo.variance = statinfo.centralMoment[SECOND];
    statinfo.stddev = Sqrt(statinfo.variance);
    statinfo.skewness =
      statinfo.centralMoment[THIRD] / Pow(statinfo.stddev, static_cast<FieldType>(3.0));
    statinfo.kurtosis =
      statinfo.centralMoment[FOURTH] / Pow(statinfo.stddev, static_cast<FieldType>(4.0));
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_FieldStatistics_h
