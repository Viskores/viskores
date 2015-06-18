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

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>

#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Timer.h>
#include <vtkm/Pair.h>
#include <vtkm/Types.h>
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
        const ValueType &v, vtkm::Id &count, ValueType &vout) const
    {  vout = v * (1./count);  }
};

template <class KeyType, class ValueType, class DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
void AverageByKey( const vtkm::cont::ArrayHandle<KeyType> &keyArray,
                   const vtkm::cont::ArrayHandle<ValueType> &valueArray,
                   vtkm::cont::ArrayHandle<KeyType> &outputKeyArray,
                   vtkm::cont::ArrayHandle<ValueType> &outputValueArray)
{
  vtkm::cont::ArrayHandle<ValueType> sumArray;

  // sort the indexed array
  vtkm::cont::ArrayHandle<KeyType> keyArraySorted;
  vtkm::cont::ArrayHandlePermutation<vtkm::cont::ArrayHandle<vtkm::Id>, vtkm::cont::ArrayHandle<ValueType> >
      valueArraySorted ;
  {
  vtkm::cont::ArrayHandleCounting<vtkm::Id> indexArray(0, keyArray.GetNumberOfValues());
  vtkm::cont::ArrayHandle<vtkm::Id> indexArraySorted;

  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy( keyArray, keyArraySorted );
  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Copy( indexArray, indexArraySorted );
  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::SortByKey( keyArraySorted, indexArraySorted, std::less<KeyType>() ) ;

  valueArraySorted = vtkm::cont::make_ArrayHandlePermutation( indexArraySorted, valueArray );
  }

  vtkm::cont::ArrayHandleConstant<vtkm::Id> constOneArray(1, valueArray.GetNumberOfValues());
  vtkm::cont::ArrayHandle<vtkm::Id> countArray;
#if 1 // reduce twice : fastest
  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ReduceByKey(
        keyArraySorted, valueArraySorted,
        outputKeyArray, sumArray,
        vtkm::internal::Add()  );
  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ReduceByKey(
        keyArraySorted, constOneArray,
        outputKeyArray, countArray,
        vtkm::internal::Add()  );

#else // use zip (a little slower)
  auto inputZipHandle = vtkm::cont::make_ArrayHandleZip(valueArraySorted, constOneArray);
  auto outputZipHandle = vtkm::cont::make_ArrayHandleZip(sumArray, countArray);

  vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::ReduceByKey( keyArraySorted, inputZipHandle,
                                                                  outputKeyArray, outputZipHandle,
                                                                  ZipAdd()  );
#endif

  // get average
  DispatcherMapField<DivideWorklet<ValueType> >().Invoke(
        sumArray, countArray, outputValueArray);

}

}} // vtkm::worklet

#endif  //vtk_m_worklet_AverageByKey_h
