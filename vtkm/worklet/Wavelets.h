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

#ifndef vtk_m_worklet_Wavelets_h
#define vtk_m_worklet_Wavelets_h

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/Math.h>

namespace vtkm {
namespace worklet {

namespace internal {

  //  template <typename T>
  //  VTKM_EXEC_EXPORT
  //  T clamp(const T& val, const T& min, const T& max)
  //  {
  //    return vtkm::Min(max, vtkm::Max(min, val));
  //  }

}

class Wavelets : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn<>, FieldOut<>);
  typedef _2 ExecutionSignature(_1);

  VTKM_CONT_EXPORT
  Wavelets() : magicNum(2.0) {}

  VTKM_CONT_EXPORT
  void SetMagicNum(const vtkm::Float64 &num)
  {
    this->magicNum = num;
  }


  VTKM_EXEC_EXPORT
  vtkm::Float64 operator()(const vtkm::Float64 &inputVal) const
  {
    return inputVal * this->magicNum;
  }

  template <typename T>
  VTKM_EXEC_EXPORT
  vtkm::Float64 operator()(const T &inputVal) const
  {
    return (*this)(static_cast<vtkm::Float64>(inputVal));
  }

private:
  vtkm::Float64 magicNum;
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_Wavelets_h
