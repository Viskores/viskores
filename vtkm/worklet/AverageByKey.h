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
#include <vtkm/Pair.h>
#include <vtkm/VecTraits.h>

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Timer.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>

namespace vtkm {
namespace worklet {

template <class ValueType>
struct DivideWorklet: public vtkm::worklet::WorkletMapField{
  typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>);
  typedef void ExecutionSignature(_1, _2, _3);

  VTKM_EXEC_EXPORT void operator()(
      const ValueType &v, const vtkm::Id &count, ValueType &vout) const
  {
    typedef typename VecTraits<ValueType>::ComponentType ComponentType;
    vout = v * ComponentType(1./count);
  }
};

template <class KeyType, class ValueType, class DeviceAdapter>
void AverageByKey( const vtkm::cont::ArrayHandle<KeyType> &keyArray,
                   const vtkm::cont::ArrayHandle<ValueType> &valueArray,
                   vtkm::cont::ArrayHandle<KeyType> &outputKeyArray,
                   vtkm::cont::ArrayHandle<ValueType> &outputValueArray)
{
  typedef vtkm::cont::ArrayHandle<ValueType> ValueArray;
  typedef vtkm::cont::ArrayHandle<vtkm::Id> IdArray;
  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;

  // sort the indexed array
  vtkm::cont::ArrayHandleCounting<vtkm::Id> indexArray(0, keyArray.GetNumberOfValues());
  IdArray indexArraySorted;
  vtkm::cont::ArrayHandle<KeyType> keyArraySorted;

  Algorithm::Copy( keyArray, keyArraySorted ); // keep the input key array unchanged
  Algorithm::Copy( indexArray, indexArraySorted );
  Algorithm::SortByKey( keyArraySorted, indexArraySorted, vtkm::SortLess() ) ;

  // generate permultation array based on the indexes
  typedef vtkm::cont::ArrayHandlePermutation<IdArray, ValueArray > PermutatedValueArray;
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
                          vtkm::internal::Add()  );

  // get average
  DispatcherMapField<DivideWorklet<ValueType> >().Invoke(
        sumArray, countArray, outputValueArray);

}

template <class KeyType, class ValueType>
void AverageByKey( const vtkm::cont::ArrayHandle<KeyType> &keyArray,
                   const vtkm::cont::ArrayHandle<ValueType> &valueArray,
                   vtkm::cont::ArrayHandle<KeyType> &outputKeyArray,
                   vtkm::cont::ArrayHandle<ValueType> &outputValueArray)
{
  AverageByKey<KeyType, ValueType, VTKM_DEFAULT_DEVICE_ADAPTER_TAG> (keyArray, valueArray, outputKeyArray, outputValueArray);
}

}} // vtkm::worklet

#endif  //vtk_m_worklet_AverageByKey_h
