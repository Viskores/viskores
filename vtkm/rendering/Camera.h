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
#ifndef vtk_m_rendering_Camera_h
#define vtk_m_rendering_Camera_h
#include <vtkm/Bounds.h>
#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/Transform3D.h>
#include <vtkm/Range.h>
#include <vtkm/VectorAnalysis.h>
#include <vtkm/rendering/MatrixHelpers.h>

namespace vtkm {
namespace rendering {

class Camera
{
  struct Camera3DStruct
  {
  public:
    VTKM_CONT_EXPORT
    Camera3DStruct()
      : LookAt(0.0f, 0.0f, 0.0f),
        Position(0.0f, 0.0f, 1.0f),
        ViewUp(0.0f, 1.0f, 0.0f),
        FieldOfView(60.0f),
        XPan(0.0f),
        YPan(0.0f),
        Zoom(1.0f)
    {}

    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> CreateViewMatrix() const
    {
      return MatrixHelpers::ViewMatrix(this->Position, this->LookAt, this->ViewUp);
    }

    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> CreateProjectionMatrix(vtkm::Id width,
                                                           vtkm::Id height,
                                                           vtkm::Float32 nearPlane,
                                                           vtkm::Float32 farPlane) const
    {
      vtkm::Matrix<vtkm::Float32,4,4> matrix;
      vtkm::MatrixIdentity(matrix);

      vtkm::Float32 AspectRatio = vtkm::Float32(width) / vtkm::Float32(height);
      vtkm::Float32 fovRad = (this->FieldOfView * 3.1415926f)/180.f;
      fovRad = vtkm::Tan( fovRad * 0.5f);
      vtkm::Float32 size = nearPlane * fovRad;
      vtkm::Float32 left = -size * AspectRatio;
      vtkm::Float32 right = size * AspectRatio;
      vtkm::Float32 bottom = -size;
      vtkm::Float32 top = size;

      matrix(0,0) = 2.f * nearPlane / (right - left);
      matrix(1,1) = 2.f * nearPlane / (top - bottom);
      matrix(0,2) = (right + left) / (right - left);
      matrix(1,2) = (top + bottom) / (top - bottom);
      matrix(2,2) = -(farPlane + nearPlane)  / (farPlane - nearPlane);
      matrix(3,2) = -1.f;
      matrix(2,3) = -(2.f * farPlane * nearPlane) / (farPlane - nearPlane);
      matrix(3,3) = 0.f;

      vtkm::Matrix<vtkm::Float32,4,4> T, Z;
      T = vtkm::Transform3DTranslate(this->XPan, this->YPan, 0.f);
      Z = vtkm::Transform3DScale(this->Zoom, this->Zoom, 1.f);
      matrix = vtkm::MatrixMultiply(Z, vtkm::MatrixMultiply(T, matrix));
      return matrix;
    }


    vtkm::Vec<vtkm::Float32,3> LookAt;
    vtkm::Vec<vtkm::Float32,3> Position;
    vtkm::Vec<vtkm::Float32,3> ViewUp;
    vtkm::Float32 FieldOfView;
    vtkm::Float32 XPan;
    vtkm::Float32 YPan;
    vtkm::Float32 Zoom;
  };

  struct Camera2DStruct
  {
  public:
    VTKM_CONT_EXPORT
    Camera2DStruct()
      : Left(-1.0f), Right(1.0f), Bottom(-1.0f), Top(1.0f), XScale(1.0f)
    {}

    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> CreateViewMatrix() const
    {
      vtkm::Vec<vtkm::Float32,3> lookAt((this->Left + this->Right)/2.f,
                                        (this->Top + this->Bottom)/2.f,
                                        0.f);
      vtkm::Vec<vtkm::Float32,3> position = lookAt;
      position[2] = 1.f;
      vtkm::Vec<vtkm::Float32,3> up(0,1,0);
      return MatrixHelpers::ViewMatrix(position, lookAt, up);
    }

