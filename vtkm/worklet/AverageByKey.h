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
#ifndef vtk_m_worklet_AverageByKey_h
#define vtk_m_worklet_AverageByKey_h

#include <vtkm/BinaryPredicates.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherReduceByKey.h>
#include <vtkm/worklet/Keys.h>

namespace vtkm
{
namespace worklet
{

struct AverageByKey
{
  struct AverageWorklet : public vtkm::worklet::WorkletReduceByKey
  {
    using ControlSignature = void(KeysIn keys, ValuesIn<> valuesIn, ReducedValuesOut<> averages);
    using ExecutionSignature = _3(_2);
    using InputDomain = _1;

    template <typename ValuesVecType>
    VTKM_EXEC typename ValuesVecType::ComponentType operator()(const ValuesVecType& valuesIn) const
    {
      using ComponentType = typename ValuesVecType::ComponentType;
      ComponentType sum = valuesIn[0];
      for (vtkm::IdComponent index = 1; index < valuesIn.GetNumberOfComponents(); ++index)
      {
        ComponentType component = valuesIn[index];
        sum = sum + component;
      }
      return sum / static_cast<ComponentType>(valuesIn.GetNumberOfComponents());
    }
  };

  /// \brief Compute average values based on a set of Keys.
  ///
  /// This method uses an existing \c Keys object to collected values by those keys and find
  /// the average of those groups.
  ///
  template <typename KeyType,
            typename ValueType,
            typename InValuesStorage,
            typename OutAveragesStorage,
            typename Device>
  VTKM_CONT static void Run(const vtkm::worklet::Keys<KeyType>& keys,
                            const vtkm::cont::ArrayHandle<ValueType, InValuesStorage>& inValues,
                            vtkm::cont::ArrayHandle<ValueType, OutAveragesStorage>& outAverages,
                            Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::worklet::DispatcherReduceByKey<AverageWorklet, Device> dispatcher;
    dispatcher.Invoke(keys, inValues, outAverages);
  }

  /// \brief Compute average values based on a set of Keys.
  ///
  /// This method uses an existing \c Keys object to collected values by those keys and find
  /// the average of those groups.
  ///
  template <typename KeyType, typename ValueType, typename InValuesStorage, typename Device>
  VTKM_CONT static vtkm::cont::ArrayHandle<ValueType> Run(
    const vtkm::worklet::Keys<KeyType>& keys,
    const vtkm::cont::ArrayHandle<ValueType, InValuesStorage>& inValues,
    Device)
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    vtkm::cont::ArrayHandle<ValueType> outAverages;
    Run(keys, inValues, outAverages, Device());
    return outAverages;
  }


  struct DivideWorklet : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn<>, FieldIn<>, FieldOut<>);
    using ExecutionSignature = void(_1, _2, _3);

    template <class ValueType>
    VTKM_EXEC void operator()(const ValueType& v, const vtkm::Id& count, ValueType& vout) const
    {
      using ComponentType = typename VecTraits<ValueType>::ComponentType;
      vout = v * ComponentType(1. / static_cast<double>(count));
    }

    template <class T1, class T2>
    VTKM_EXEC void operator()(const T1&, const vtkm::Id&, T2&) const
    {
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
            class ValueOutStorage,
            class DeviceAdapter>
  VTKM_CONT static void Run(const vtkm::cont::ArrayHandle<KeyType, KeyInStorage>& keyArray,
                            const vtkm::cont::ArrayHandle<ValueType, ValueInStorage>& valueArray,
                            vtkm::cont::ArrayHandle<KeyType, KeyOutStorage>& outputKeyArray,
                            vtkm::cont::ArrayHandle<ValueType, ValueOutStorage>& outputValueArray,
                            DeviceAdapter)
  {
    using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    using ValueInArray = vtkm::cont::ArrayHandle<ValueType, ValueInStorage>;
    using IdArray = vtkm::cont::ArrayHandle<vtkm::Id>;
    using ValueArray = vtkm::cont::ArrayHandle<ValueType>;

    // sort the indexed array
    vtkm::cont::ArrayHandleIndex indexArray(keyArray.GetNumberOfValues());
    IdArray indexArraySorted;
    vtkm::cont::ArrayHandle<KeyType> keyArraySorted;

    Algorithm::Copy(keyArray, keyArraySorted); // keep the input key array unchanged
    Algorithm::Copy(indexArray, indexArraySorted);
    Algorithm::SortByKey(keyArraySorted, indexArraySorted, vtkm::SortLess());

    // generate permultation array based on the indexes
    using PermutatedValueArray = vtkm::cont::ArrayHandlePermutation<IdArray, ValueInArray>;
    PermutatedValueArray valueArraySorted =
      vtkm::cont::make_ArrayHandlePermutation(indexArraySorted, valueArray);

    // reduce both sumArray and countArray by key
    using ConstIdArray = vtkm::cont::ArrayHandleConstant<vtkm::Id>;
    ConstIdArray constOneArray(1, valueArray.GetNumberOfValues());
    IdArray countArray;
    ValueArray sumArray;
    vtkm::cont::ArrayHandleZip<PermutatedValueArray, ConstIdArray> inputZipHandle(valueArraySorted,
                                                                                  constOneArray);
    vtkm::cont::ArrayHandleZip<ValueArray, IdArray> outputZipHandle(sumArray, countArray);

    Algorithm::ReduceByKey(
      keyArraySorted, inputZipHandle, outputKeyArray, outputZipHandle, vtkm::Add());

    // get average
    DispatcherMapField<DivideWorklet, DeviceAdapter>().Invoke(
      sumArray, countArray, outputValueArray);
  }
};
}
} // vtkm::worklet

#endif //vtk_m_worklet_AverageByKey_h
