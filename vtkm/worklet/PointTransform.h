//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  VTKM_CONT
  PointTransform() {}

  //Translation
  VTKM_CONT void SetTranslation(const T& tx, const T& ty, const T& tz)
  {
    matrix = vtkm::Transform3DTranslate(tx, ty, tz);
  }

  VTKM_CONT void SetTranslation(const vtkm::Vec<T, 3>& v) { SetTranslation(v[0], v[1], v[2]); }

  //Rotation
  VTKM_CONT void SetRotation(const T& angleDegrees, const vtkm::Vec<T, 3>& axis)
  {
    matrix = vtkm::Transform3DRotate(angleDegrees, axis);
  }

  VTKM_CONT void SetRotation(const T& angleDegrees, const T& rx, const T& ry, const T& rz)
  {
    SetRotation(angleDegrees, { rx, ry, rz });
  }

  VTKM_CONT void SetRotationX(const T& angleDegrees) { SetRotation(angleDegrees, 1, 0, 0); }

  VTKM_CONT void SetRotationY(const T& angleDegrees) { SetRotation(angleDegrees, 0, 1, 0); }

  VTKM_CONT void SetRotationZ(const T& angleDegrees) { SetRotation(angleDegrees, 0, 0, 1); }

  //Scaling
  VTKM_CONT void SetScale(const T& s) { matrix = vtkm::Transform3DScale(s, s, s); }

  VTKM_CONT void SetScale(const T& sx, const T& sy, const T& sz)
  {
    matrix = vtkm::Transform3DScale(sx, sy, sz);
  }

  VTKM_CONT void SetScale(const vtkm::Vec<T, 3>& v)
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