    VTKM_CONT_EXPORT
    vtkm::Matrix<vtkm::Float32,4,4> CreateProjectionMatrix(vtkm::Float32 size,
                                                           vtkm::Float32 near,
                                                           vtkm::Float32 far,
                                                           vtkm::Float32 aspect) const
    {
      vtkm::Matrix<vtkm::Float32,4,4> matrix(0.f);
      vtkm::Float32 left = -size/2.f * aspect;
      vtkm::Float32 right = size/2.f * aspect;
      vtkm::Float32 bottom = -size/2.f;
      vtkm::Float32 top = size/2.f;

      matrix(0,0) = 2.f/(right-left);
      matrix(1,1) = 2.f/(top-bottom);
      matrix(2,2) = -2.f/(far-near);
      matrix(0,3) = -(right+left)/(right-left);
      matrix(1,3) = -(top+bottom)/(top-bottom);
      matrix(2,3) = -(far+near)/(far-near);
      matrix(3,3) = 1.f;
      return matrix;
    }

    vtkm::Float32 Left;
    vtkm::Float32 Right;
    vtkm::Float32 Bottom;
    vtkm::Float32 Top;
    vtkm::Float32 XScale;
  };

public:
  enum ModeEnum { MODE_2D, MODE_3D };
  VTKM_CONT_EXPORT
  Camera(ModeEnum vtype=Camera::MODE_3D)
    : Mode(vtype),
      NearPlane(0.01f),
      FarPlane(1000.0f),
      ViewportLeft(-1.0f),
      ViewportRight(1.0f),
      ViewportBottom(-1.0f),
      ViewportTop(1.0f)
  {}

  VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> CreateViewMatrix() const
  {
    if (this->Mode == Camera::MODE_3D)
    {
      return this->Camera3D.CreateViewMatrix();
    }
    else
    {
      return this->Camera2D.CreateViewMatrix();
    }
  }

  VTKM_CONT_EXPORT
  vtkm::Matrix<vtkm::Float32,4,4> CreateProjectionMatrix(
      vtkm::Id screenWidth, vtkm::Id screenHeight) const
  {
    if (this->Mode == Camera::MODE_3D)
    {
      return this->Camera3D.CreateProjectionMatrix(
            screenWidth, screenHeight, this->NearPlane, this->FarPlane);
    }
    else
    {
      vtkm::Float32 size = vtkm::Abs(this->Camera2D.Top - this->Camera2D.Bottom);
      vtkm::Float32 left,right,bottom,top;
      this->GetRealViewport(screenWidth,screenHeight,left,right,bottom,top);
      vtkm::Float32 aspect =
          (static_cast<vtkm::Float32>(screenWidth)*(right-left)) /
          (static_cast<vtkm::Float32>(screenHeight)*(top-bottom));

      return this->Camera2D.CreateProjectionMatrix(
            size, this->NearPlane, this->FarPlane, aspect);
    }
  }

  VTKM_CONT_EXPORT
  void GetRealViewport(vtkm::Id screenWidth, vtkm::Id screenHeight,
                       vtkm::Float32 &left, vtkm::Float32 &right,
                       vtkm::Float32 &bottom, vtkm::Float32 &top) const
  {
    if (this->Mode == Camera::MODE_3D)
    {
      left = this->ViewportLeft;
      right = this->ViewportRight;
      bottom = this->ViewportBottom;
      top = this->ViewportTop;
    }
    else
    {
      vtkm::Float32 maxvw = (this->ViewportRight-this->ViewportLeft) * static_cast<vtkm::Float32>(screenWidth);
      vtkm::Float32 maxvh = (this->ViewportTop-this->ViewportBottom) * static_cast<vtkm::Float32>(screenHeight);
      vtkm::Float32 waspect = maxvw / maxvh;
      vtkm::Float32 daspect = (this->Camera2D.Right - this->Camera2D.Left) / (this->Camera2D.Top - this->Camera2D.Bottom);
      daspect *= this->Camera2D.XScale;
      //cerr << "waspect="<<waspect << "   \tdaspect="<<daspect<<endl;
      const bool center = true; // if false, anchor to bottom-left
      if (waspect > daspect)
      {
        vtkm::Float32 new_w = (this->ViewportRight-this->ViewportLeft) * daspect / waspect;
        if (center)
        {
          left = (this->ViewportLeft+this->ViewportRight)/2.f - new_w/2.f;
          right = (this->ViewportLeft+this->ViewportRight)/2.f + new_w/2.f;
        }
        else
        {
          left = this->ViewportLeft;
          right = this->ViewportLeft + new_w;
        }
        bottom = this->ViewportBottom;
        top = this->ViewportTop;
      }
      else
      {
        vtkm::Float32 new_h = (this->ViewportTop-this->ViewportBottom) * waspect / daspect;
        if (center)
        {
          bottom = (this->ViewportBottom+this->ViewportTop)/2.f - new_h/2.f;
          top = (this->ViewportBottom+this->ViewportTop)/2.f + new_h/2.f;
        }
        else
        {
          bottom = this->ViewportBottom;
          top = this->ViewportBottom + new_h;
        }
        left = this->ViewportLeft;
        right = this->ViewportRight;
      }
    }
  }

