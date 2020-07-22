//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_AverageByKey_h
#define vtk_m_worklet_AverageByKey_h

#include <vtkm/VecTraits.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/worklet/DescriptiveStatistics.h>
#include <vtkm/worklet/Keys.h>
#include <vtkm/worklet/WorkletReduceByKey.h>

namespace vtkm
{
namespace worklet
{

struct AverageByKey
{
  struct AverageWorklet : public vtkm::worklet::WorkletReduceByKey
  {
    using ControlSignature = void(KeysIn keys, ValuesIn valuesIn, ReducedValuesOut averages);
    using ExecutionSignature = _3(_2);
    using InputDomain = _1;

    template <typename ValuesVecType>
    VTKM_EXEC typename ValuesVecType::ComponentType operator()(const ValuesVecType& valuesIn) const
    {
      using FieldType = typename ValuesVecType::ComponentType;
      FieldType sum = valuesIn[0];
      for (vtkm::IdComponent index = 1; index < valuesIn.GetNumberOfComponents(); ++index)
      {
        FieldType component = valuesIn[index];
        // FieldType constructor is for when OutType is a Vec.
        // static_cast is for when FieldType is a small int that gets promoted to int32.
        sum = static_cast<FieldType>(sum + component);
      }

      // To get the average, we (of course) divide the sum by the amount of values, which is
      // returned from valuesIn.GetNumberOfComponents(). To do this, we need to cast the number of
      // components (returned as a vtkm::IdComponent) to a FieldType. This is a little more complex
      // than it first seems because FieldType might be a Vec type. If you just try a
      // static_cast<FieldType>(), it will use the constructor to FieldType which might be a Vec
      // constructor expecting the type of the component. So, get around this problem by first
      // casting to the component type of the field and then constructing a field value from that.
      // We use the VecTraits class to make this work regardless of whether FieldType is a real Vec
      // or just a scalar.
      using ComponentType = typename vtkm::VecTraits<FieldType>::ComponentType;
      // FieldType constructor is for when OutType is a Vec.
      // static_cast is for when FieldType is a small int that gets promoted to int32.
      return static_cast<FieldType>(
        sum / FieldType(static_cast<ComponentType>(valuesIn.GetNumberOfComponents())));
    }
  };

  /// \brief Compute average values based on a set of Keys.
  ///
  /// This method uses an existing \c Keys object to collected values by those keys and find
  /// the average of those groups.
  ///
  template <typename InArrayType, typename OutArrayType>
  VTKM_CONT static void Run(const vtkm::worklet::internal::KeysBase& keys,
                            const InArrayType& inValues,
                            OutArrayType& outAverages)
  {

    vtkm::worklet::DispatcherReduceByKey<AverageWorklet> dispatcher;
    dispatcher.Invoke(keys, inValues, outAverages);
  }

  /// \brief Compute average values based on a set of Keys.
  ///
  /// This method uses an existing \c Keys object to collected values by those keys and find
  /// the average of those groups.
  ///
  template <typename ValueType, typename InValuesStorage>
  VTKM_CONT static vtkm::cont::ArrayHandle<ValueType> Run(
    const vtkm::worklet::internal::KeysBase& keys,
    const vtkm::cont::ArrayHandle<ValueType, InValuesStorage>& inValues)
  {

    vtkm::cont::ArrayHandle<ValueType> outAverages;
    Run(keys, inValues, outAverages);
    return outAverages;
  }

  struct ExtractKey
  {
    template <typename First, typename Second>
    VTKM_EXEC First operator()(const vtkm::Pair<First, Second>& pair) const
    {
      return pair.first;
    }
  };

  struct ExtractMean
  {
    template <typename KeyType, typename ValueType>
    VTKM_EXEC ValueType operator()(
      const vtkm::Pair<KeyType, vtkm::worklet::DescriptiveStatistics::StatState<ValueType>>& pair)
      const
    {
      return pair.second.Mean();
    }
  };

  /// \brief Compute average values based on an array of keys.
  ///
  /// This method uses an array of keys and an equally sized array of values. The keys in that
  /// array are collected into groups of equal keys, and the values corresponding to those groups
  /// are averaged.
  ///
  /// This method is less sensitive to constructing large groups with the keys than doing the
  /// similar reduction with a \c Keys object. For example, if you have only one key, the reduction
  /// will still be parallel. However, if you need to run the average of different values with the
  /// same keys, you will have many duplicated operations.
  ///
  template <class KeyType,
            class ValueType,
            class KeyInStorage,
            class KeyOutStorage,
            class ValueInStorage,
            class ValueOutStorage>
  VTKM_CONT static void Run(const vtkm::cont::ArrayHandle<KeyType, KeyInStorage>& keyArray,
                            const vtkm::cont::ArrayHandle<ValueType, ValueInStorage>& valueArray,
                            vtkm::cont::ArrayHandle<KeyType, KeyOutStorage>& outputKeyArray,
                            vtkm::cont::ArrayHandle<ValueType, ValueOutStorage>& outputValueArray)
  {
    auto results = vtkm::worklet::DescriptiveStatistics::Run(keyArray, valueArray);

    // Copy/TransformCopy from results to outputKeyArray and outputValueArray
    auto results_key = vtkm::cont::make_ArrayHandleTransform(results, ExtractKey{});
    auto results_mean = vtkm::cont::make_ArrayHandleTransform(results, ExtractMean{});

    vtkm::cont::ArrayCopy(results_key, outputKeyArray);
    vtkm::cont::ArrayCopy(results_mean, outputValueArray);
  }
};
}
} // vtkm::worklet

#endif //vtk_m_worklet_AverageByKey_h
