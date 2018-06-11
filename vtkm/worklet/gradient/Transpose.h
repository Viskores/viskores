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

#ifndef vtk_m_worklet_gradient_Transpose_h
#define vtk_m_worklet_gradient_Transpose_h

#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace gradient
{

template <typename T>
struct TransposeType : vtkm::ListTagBase<vtkm::Vec<vtkm::Vec<T, 3>, 3>>
{
};

template <typename T>
struct Transpose3x3 : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldInOut<TransposeType<T>> field);

  using ExecutionSignature = void(_1);
  using InputDomain = _1;

  template <typename FieldInVecType>
  VTKM_EXEC void operator()(FieldInVecType& field) const
  {
    T tempA, tempB, tempC;
    tempA = field[0][1];
    field[0][1] = field[1][0];
    field[1][0] = tempA;
    tempB = field[0][2];
    field[0][2] = field[2][0];
    field[2][0] = tempB;
    tempC = field[1][2];
    field[1][2] = field[2][1];
    field[2][1] = tempC;
  }

  template <typename S, typename DeviceAdapter>
  void Run(vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Vec<T, 3>, 3>, S>& field, DeviceAdapter)
  {
    vtkm::worklet::DispatcherMapField<Transpose3x3<T>, DeviceAdapter> dispatcher;
    dispatcher.Invoke(field);
  }
};
}
}
}

#endif
