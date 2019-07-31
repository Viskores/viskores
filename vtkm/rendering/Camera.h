//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_Camera_h
#define vtk_m_rendering_Camera_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/Bounds.h>
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/Range.h>
#include <vtkm/Transform3D.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/rendering/MatrixHelpers.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT Camera
{
  struct Camera3DStruct
  {
  public:
    VTKM_CONT
    Camera3DStruct()
      : LookAt(0.0f, 0.0f, 0.0f)
      , Position(0.0f, 0.0f, 1.0f)
      , ViewUp(0.0f, 1.0f, 0.0f)
      , FieldOfView(60.0f)
      , XPan(0.0f)
      , YPan(0.0f)
      , Zoom(1.0f)
    {
    }

    vtkm::Matrix<vtkm::Float32, 4, 4> CreateViewMatrix() const;

    vtkm::Matrix<vtkm::Float32, 4, 4> CreateProjectionMatrix(vtkm::Id width,
                                                             vtkm::Id height,
                                                             vtkm::Float32 nearPlane,
                                                             vtkm::Float32 farPlane) const;

    vtkm::Vec3f_32 LookAt;
    vtkm::Vec3f_32 Position;
    vtkm::Vec3f_32 ViewUp;
    vtkm::Float32 FieldOfView;
    vtkm::Float32 XPan;
    vtkm::Float32 YPan;
    vtkm::Float32 Zoom;
  };

  struct VTKM_RENDERING_EXPORT Camera2DStruct
  {
  public:
    VTKM_CONT
    Camera2DStruct()
      : Left(-1.0f)
      , Right(1.0f)
      , Bottom(-1.0f)
      , Top(1.0f)
      , XScale(1.0f)
      , XPan(0.0f)
      , YPan(0.0f)
      , Zoom(1.0f)
    {
    }

    vtkm::Matrix<vtkm::Float32, 4, 4> CreateViewMatrix() const;

    vtkm::Matrix<vtkm::Float32, 4, 4> CreateProjectionMatrix(vtkm::Float32 size,
                                                             vtkm::Float32 znear,
                                                             vtkm::Float32 zfar,
                                                             vtkm::Float32 aspect) const;

    vtkm::Float32 Left;
    vtkm::Float32 Right;
    vtkm::Float32 Bottom;
    vtkm::Float32 Top;
    vtkm::Float32 XScale;
    vtkm::Float32 XPan;
    vtkm::Float32 YPan;
    vtkm::Float32 Zoom;
  };

public:
  enum ModeEnum
  {
    MODE_2D,
    MODE_3D
  };
  VTKM_CONT
  Camera(ModeEnum vtype = Camera::MODE_3D)
    : Mode(vtype)
    , NearPlane(0.01f)
    , FarPlane(1000.0f)
    , ViewportLeft(-1.0f)
    , ViewportRight(1.0f)
    , ViewportBottom(-1.0f)
    , ViewportTop(1.0f)
  {
  }

  vtkm::Matrix<vtkm::Float32, 4, 4> CreateViewMatrix() const;

  vtkm::Matrix<vtkm::Float32, 4, 4> CreateProjectionMatrix(vtkm::Id screenWidth,
                                                           vtkm::Id screenHeight) const;

  void GetRealViewport(vtkm::Id screenWidth,
                       vtkm::Id screenHeight,
                       vtkm::Float32& left,
                       vtkm::Float32& right,
                       vtkm::Float32& bottom,
                       vtkm::Float32& top) const;

  /// \brief The mode of the camera (2D or 3D).
  ///
  /// \c vtkm::Camera can be set to a 2D or 3D mode. 2D mode is used for
  /// looking at data in the x-y plane. 3D mode allows the camera to be
  /// positioned anywhere and pointing at any place in 3D.
  ///
  VTKM_CONT
  vtkm::rendering::Camera::ModeEnum GetMode() const { return this->Mode; }
  VTKM_CONT
  void SetMode(vtkm::rendering::Camera::ModeEnum mode) { this->Mode = mode; }
  VTKM_CONT
  void SetModeTo3D() { this->SetMode(vtkm::rendering::Camera::MODE_3D); }
  VTKM_CONT
  void SetModeTo2D() { this->SetMode(vtkm::rendering::Camera::MODE_2D); }

  /// \brief The clipping range of the camera.
  ///
  /// The clipping range establishes the near and far clipping planes. These
  /// clipping planes are parallel to the viewing plane. The planes are defined
  /// by simply specifying the distance from the viewpoint. Renderers can (and
  /// usually do) remove any geometry closer than the near plane and further
  /// than the far plane.
  ///
  /// For precision purposes, it is best to place the near plane as far away as
  /// possible (while still being in front of any geometry). The far plane
  /// usually has less effect on the depth precision, so can be placed well far
  /// behind the geometry.
  ///
  VTKM_CONT
  vtkm::Range GetClippingRange() const { return vtkm::Range(this->NearPlane, this->FarPlane); }
  VTKM_CONT
  void SetClippingRange(vtkm::Float32 nearPlane, vtkm::Float32 farPlane)
  {
    this->NearPlane = nearPlane;
    this->FarPlane = farPlane;
  }
  VTKM_CONT
  void SetClippingRange(vtkm::Float64 nearPlane, vtkm::Float64 farPlane)
  {
    this->SetClippingRange(static_cast<vtkm::Float32>(nearPlane),
                           static_cast<vtkm::Float32>(farPlane));
  }
  VTKM_CONT
  void SetClippingRange(const vtkm::Range& nearFarRange)
  {
    this->SetClippingRange(nearFarRange.Min, nearFarRange.Max);
  }

  /// \brief The viewport of the projection
  ///
  /// The projection of the camera can be offset to be centered around a subset
  /// of the rendered image. This is established with a "viewport," which is
  /// defined by the left/right and bottom/top of this viewport. The values of
  /// the viewport are relative to the rendered image's bounds. The left and
  /// bottom of the image are at -1 and the right and top are at 1.
  ///
  VTKM_CONT
  void GetViewport(vtkm::Float32& left,
                   vtkm::Float32& right,
                   vtkm::Float32& bottom,
                   vtkm::Float32& top) const
  {
    left = this->ViewportLeft;
    right = this->ViewportRight;
    bottom = this->ViewportBottom;
    top = this->ViewportTop;
  }
  VTKM_CONT
  void GetViewport(vtkm::Float64& left,
                   vtkm::Float64& right,
                   vtkm::Float64& bottom,
                   vtkm::Float64& top) const
  {
    left = this->ViewportLeft;
    right = this->ViewportRight;
    bottom = this->ViewportBottom;
    top = this->ViewportTop;
  }
  VTKM_CONT
  vtkm::Bounds GetViewport() const
  {
    return vtkm::Bounds(
      this->ViewportLeft, this->ViewportRight, this->ViewportBottom, this->ViewportTop, 0.0, 0.0);
  }
  VTKM_CONT
  void SetViewport(vtkm::Float32 left, vtkm::Float32 right, vtkm::Float32 bottom, vtkm::Float32 top)
  {
    this->ViewportLeft = left;
    this->ViewportRight = right;
    this->ViewportBottom = bottom;
    this->ViewportTop = top;
  }
  VTKM_CONT
  void SetViewport(vtkm::Float64 left, vtkm::Float64 right, vtkm::Float64 bottom, vtkm::Float64 top)
  {
    this->SetViewport(static_cast<vtkm::Float32>(left),
                      static_cast<vtkm::Float32>(right),
                      static_cast<vtkm::Float32>(bottom),
                      static_cast<vtkm::Float32>(top));
  }
  VTKM_CONT
  void SetViewport(const vtkm::Bounds& viewportBounds)
  {
    this->SetViewport(
      viewportBounds.X.Min, viewportBounds.X.Max, viewportBounds.Y.Min, viewportBounds.Y.Max);
  }

  /// \brief The focal point the camera is looking at in 3D mode
  ///
  /// When in 3D mode, the camera is set up to be facing the \c LookAt
  /// position. If \c LookAt is set, the mode is changed to 3D mode.
  ///
  VTKM_CONT
  const vtkm::Vec3f_32& GetLookAt() const { return this->Camera3D.LookAt; }
  VTKM_CONT
  void SetLookAt(const vtkm::Vec3f_32& lookAt)
  {
    this->SetModeTo3D();
    this->Camera3D.LookAt = lookAt;
  }
  VTKM_CONT
  void SetLookAt(const vtkm::Vec<Float64, 3>& lookAt)
  {
    this->SetLookAt(vtkm::Vec<Float32, 3>(lookAt));
  }

  /// \brief The spatial position of the camera in 3D mode
  ///
  /// When in 3D mode, the camera is modeled to be at a particular location. If
  /// \c Position is set, the mode is changed to 3D mode.
  ///
  VTKM_CONT
  const vtkm::Vec3f_32& GetPosition() const { return this->Camera3D.Position; }
  VTKM_CONT
  void SetPosition(const vtkm::Vec3f_32& position)
  {
    this->SetModeTo3D();
    this->Camera3D.Position = position;
  }
  VTKM_CONT
  void SetPosition(const vtkm::Vec3f_64& position) { this->SetPosition(vtkm::Vec3f_32(position)); }

  /// \brief The up orientation of the camera in 3D mode
  ///
  /// When in 3D mode, the camera is modeled to be at a particular location and
  /// looking at a particular spot. The view up vector orients the rotation of
  /// the image so that the top of the image is in the direction pointed to by
  /// view up. If \c ViewUp is set, the mode is changed to 3D mode.
  ///
  VTKM_CONT
  const vtkm::Vec3f_32& GetViewUp() const { return this->Camera3D.ViewUp; }
  VTKM_CONT
  void SetViewUp(const vtkm::Vec3f_32& viewUp)
  {
    this->SetModeTo3D();
    this->Camera3D.ViewUp = viewUp;
  }
  VTKM_CONT
  void SetViewUp(const vtkm::Vec3f_64& viewUp) { this->SetViewUp(vtkm::Vec3f_32(viewUp)); }

  /// \brief The xscale of the camera
  ///
  /// The xscale forces the 2D curves to be full-frame
  ///
  /// Setting the xscale changes the mode to 2D.
  ///
  VTKM_CONT
  vtkm::Float32 GetXScale() const { return this->Camera2D.XScale; }
  VTKM_CONT
  void SetXScale(vtkm::Float32 xscale)
  {
    this->SetModeTo2D();
    this->Camera2D.XScale = xscale;
  }
  VTKM_CONT
  void SetXScale(vtkm::Float64 xscale) { this->SetXScale(static_cast<vtkm::Float32>(xscale)); }

  /// \brief The field of view angle
  ///
  /// The field of view defines the angle (in degrees) that are visible from
  /// the camera position.
  ///
  /// Setting the field of view changes the mode to 3D.
  ///
  VTKM_CONT
  vtkm::Float32 GetFieldOfView() const { return this->Camera3D.FieldOfView; }
  VTKM_CONT
  void SetFieldOfView(vtkm::Float32 fov)
  {
    this->SetModeTo3D();
    this->Camera3D.FieldOfView = fov;
  }
  VTKM_CONT
  void SetFieldOfView(vtkm::Float64 fov) { this->SetFieldOfView(static_cast<vtkm::Float32>(fov)); }

  /// \brief Pans the camera
  ///
  void Pan(vtkm::Float32 dx, vtkm::Float32 dy);

  /// \brief Pans the camera
  ///
  VTKM_CONT
  void Pan(vtkm::Float64 dx, vtkm::Float64 dy)
  {
    this->Pan(static_cast<vtkm::Float32>(dx), static_cast<vtkm::Float32>(dy));
  }
  VTKM_CONT
  void Pan(vtkm::Vec2f_32 direction) { this->Pan(direction[0], direction[1]); }

  VTKM_CONT
  void Pan(vtkm::Vec2f_64 direction) { this->Pan(direction[0], direction[1]); }

  VTKM_CONT
  vtkm::Vec2f_32 GetPan() const
  {
    vtkm::Vec2f_32 pan;
    pan[0] = this->Camera3D.XPan;
    pan[1] = this->Camera3D.YPan;
    return pan;
  }


  /// \brief Zooms the camera in or out
  ///
  /// Zooming the camera scales everything in the image up or down. Positive
  /// zoom makes the geometry look bigger or closer. Negative zoom has the
  /// opposite effect. A zoom of 0 has no effect.
  ///
  void Zoom(vtkm::Float32 zoom);

  VTKM_CONT
  void Zoom(vtkm::Float64 zoom) { this->Zoom(static_cast<vtkm::Float32>(zoom)); }

  VTKM_CONT
  vtkm::Float32 GetZoom() const { return this->Camera3D.Zoom; }

  /// \brief Moves the camera as if a point was dragged along a sphere.
  ///
  /// \c TrackballRotate takes the normalized screen coordinates (in the range
  /// -1 to 1) and rotates the camera around the \c LookAt position. The rotation
  /// first projects the points to a sphere around the \c LookAt position. The
  /// camera is then rotated as if the start point was dragged to the end point
  /// along with the world.
  ///
  /// \c TrackballRotate changes the mode to 3D.
  ///
  void TrackballRotate(vtkm::Float32 startX,
                       vtkm::Float32 startY,
                       vtkm::Float32 endX,
                       vtkm::Float32 endY);

  VTKM_CONT
  void TrackballRotate(vtkm::Float64 startX,
                       vtkm::Float64 startY,
                       vtkm::Float64 endX,
                       vtkm::Float64 endY)
  {
    this->TrackballRotate(static_cast<vtkm::Float32>(startX),
                          static_cast<vtkm::Float32>(startY),
                          static_cast<vtkm::Float32>(endX),
                          static_cast<vtkm::Float32>(endY));
  }

  /// \brief Set up the camera to look at geometry
  ///
  /// \c ResetToBounds takes a \c Bounds structure containing the bounds in
  /// 3D space that contain the geometry being rendered. This method sets up
  /// the camera so that it is looking at this region in space. The view
  /// direction is preserved.
  ///
  void ResetToBounds(const vtkm::Bounds& dataBounds);

  /// \brief Set up the camera to look at geometry with padding
  ///
  /// \c ResetToBounds takes a \c Bounds structure containing the bounds in
  /// 3D space that contain the geometry being rendered and a \c Float64 value
  /// representing the percent that a view should be padded in x, y, and z.
  /// This method sets up the camera so that it is looking at this region in
  // space with the given padding percent. The view direction is preserved.
  ///
  void ResetToBounds(const vtkm::Bounds& dataBounds, vtkm::Float64 dataViewPadding);
  void ResetToBounds(const vtkm::Bounds& dataBounds,
                     vtkm::Float64 XDataViewPadding,
                     vtkm::Float64 YDataViewPadding,
                     vtkm::Float64 ZDataViewPadding);

  /// \brief Roll the camera
  ///
  /// Rotates the camera around the view direction by the given angle. The
  /// angle is given in degrees.
  ///
  /// Roll is currently only supported for 3D cameras.
  ///
  void Roll(vtkm::Float32 angleDegrees);

  VTKM_CONT
  void Roll(vtkm::Float64 angleDegrees) { this->Roll(static_cast<vtkm::Float32>(angleDegrees)); }

  /// \brief Rotate the camera about the view up vector centered at the focal point.
  ///
  /// Note that the view up vector is whatever was set via SetViewUp, and is
  /// not necessarily perpendicular to the direction of projection. The angle is
  /// given in degrees.
  ///
  /// Azimuth only makes sense for 3D cameras, so the camera mode will be set
  /// to 3D when this method is called.
  ///
  void Azimuth(vtkm::Float32 angleDegrees);

  VTKM_CONT
  void Azimuth(vtkm::Float64 angleDegrees)
  {
    this->Azimuth(static_cast<vtkm::Float32>(angleDegrees));
  }

  /// \brief Rotate the camera vertically around the focal point.
  ///
  /// Specifically, this rotates the camera about the cross product of the
  /// negative of the direction of projection and the view up vector, using the
  /// focal point (LookAt) as the center of rotation. The angle is given
  /// in degrees.
  ///
  /// Elevation only makes sense for 3D cameras, so the camera mode will be set
  /// to 3D when this method is called.
  ///
  void Elevation(vtkm::Float32 angleDegrees);

  VTKM_CONT
  void Elevation(vtkm::Float64 angleDegrees)
  {
    this->Elevation(static_cast<vtkm::Float32>(angleDegrees));
  }

  /// \brief Move the camera toward or away from the focal point.
  ///
  /// Specifically, this divides the camera's distance from the focal point
  /// (LookAt) by the given value. Use a value greater than one to dolly in
  /// toward the focal point, and use a value less than one to dolly-out away
  /// from the focal point.
  ///
  /// Dolly only makes sense for 3D cameras, so the camera mode will be set to
  /// 3D when this method is called.
  ///
  void Dolly(vtkm::Float32 value);

  VTKM_CONT
  void Dolly(vtkm::Float64 value) { this->Dolly(static_cast<vtkm::Float32>(value)); }

  /// \brief The viewable region in the x-y plane
  ///
  /// When the camera is in 2D, it is looking at some region of the x-y plane.
  /// The region being looked at is defined by the range in x (determined by
  /// the left and right sides) and by the range in y (determined by the bottom
  /// and top sides).
  ///
  /// \c SetViewRange2D changes the camera mode to 2D.
  ///
  VTKM_CONT
  void GetViewRange2D(vtkm::Float32& left,
                      vtkm::Float32& right,
                      vtkm::Float32& bottom,
                      vtkm::Float32& top) const
  {
    left = this->Camera2D.Left;
    right = this->Camera2D.Right;
    bottom = this->Camera2D.Bottom;
    top = this->Camera2D.Top;
  }
  VTKM_CONT
  vtkm::Bounds GetViewRange2D() const
  {
    return vtkm::Bounds(this->Camera2D.Left,
                        this->Camera2D.Right,
                        this->Camera2D.Bottom,
                        this->Camera2D.Top,
                        0.0,
                        0.0);
  }
  VTKM_CONT
  void SetViewRange2D(vtkm::Float32 left,
                      vtkm::Float32 right,
                      vtkm::Float32 bottom,
                      vtkm::Float32 top)
  {
    this->SetModeTo2D();
    this->Camera2D.Left = left;
    this->Camera2D.Right = right;
    this->Camera2D.Bottom = bottom;
    this->Camera2D.Top = top;

    this->Camera2D.XPan = 0;
    this->Camera2D.YPan = 0;
    this->Camera2D.Zoom = 1;
  }
  VTKM_CONT
  void SetViewRange2D(vtkm::Float64 left,
                      vtkm::Float64 right,
                      vtkm::Float64 bottom,
                      vtkm::Float64 top)
  {
    this->SetViewRange2D(static_cast<vtkm::Float32>(left),
                         static_cast<vtkm::Float32>(right),
                         static_cast<vtkm::Float32>(bottom),
                         static_cast<vtkm::Float32>(top));
  }
  VTKM_CONT
  void SetViewRange2D(const vtkm::Range& xRange, const vtkm::Range& yRange)
  {
    this->SetViewRange2D(xRange.Min, xRange.Max, yRange.Min, yRange.Max);
  }
  VTKM_CONT
  void SetViewRange2D(const vtkm::Bounds& viewRange)
  {
    this->SetViewRange2D(viewRange.X, viewRange.Y);
  }

  VTKM_CONT
  void Print() const;

private:
  ModeEnum Mode;
  Camera3DStruct Camera3D;
  Camera2DStruct Camera2D;

  vtkm::Float32 NearPlane;
  vtkm::Float32 FarPlane;

  vtkm::Float32 ViewportLeft;
  vtkm::Float32 ViewportRight;
  vtkm::Float32 ViewportBottom;
  vtkm::Float32 ViewportTop;
};
}
} // namespace vtkm::rendering

#endif // vtk_m_rendering_Camera_h
