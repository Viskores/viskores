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
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm {
namespace worklet {

struct DivideWorklet: public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2, _3);

  template <class ValueType>
  VTKM_EXEC
  void operator()(const ValueType &v, const vtkm::Id &count, ValueType &vout) const
  {
    typedef typename VecTraits<ValueType>::ComponentType ComponentType;
    vout = v * ComponentType(1./static_cast<double>(count));
  }

  template <class T1, class T2>
  VTKM_EXEC
  void operator()(const T1&, const vtkm::Id &, T2 &) const
  {  }
};

template <class KeyType, class ValueType,
          class KeyInStorage, class KeyOutStorage,
          class ValueInStorage, class ValueOutStorage,
          class DeviceAdapter>
void AverageByKey(const vtkm::cont::ArrayHandle<KeyType, KeyInStorage> &keyArray,
                  const vtkm::cont::ArrayHandle<ValueType, ValueInStorage> &valueArray,
                  vtkm::cont::ArrayHandle<KeyType, KeyOutStorage> &outputKeyArray,
                  vtkm::cont::ArrayHandle<ValueType, ValueOutStorage> &outputValueArray,
                  DeviceAdapter)
{
  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;
  typedef vtkm::cont::ArrayHandle<ValueType, ValueInStorage> ValueInArray;
  typedef vtkm::cont::ArrayHandle<vtkm::Id> IdArray;
  typedef vtkm::cont::ArrayHandle<ValueType> ValueArray;

  // sort the indexed array
  vtkm::cont::ArrayHandleIndex indexArray(keyArray.GetNumberOfValues());
  IdArray indexArraySorted;
  vtkm::cont::ArrayHandle<KeyType> keyArraySorted;

  Algorithm::Copy( keyArray, keyArraySorted ); // keep the input key array unchanged
  Algorithm::Copy( indexArray, indexArraySorted );
  Algorithm::SortByKey( keyArraySorted, indexArraySorted, vtkm::SortLess() ) ;

  // generate permultation array based on the indexes
  typedef vtkm::cont::ArrayHandlePermutation<IdArray, ValueInArray > PermutatedValueArray;
  PermutatedValueArray valueArraySorted = vtkm::cont::make_ArrayHandlePermutation( indexArraySorted, valueArray );

  // reduce both sumArray and countArray by key
  typedef vtkm::cont::ArrayHandleConstant<vtkm::Id> ConstIdArray;
  ConstIdArray constOneArray(1, valueArray.GetNumberOfValues());
  IdArray countArray;
  ValueArray sumArray;
  vtkm::cont::ArrayHandleZip< PermutatedValueArray, ConstIdArray > inputZipHandle(valueArraySorted, constOneArray);
  vtkm::cont::ArrayHandleZip< ValueArray, IdArray > outputZipHandle(sumArray, countArray);

  Algorithm::ReduceByKey( keyArraySorted, inputZipHandle,
                          outputKeyArray, outputZipHandle,
                         vtkm::Add()  );

  // get average
  DispatcherMapField<DivideWorklet, DeviceAdapter>().Invoke(sumArray,
                                                            countArray,
                                                            outputValueArray);
}

}
} // vtkm::worklet

#endif  //vtk_m_worklet_AverageByKey_h
