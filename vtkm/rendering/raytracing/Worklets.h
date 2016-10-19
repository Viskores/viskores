//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Worklets_h
#define vtk_m_rendering_raytracing_Worklets_h
#include <vtkm/worklet/WorkletMapField.h>
namespace vtkm {
namespace rendering {
namespace raytracing {
//
// Utility memory set functor
//
template<class T>
class MemSet : public vtkm::worklet::WorkletMapField
{
  T Value;
public:
  VTKM_CONT
  MemSet(T value)
    : Value(value)
  {}
  typedef void ControlSignature(FieldOut<>);
  typedef void ExecutionSignature(_1);
  VTKM_EXEC
  void operator()(T &outValue) const
  {
    outValue = Value;
  }
}; //class MemSet

struct MaxValue
{
  template<typename T>
  VTKM_EXEC_CONT T operator()(const T& a,const T& b) const
  {
    return (a > b) ? a : b;
  }

}; //struct MaxValue

struct MinValue
{
  template<typename T>
  VTKM_EXEC_CONT T operator()(const T& a,const T& b) const
  {
    return (a < b) ? a : b;
  }

}; //struct MinValue

}}}//namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Worklets_h