  /// \brief The mode of the camera (2D or 3D).
  ///
  /// \c vtkm::Camera can be set to a 2D or 3D mode. 2D mode is used for
  /// looking at data in the x-y plane. 3D mode allows the camera to be
  /// positioned anywhere and pointing at any place in 3D.
  ///
  VTKM_CONT_EXPORT
  vtkm::rendering::Camera::ModeEnum GetMode() const
  {
    return this->Mode;
  }
  VTKM_CONT_EXPORT
  void SetMode(vtkm::rendering::Camera::ModeEnum mode)
  {
    this->Mode = mode;
  }
  VTKM_CONT_EXPORT
  void SetModeTo3D()
  {
    this->SetMode(vtkm::rendering::Camera::MODE_3D);
  }
  VTKM_CONT_EXPORT
  void SetModeTo2D()
  {
    this->SetMode(vtkm::rendering::Camera::MODE_2D);
  }

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
  VTKM_CONT_EXPORT
  vtkm::Range GetClippingRange() const
  {
    return vtkm::Range(this->NearPlane, this->FarPlane);
  }
  VTKM_CONT_EXPORT
  void SetClippingRange(vtkm::Float32 nearPlane, vtkm::Float32 farPlane)
  {
    this->NearPlane = nearPlane;
    this->FarPlane = farPlane;
  }
  VTKM_CONT_EXPORT
  void SetClippingRange(const vtkm::Range &nearFarRange)
  {
    this->SetClippingRange(static_cast<vtkm::Float32>(nearFarRange.Min),
                           static_cast<vtkm::Float32>(nearFarRange.Max));
  }

  /// \brief The viewport of the projection
  ///
  /// The projection of the camera can be offset to be centered around a subset
  /// of the rendered image. This is established with a "viewport," which is
  /// defined by the left/right and bottom/top of this viewport. The values of
  /// the viewport are relative to the rendered image's bounds. The left and
  /// bottom of the image are at -1 and the right and top are at 1.
  ///
  VTKM_CONT_EXPORT
  void GetViewport(vtkm::Float32 &left,
                   vtkm::Float32 &right,
                   vtkm::Float32 &bottom,
                   vtkm::Float32 &top) const
  {
    left = this->ViewportLeft;
    right = this->ViewportRight;
    bottom = this->ViewportBottom;
    top = this->ViewportTop;
  }
  VTKM_CONT_EXPORT
  vtkm::Bounds GetViewport() const
  {
    return vtkm::Bounds(this->ViewportLeft,
                        this->ViewportRight,
                        this->ViewportBottom,
                        this->ViewportTop,
                        0.0,
                        0.0);
  }
  VTKM_CONT_EXPORT
  void SetViewport(vtkm::Float32 left,
                   vtkm::Float32 right,
                   vtkm::Float32 bottom,
                   vtkm::Float32 top)
  {
    this->ViewportLeft = left;
    this->ViewportRight = right;
    this->ViewportBottom = bottom;
    this->ViewportTop = top;
  }
  VTKM_CONT_EXPORT
  void SetViewport(const vtkm::Bounds &viewportBounds)
  {
    this->SetViewport(static_cast<vtkm::Float32>(viewportBounds.X.Min),
                      static_cast<vtkm::Float32>(viewportBounds.X.Max),
                      static_cast<vtkm::Float32>(viewportBounds.Y.Min),
                      static_cast<vtkm::Float32>(viewportBounds.Y.Max));
  }

