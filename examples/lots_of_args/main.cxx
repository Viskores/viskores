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
  typedef void ControlSignature( FieldIn<>, FieldIn<>, FieldIn<>,
                                 FieldOut<>, FieldOut<>, FieldOut<> );
  typedef void ExecutionSignature( _1, _2, _3, _4, _5, _6 );

  template<typename T, typename U, typename V>
  VTKM_EXEC_EXPORT
  void operator()( const vtkm::Vec< T, 3 > & vec,
                   const U & scalar1,
                   const V& scalar2,
                   vtkm::Vec<T, 3>& out_vec,
                   U& out_scalar1,
                   V& out_scalar2 ) const
  {
    out_vec = vec * scalar1;
    out_scalar1 = scalar1 + scalar2;
    out_scalar2 = scalar2;
  }

  template<typename T, typename U, typename V, typename W, typename X, typename Y>
  VTKM_EXEC_EXPORT
  void operator()( const T & vec,
                   const U & scalar1,
                   const V& scalar2,
                   W& out_vec,
                   X& out_scalar,
                   Y& ) const
  {
  //no-op
  }
};


int main(int argc, char** argv)
{
  std::vector< vtkm::Vec<vtkm::Float32, 3> > inputVec;
  std::vector< vtkm::Int32 > inputScalar1;
  std::vector< vtkm::Float64 > inputScalar2;

  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32, 3> > handleV =
    vtkm::cont::make_ArrayHandle(inputVec);

  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32, 3> > handleS1 =
    vtkm::cont::make_ArrayHandle(inputVec);

  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32, 3> > handleS2 =
    vtkm::cont::make_ArrayHandle(inputVec);

  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32, 3> > handleOV;
  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32, 3> > handleOS1;
  vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32, 3> > handleOS2;

  std::cout << "Making 3 output DynamicArrayHandles " << std::endl;
  vtkm::cont::DynamicArrayHandle out1(handleOV), out2(handleOS1), out3(handleOS2);

  typedef vtkm::worklet::DispatcherMapField<ExampleFieldWorklet> DispatcherType;

  std::cout << "Invoking ExampleFieldWorklet" << std::endl;
  DispatcherType dispatcher;

  dispatcher.Invoke(handleV, handleS1, handleS2, out1, out2, out3);

}

