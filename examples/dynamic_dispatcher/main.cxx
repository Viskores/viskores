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

#ifndef VTKM_DEVICE_ADAPTER
#define VTKM_DEVICE_ADAPTER VTKM_DEVICE_ADAPTER_SERIAL
#endif

#include <iostream>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

struct ExampleFieldWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature =
    void(FieldIn<>, FieldIn<>, FieldIn<>, FieldOut<>, FieldOut<>, FieldOut<>);
  using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

  template <typename T, typename U, typename V>
  VTKM_EXEC void operator()(const vtkm::Vec<T, 3>& vec,
                            const U& scalar1,
                            const V& scalar2,
                            vtkm::Vec<T, 3>& out_vec,
                            U& out_scalar1,
                            V& out_scalar2) const
  {
    out_vec = vec * scalar1;
    out_scalar1 = static_cast<U>(scalar1 + scalar2);
    out_scalar2 = scalar2;
    std::cout << "hello world" << std::endl;
  }

  template <typename T, typename U, typename V, typename W, typename X, typename Y>
  VTKM_EXEC void operator()(const T&, const U&, const V&, W&, X&, Y&) const
  {
    //no-op
  }
};

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  std::vector<vtkm::Vec<vtkm::Float32, 3>> inputVec(10);
  std::vector<vtkm::Int32> inputScalar1(10);
  std::vector<vtkm::Float64> inputScalar2(10);

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> handleV =
    vtkm::cont::make_ArrayHandle(inputVec);

  vtkm::cont::ArrayHandle<vtkm::Int32> handleS1 = vtkm::cont::make_ArrayHandle(inputScalar1);

  vtkm::cont::ArrayHandle<vtkm::Float64> handleS2 = vtkm::cont::make_ArrayHandle(inputScalar2);

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> handleOV;
  vtkm::cont::ArrayHandle<vtkm::Int32> handleOS1;
  vtkm::cont::ArrayHandle<vtkm::Float64> handleOS2;

  vtkm::cont::DynamicArrayHandle out1(handleOV), out2(handleOS1), out3(handleOS2);

  vtkm::worklet::DispatcherMapField<ExampleFieldWorklet> dispatcher;
  dispatcher.Invoke(handleV, handleS1, handleS2, out1, out2, out3);
}
