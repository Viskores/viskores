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

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
/// \brief Perform affine transforms to point coordinates or vector fields.
///
/// This filter will take a data set and a field of 3 dimensional vectors and perform
/// the specified point transform operation. Several methods are provided to apply
/// many common affine transformations (e.g., translation, rotation, and scale).
/// You can also provide a general 4x4 transformation matrix with `SetTransform()`.
///
/// The main use case for `PointTransform` is to perform transformations of
/// objects in 3D space, which is done by applying these transforms to the
/// coordinate system. This filter will operate on the `vtkm::cont::CoordinateSystem`
/// of the input data unless a different active field is specified. Likewise,
/// this filter will save its results as the first coordinate system in the output
/// unless `SetChangeCoordinateSystem()` is set to say otherwise.
///
/// The default name for the output field is "transform", but that can be overridden as
/// always using the `SetOutputFieldName()` method.
///
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT PointTransform : public vtkm::filter::Filter
{
public:
  VTKM_CONT
  PointTransform();

  /// @brief Translates, or moves, each point in the input field by a given direction.
  VTKM_CONT void SetTranslation(const vtkm::FloatDefault& tx,
                                const vtkm::FloatDefault& ty,
                                const vtkm::FloatDefault& tz)
  {
    matrix = vtkm::Transform3DTranslate(tx, ty, tz);
  }

  /// @copydoc SetTranslation
  VTKM_CONT void SetTranslation(const vtkm::Vec3f& v) { SetTranslation(v[0], v[1], v[2]); }

  /// @brief Rotate the input field about a given axis.
  ///
  /// @param[in] angleDegrees The amount of rotation to perform, given in degrees.
  /// @param[in] axis The rotation is made around a line that goes through the origin
  ///   and pointing in this direction in the counterclockwise direction.
  VTKM_CONT void SetRotation(const vtkm::FloatDefault& angleDegrees, const vtkm::Vec3f& axis)
  {
    matrix = vtkm::Transform3DRotate(angleDegrees, axis);
  }

  /// @brief Rotate the input field about a given axis.
  ///
  /// The rotation is made around a line that goes through the origin
  /// and pointing in the direction specified by @p axisX, @p axisY,
  /// and @p axisZ in the counterclockwise direction.
  ///
  /// @param[in] angleDegrees The amount of rotation to perform, given in degrees.
  /// @param[in] axisX The X value of the rotation axis.
  /// @param[in] axisY The Y value of the rotation axis.
  /// @param[in] axisZ The Z value of the rotation axis.
  VTKM_CONT void SetRotation(const vtkm::FloatDefault& angleDegrees,
                             const vtkm::FloatDefault& axisX,
                             const vtkm::FloatDefault& axisY,
                             const vtkm::FloatDefault& axisZ)
  {
    SetRotation(angleDegrees, { axisX, axisY, axisZ });
  }

  /// @brief Rotate the input field around the X axis by the given degrees.
  VTKM_CONT void SetRotationX(const vtkm::FloatDefault& angleDegrees)
  {
    SetRotation(angleDegrees, 1, 0, 0);
  }

  /// @brief Rotate the input field around the Y axis by the given degrees.
  VTKM_CONT void SetRotationY(const vtkm::FloatDefault& angleDegrees)
  {
    SetRotation(angleDegrees, 0, 1, 0);
  }

  /// @brief Rotate the input field around the Z axis by the given degrees.
  VTKM_CONT void SetRotationZ(const vtkm::FloatDefault& angleDegrees)
  {
    SetRotation(angleDegrees, 0, 0, 1);
  }

  /// @brief Scale the input field.
  ///
  /// Each coordinate is multiplied by tghe associated scale factor.
  VTKM_CONT void SetScale(const vtkm::FloatDefault& s) { matrix = vtkm::Transform3DScale(s, s, s); }

  /// @copydoc SetScale
  VTKM_CONT void SetScale(const vtkm::FloatDefault& sx,
                          const vtkm::FloatDefault& sy,
                          const vtkm::FloatDefault& sz)
  {
    matrix = vtkm::Transform3DScale(sx, sy, sz);
  }

  /// @copydoc SetScale
  VTKM_CONT void SetScale(const vtkm::Vec3f& v)
  {
    matrix = vtkm::Transform3DScale(v[0], v[1], v[2]);
  }

  /// @brief Set a general transformation matrix.
  ///
  /// Each field value is multiplied by this 4x4 as a homogeneous coordinate. That is
  /// a 1 component is added to the end of each 3D vector to put it in the form [x, y, z, 1].
  /// The matrix is then premultiplied to this as a column vector.
  ///
  /// The functions in vtkm/Transform3D.h can be used to help build these transform
  /// matrices.
  VTKM_CONT
  void SetTransform(const vtkm::Matrix<vtkm::FloatDefault, 4, 4>& mtx) { matrix = mtx; }

  /// @brief Specify whether the result should become the coordinate system of the output.
  ///
  /// When this flag is on (the default) the first coordinate system in the output
  /// `vtkm::cont::DataSet` is set to the transformed point coordinates.
  void SetChangeCoordinateSystem(bool flag);
  bool GetChangeCoordinateSystem() const;

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::Matrix<vtkm::FloatDefault, 4, 4> matrix;
  bool ChangeCoordinateSystem = true;
};
} // namespace field_transform
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_field_transform_PointTransform_h
