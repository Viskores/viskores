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
#include <vtkm/cont/ArrayCopyDevice.h>
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
    using ExecutionSignature = void(_2, _3);
    using InputDomain = _1;

    template <typename ValuesVecType, typename OutType>
    VTKM_EXEC void operator()(const ValuesVecType& valuesIn, OutType& sum) const
    {
      sum = valuesIn[0];
      for (vtkm::IdComponent index = 1; index < valuesIn.GetNumberOfComponents(); ++index)
      {
        sum += valuesIn[index];
      }

      // To get the average, we (of course) divide the sum by the amount of values, which is
      // returned from valuesIn.GetNumberOfComponents(). To do this, we need to cast the number of
      // components (returned as a vtkm::IdComponent) to a FieldType. This is a little more complex
      // than it first seems because FieldType might be a Vec type or a Vec-like type that cannot
      // be constructed. To do this safely, we will do a component-wise divide.
      using VTraits = vtkm::VecTraits<OutType>;
      using ComponentType = typename VTraits::ComponentType;
      ComponentType divisor = static_cast<ComponentType>(valuesIn.GetNumberOfComponents());
      for (vtkm::IdComponent cIndex = 0; cIndex < VTraits::GetNumberOfComponents(sum); ++cIndex)
      {
        VTraits::SetComponent(sum, cIndex, VTraits::GetComponent(sum, cIndex) / divisor);
      }
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
                            const OutArrayType& outAverages)
  {
    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "AverageByKey::Run");

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

  struct ExtractMean
  {
    template <typename ValueType>
    VTKM_EXEC ValueType
    operator()(const vtkm::worklet::DescriptiveStatistics::StatState<ValueType>& state) const
    {
      return state.Mean();
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
    VTKM_LOG_SCOPE(vtkm::cont::LogLevel::Perf, "AverageByKey::Run");

    auto results = vtkm::worklet::DescriptiveStatistics::Run(keyArray, valueArray);
    // Extract results to outputKeyArray and outputValueArray
    outputKeyArray = results.GetFirstArray();
    // TODO: DescriptiveStatistics should write its output to a SOA instead of an AOS.
    // An ArrayHandle of a weird struct by itself is not useful in any general algorithm.
    // In fact, using DescriptiveStatistics at all seems like way overkill. It computes
    // all sorts of statistics, and we then throw them all away except for mean.
    auto resultsMean =
      vtkm::cont::make_ArrayHandleTransform(results.GetSecondArray(), ExtractMean{});
    vtkm::cont::ArrayCopyDevice(resultsMean, outputValueArray);
  }
};
}
} // vtkm::worklet

#endif //vtk_m_worklet_AverageByKey_h
