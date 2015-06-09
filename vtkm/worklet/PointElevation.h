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

#ifndef vtk_m_worklet_PointElevation_h
#define vtk_m_worklet_PointElevation_h

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm {
namespace worklet {

namespace internal {

template <typename T>
VTKM_EXEC_EXPORT
T clamp(const T& val, const T& min, const T& max)
{
  return (val < min) ? min : ((val > max) ? max : val);
}

}

class PointElevation : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<Scalar>, FieldIn<Scalar>, FieldIn<Scalar>,
                                FieldOut<Scalar>);
  typedef void ExecutionSignature(_1, _2, _3, _4);

  VTKM_CONT_EXPORT
  PointElevation() : LowPoint(0.0, 0.0, 0.0), HighPoint(0.0, 0.0, 1.0),
      RangeLow(0.0), RangeHigh(1.0) {}

  VTKM_CONT_EXPORT
  void SetLowPoint(const vtkm::Vec<vtkm::Float64, 3> &point)
  {
    this->LowPoint = point;
  }

  VTKM_CONT_EXPORT
  void SetHighPoint(const vtkm::Vec<vtkm::Float64, 3> &point)
  {
    this->HighPoint = point;
  }

  VTKM_CONT_EXPORT
  void SetRange(vtkm::Float64 low, vtkm::Float64 high)
  {
    this->RangeLow = low;
    this->RangeHigh = high;
  }

  template <typename T1, typename T2, typename T3, typename T4>
  VTKM_EXEC_EXPORT
  void operator()(const T1 &x, const T2 &y, const T3 &z, T4 &elevation) const
  {
    vtkm::Vec<vtkm::Float64, 3> direction = this->HighPoint - this->LowPoint;
    vtkm::Float64 length = vtkm::dot(direction, direction);
    vtkm::Float64 rangeLength = this->RangeHigh - this->RangeLow;
    vtkm::Vec<vtkm::Float64, 3> vec = vtkm::make_Vec<vtkm::Float64>(x, y, z) -
                                      this->LowPoint;
    vtkm::Float64 s = vtkm::dot(vec, direction) / length;
    s = internal::clamp(s, 0.0, 1.0);
    elevation = static_cast<T4>(this->RangeLow + (s * rangeLength));
  }

private:
  vtkm::Vec<vtkm::Float64, 3> LowPoint, HighPoint;
  vtkm::Float64 RangeLow, RangeHigh;
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_PointElevation_h