  /// \brief The focal point the camera is looking at in 3D mode
  ///
  /// When in 3D mode, the camera is set up to be facing the \c LookAt
  /// position. If \c LookAt is set, the mode is changed to 3D mode.
  ///
  VTKM_CONT_EXPORT
  const vtkm::Vec<vtkm::Float32,3> &GetLookAt() const
  {
    return this->Camera3D.LookAt;
  }
  VTKM_CONT_EXPORT
  void SetLookAt(const vtkm::Vec<vtkm::Float32,3> &lookAt)
  {
    this->SetModeTo3D();
    this->Camera3D.LookAt = lookAt;
  }

  /// \brief The spatial position of the camera in 3D mode
  ///
  /// When in 3D mode, the camera is modeled to be at a particular location. If
  /// \c Position is set, the mode is changed to 3D mode.
  ///
  VTKM_CONT_EXPORT
  const vtkm::Vec<vtkm::Float32,3> &GetPosition() const
  {
    return this->Camera3D.Position;
  }
  VTKM_CONT_EXPORT
  void SetPosition(const vtkm::Vec<vtkm::Float32,3> &position)
  {
    this->SetModeTo3D();
    this->Camera3D.Position = position;
  }

  /// \brief The up orientation of the camera in 3D mode
  ///
  /// When in 3D mode, the camera is modeled to be at a particular location and
  /// looking at a particular spot. The view up vector orients the rotation of
  /// the image so that the top of the image is in the direction pointed to by
  /// view up. If \c ViewUp is set, the mode is changed to 3D mode.
  ///
  VTKM_CONT_EXPORT
  const vtkm::Vec<vtkm::Float32,3> &GetViewUp() const
  {
    return this->Camera3D.ViewUp;
  }
  VTKM_CONT_EXPORT
  void SetViewUp(const vtkm::Vec<vtkm::Float32,3> &viewUp)
  {
    this->SetModeTo3D();
    this->Camera3D.ViewUp = viewUp;
  }

  /// \brief The field of view angle
  ///
  /// The field of view defines the angle (in degrees) that are visible from
  /// the camera position.
  ///
  /// Setting the field of view changes the mode to 3D.
  ///
  VTKM_CONT_EXPORT
  vtkm::Float32 GetFieldOfView() const
  {
    return this->Camera3D.FieldOfView;
  }
  VTKM_CONT_EXPORT
  void SetFieldOfView(vtkm::Float32 fov)
  {
    this->SetModeTo3D();
    this->Camera3D.FieldOfView = fov;
  }

  /// \brief Pans the camera in 3D mode.
  ///
  /// Panning the camera in this way changes the mode to 3D.
  ///
  VTKM_CONT_EXPORT
  void Pan3D(vtkm::Float32 dx, vtkm::Float32 dy)
  {
    this->SetModeTo3D();
    this->Camera3D.XPan += dx;
    this->Camera3D.YPan += dy;
  }
  VTKM_CONT_EXPORT
  void Pan3D(vtkm::Vec<vtkm::Float32,2> direction)
  {
    this->Pan3D(direction[0], direction[1]);
  }

  /// \brief Zooms the camera in or out
  ///
  /// Zooming the camera scales everything in the image up or down. Positive
  /// zoom makes the geometry look bigger or closer. Negative zoom has the
  /// opposite effect. A zoom of 0 has no effect.
  ///
  /// Zooming the camera changes the mode to 3D.
  ///
  VTKM_CONT_EXPORT
  void Zoom3D(vtkm::Float32 zoom)
  {
    this->SetModeTo3D();
    vtkm::Float32 factor = vtkm::Pow(4.0f, zoom);
    this->Camera3D.Zoom *= factor;
    this->Camera3D.XPan *= factor;
    this->Camera3D.YPan *= factor;
  }

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
  VTKM_CONT_EXPORT
  void TrackballRotate(vtkm::Float32 startX,
                       vtkm::Float32 startY,
                       vtkm::Float32 endX,
                       vtkm::Float32 endY)
  {
    vtkm::Matrix<vtkm::Float32,4,4> rotate =
        MatrixHelpers::TrackballMatrix(startX,startY, endX,endY);

    //Translate matrix
    vtkm::Matrix<vtkm::Float32,4,4> translate =
        vtkm::Transform3DTranslate(-this->Camera3D.LookAt);

    //Translate matrix
    vtkm::Matrix<vtkm::Float32,4,4> inverseTranslate =
        vtkm::Transform3DTranslate(this->Camera3D.LookAt);

    vtkm::Matrix<vtkm::Float32,4,4> view = this->CreateViewMatrix();
    view(0,3) = 0;
    view(1,3) = 0;
    view(2,3) = 0;

    vtkm::Matrix<vtkm::Float32,4,4> inverseView = vtkm::MatrixTranspose(view);

    //fullTransform = inverseTranslate * inverseView * rotate * view * translate
    vtkm::Matrix<vtkm::Float32,4,4> fullTransform;
    fullTransform = vtkm::MatrixMultiply(
          inverseTranslate, vtkm::MatrixMultiply(
            inverseView, vtkm::MatrixMultiply(
              rotate, vtkm::MatrixMultiply(
                view,translate))));
    this->Camera3D.Position =
        vtkm::Transform3DPoint(fullTransform, this->Camera3D.Position);
    this->Camera3D.LookAt =
        vtkm::Transform3DPoint(fullTransform, this->Camera3D.LookAt);
    this->Camera3D.ViewUp =
        vtkm::Transform3DVector(fullTransform, this->Camera3D.ViewUp);
  }

