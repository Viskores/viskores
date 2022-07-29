//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_transform_PointTransform_h
#define vtk_m_filter_field_transform_PointTransform_h

#include <vtkm/Matrix.h>
#include <vtkm/Transform3D.h>

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
/// \brief
///
/// Generate scalar field from a dataset.
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT PointTransform : public vtkm::filter::NewFilterField
{
public:
  VTKM_CONT
  PointTransform();

  //Translation
  VTKM_CONT void SetTranslation(const vtkm::FloatDefault& tx,
                                const vtkm::FloatDefault& ty,
                                const vtkm::FloatDefault& tz)
  {
    matrix = vtkm::Transform3DTranslate(tx, ty, tz);
  }

  VTKM_CONT void SetTranslation(const vtkm::Vec<vtkm::FloatDefault, 3>& v)
  {
    SetTranslation(v[0], v[1], v[2]);
  }

  //Rotation
  VTKM_CONT void SetRotation(const vtkm::FloatDefault& angleDegrees,
                             const vtkm::Vec<vtkm::FloatDefault, 3>& axis)
  {
    matrix = vtkm::Transform3DRotate(angleDegrees, axis);
  }

  VTKM_CONT void SetRotation(const vtkm::FloatDefault& angleDegrees,
                             const vtkm::FloatDefault& rx,
                             const vtkm::FloatDefault& ry,
                             const vtkm::FloatDefault& rz)
  {
    SetRotation(angleDegrees, { rx, ry, rz });
  }

  VTKM_CONT void SetRotationX(const vtkm::FloatDefault& angleDegrees)
  {
    SetRotation(angleDegrees, 1, 0, 0);
  }

  VTKM_CONT void SetRotationY(const vtkm::FloatDefault& angleDegrees)
  {
    SetRotation(angleDegrees, 0, 1, 0);
  }

  VTKM_CONT void SetRotationZ(const vtkm::FloatDefault& angleDegrees)
  {
    SetRotation(angleDegrees, 0, 0, 1);
  }

  //Scaling
  VTKM_CONT void SetScale(const vtkm::FloatDefault& s) { matrix = vtkm::Transform3DScale(s, s, s); }

  VTKM_CONT void SetScale(const vtkm::FloatDefault& sx,
                          const vtkm::FloatDefault& sy,
                          const vtkm::FloatDefault& sz)
  {
    matrix = vtkm::Transform3DScale(sx, sy, sz);
  }

  VTKM_CONT void SetScale(const vtkm::Vec<vtkm::FloatDefault, 3>& v)
  {
    matrix = vtkm::Transform3DScale(v[0], v[1], v[2]);
  }

  //General transformation
  VTKM_CONT
  void SetTransform(const vtkm::Matrix<vtkm::FloatDefault, 4, 4>& mtx) { matrix = mtx; }

  void SetChangeCoordinateSystem(bool flag);
  bool GetChangeCoordinateSystem() const;

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Matrix<vtkm::FloatDefault, 4, 4> matrix;
  bool ChangeCoordinateSystem = true;
};
} // namespace field_transform
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::field_transform::PointTransform.") PointTransform
  : public vtkm::filter::field_transform::PointTransform
{
  using field_transform::PointTransform::PointTransform;
};
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_transform_PointTransform_h
