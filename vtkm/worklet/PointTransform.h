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

#ifndef vtk_m_worklet_PointTransform_h
#define vtk_m_worklet_PointTransform_h

#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/Transform3D.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{

template <typename T>
class PointTransform : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn<Vec3>, FieldOut<Vec3>);
  using ExecutionSignature = _2(_1);

  VTKM_CONT
  PointTransform() {}

  //Translation
  template <typename S>
  VTKM_CONT void SetTranslation(const S& tx, const S& ty, const S& tz)
  {
    matrix = vtkm::Transform3DTranslate(static_cast<T>(tx), static_cast<T>(ty), static_cast<T>(tz));
  }

  template <typename S>
  VTKM_CONT void SetTranslation(const vtkm::Vec<S, 3>& v)
  {
    SetTranslation(v[0], v[1], v[2]);
  }

  //Rotation
  template <typename S>
  VTKM_CONT void SetRotation(const S& angleDegrees, const vtkm::Vec<S, 3>& axis)
  {
    matrix = vtkm::Transform3DRotate(angleDegrees, axis);
  }

  template <typename S>
  VTKM_CONT void SetRotation(const S& angleDegrees, const S& rx, const S& ry, const S& rz)
  {
    SetRotation(angleDegrees, vtkm::Vec<S, 3>(rx, ry, rz));
  }

  template <typename S>
  VTKM_CONT void SetRotationX(const S& angleDegrees)
  {
    SetRotation(angleDegrees, 1, 0, 0);
  }

  template <typename S>
  VTKM_CONT void SetRotationY(const S& angleDegrees)
  {
    SetRotation(angleDegrees, 0, 1, 0);
  }

  template <typename S>
  VTKM_CONT void SetRotationZ(const S& angleDegrees)
  {
    SetRotation(angleDegrees, 0, 0, 1);
  }

  //Scaling
  template <typename S>
  VTKM_CONT void SetScale(const S& s)
  {
    matrix = vtkm::Transform3DScale(s, s, s);
  }

  template <typename S>
  VTKM_CONT void SetScale(const S& sx, const S& sy, const S& sz)
  {
    matrix = vtkm::Transform3DScale(static_cast<T>(sx), static_cast<T>(sy), static_cast<T>(sz));
  }

  template <typename S>
  VTKM_CONT void SetScale(const vtkm::Vec<S, 3>& v)
  {
    matrix = vtkm::Transform3DScale(v[0], v[1], v[2]);
  }

  //General transformation
  VTKM_CONT
  void SetTransform(const vtkm::Matrix<T, 4, 4>& mtx) { matrix = mtx; }

  //Functor
  VTKM_EXEC
  vtkm::Vec<T, 3> operator()(const vtkm::Vec<T, 3>& vec) const
  {
    return vtkm::Transform3DPoint(matrix, vec);
  }

private:
  vtkm::Matrix<T, 4, 4> matrix;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_PointTransform_h