  /// \brief Set up the camera to look at geometry
  ///
  /// \c ResetToBounds takes a \c Bounds structure containing the bounds in
  /// 3D space that contain the geometry being rendered. This method sets up
  /// the camera so that it is looking at this region in space. The view
  /// direction is preserved.
  ///
  VTKM_CONT_EXPORT
  void ResetToBounds(const vtkm::Bounds &dataBounds)
  {
    vtkm::Vec<vtkm::Float32,3> directionOfProjection =
        this->GetPosition() - this->GetLookAt();
    vtkm::Normalize(directionOfProjection);

    vtkm::Vec<vtkm::Float32,3> center = dataBounds.Center();
    this->SetLookAt(center);

    vtkm::Vec<vtkm::Float32,3> totalExtent;
    totalExtent[0] = vtkm::Float32(dataBounds.X.Length());
    totalExtent[1] = vtkm::Float32(dataBounds.Y.Length());
    totalExtent[2] = vtkm::Float32(dataBounds.Z.Length());
    vtkm::Float32 diagonalLength = vtkm::Magnitude(totalExtent);
    this->SetPosition(center + directionOfProjection * diagonalLength * 1.0f);
    this->SetFieldOfView(60.0f);
    this->SetClippingRange(1.0f, diagonalLength*10.0f);
  }

  /// \brief The viewable region in the x-y plane
  ///
  /// When the camera is in 2D, it is looking at some region of the x-y plane.
  /// The region being looked at is defined by the range in x (determined by
  /// the left and right sides) and by the range in y (determined by the bottom
  /// and top sides).
  ///
  /// \c SetViewRange2D changes the camera mode to 2D.
  ///
  VTKM_CONT_EXPORT
  void GetViewRange2D(vtkm::Float32 &left,
                      vtkm::Float32 &right,
                      vtkm::Float32 &bottom,
                      vtkm::Float32 &top) const
  {
    left = this->Camera2D.Left;
    right = this->Camera2D.Right;
    bottom = this->Camera2D.Bottom;
    top = this->Camera2D.Top;
  }
  VTKM_CONT_EXPORT
  vtkm::Bounds GetViewRange2D() const
  {
    return vtkm::Bounds(this->Camera2D.Left,
                        this->Camera2D.Right,
                        this->Camera2D.Bottom,
                        this->Camera2D.Top,
                        0.0,
                        0.0);
  }
  VTKM_CONT_EXPORT
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
  }
  VTKM_CONT_EXPORT
  void SetViewRange2D(const vtkm::Range &xRange,
                      const vtkm::Range &yRange)
  {
    this->SetViewRange2D(static_cast<vtkm::Float32>(xRange.Min),
                         static_cast<vtkm::Float32>(xRange.Max),
                         static_cast<vtkm::Float32>(yRange.Min),
                         static_cast<vtkm::Float32>(yRange.Max));
  }
  VTKM_CONT_EXPORT
  void SetViewRange2D(const vtkm::Bounds &viewRange)
  {
    this->SetViewRange2D(viewRange.X, viewRange.Y);
  }

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

}} // namespace vtkm::rendering

#endif // vtk_m_rendering_Camera_h
