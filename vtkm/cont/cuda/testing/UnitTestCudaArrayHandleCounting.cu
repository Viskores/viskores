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
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_CUDA
#define BOOST_SP_DISABLE_THREADS

#include <vtkm/cont/cuda/DeviceAdapterCuda.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/cuda/internal/testing/Testing.h>

namespace ut_handle_counting{

const vtkm::Id ARRAY_SIZE = 300;

struct PassThrough : public vtkm::worklet::WorkletMapField
{
  typedef void ControlSignature(FieldIn<>, FieldOut<>);
  typedef _2 ExecutionSignature(_1);

  template<class ValueType>
  VTKM_EXEC_EXPORT
  ValueType operator()(const ValueType &inValue) const
  { return inValue; }

};


template< typename ValueType >
struct CountingTest
{
  void operator()(const ValueType v) const
  {

    const ValueType start = v;
    const ValueType end = start + ARRAY_SIZE;

    vtkm::cont::ArrayHandleCounting< ValueType > implicit =
        vtkm::cont::make_ArrayHandleCounting(start, end);
    vtkm::cont::ArrayHandle< ValueType > result;

    vtkm::worklet::DispatcherMapField< ut_handle_counting::PassThrough > dispatcher;
    dispatcher.Invoke(implicit, result);

    //verify that the control portal works
    for(int i=v; i < ARRAY_SIZE; ++i)
      {
      const ValueType result_v = result.GetPortalConstControl().Get(i);
      const ValueType correct_value = v + ValueType(i);
      VTKM_TEST_ASSERT(result_v == correct_value, "Counting Handle Failed");
      }
  }

};


template <typename T>
void RunCountingTest(const T t)
{
  CountingTest<T> tests;
  tests(t);
}

void TestArrayHandleCounting()
{
  RunCountingTest( vtkm::Id(42) );
  RunCountingTest( vtkm::Float32(3) );
  // RunCountingTest( vtkm::Vec< vtkm::Float32, 3>() );
}



} // ut_handle_counting namespace

int UnitTestCudaArrayHandleCounting(int, char *[])
{
  return vtkm::cont::cuda::internal::Testing::Run(
                                      ut_handle_counting::TestArrayHandleCounting);
}
