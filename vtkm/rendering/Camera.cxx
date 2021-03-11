//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/Camera.h>

namespace vtkm
{
namespace rendering
{

vtkm::Matrix<vtkm::Float32, 4, 4> Camera::Camera3DStruct::CreateViewMatrix() const
{
  return MatrixHelpers::ViewMatrix(this->Position, this->LookAt, this->ViewUp);
}

vtkm::Matrix<vtkm::Float32, 4, 4> Camera::Camera3DStruct::CreateProjectionMatrix(
  vtkm::Id width,
  vtkm::Id height,
  vtkm::Float32 nearPlane,
  vtkm::Float32 farPlane) const
{
  vtkm::Matrix<vtkm::Float32, 4, 4> matrix;
  vtkm::MatrixIdentity(matrix);

  vtkm::Float32 AspectRatio = vtkm::Float32(width) / vtkm::Float32(height);
  vtkm::Float32 fovRad = this->FieldOfView * vtkm::Pi_180f();
  fovRad = vtkm::Tan(fovRad * 0.5f);
  vtkm::Float32 size = nearPlane * fovRad;
  vtkm::Float32 left = -size * AspectRatio;
  vtkm::Float32 right = size * AspectRatio;
  vtkm::Float32 bottom = -size;
  vtkm::Float32 top = size;

  matrix(0, 0) = 2.f * nearPlane / (right - left);
  matrix(1, 1) = 2.f * nearPlane / (top - bottom);
  matrix(0, 2) = (right + left) / (right - left);
  matrix(1, 2) = (top + bottom) / (top - bottom);
  matrix(2, 2) = -(farPlane + nearPlane) / (farPlane - nearPlane);
  matrix(3, 2) = -1.f;
  matrix(2, 3) = -(2.f * farPlane * nearPlane) / (farPlane - nearPlane);
  matrix(3, 3) = 0.f;

  vtkm::Matrix<vtkm::Float32, 4, 4> T, Z;
  T = vtkm::Transform3DTranslate(this->XPan, this->YPan, 0.f);
  Z = vtkm::Transform3DScale(this->Zoom, this->Zoom, 1.f);
  matrix = vtkm::MatrixMultiply(Z, vtkm::MatrixMultiply(T, matrix));
  return matrix;
}

//---------------------------------------------------------------------------

vtkm::Matrix<vtkm::Float32, 4, 4> Camera::Camera2DStruct::CreateViewMatrix() const
{
  vtkm::Vec3f_32 lookAt((this->Left + this->Right) / 2.f, (this->Top + this->Bottom) / 2.f, 0.f);
  vtkm::Vec3f_32 position = lookAt;
  position[2] = 1.f;
  vtkm::Vec3f_32 up(0, 1, 0);
  vtkm::Matrix<vtkm::Float32, 4, 4> V = MatrixHelpers::ViewMatrix(position, lookAt, up);
  vtkm::Matrix<vtkm::Float32, 4, 4> scaleMatrix = MatrixHelpers::CreateScale(this->XScale, 1, 1);
  V = vtkm::MatrixMultiply(scaleMatrix, V);
  return V;
}

vtkm::Matrix<vtkm::Float32, 4, 4> Camera::Camera2DStruct::CreateProjectionMatrix(
  vtkm::Float32 size,
  vtkm::Float32 znear,
  vtkm::Float32 zfar,
  vtkm::Float32 aspect) const
{
  vtkm::Matrix<vtkm::Float32, 4, 4> matrix(0.f);
  vtkm::Float32 left = -size / 2.f * aspect;
  vtkm::Float32 right = size / 2.f * aspect;
  vtkm::Float32 bottom = -size / 2.f;
  vtkm::Float32 top = size / 2.f;

  matrix(0, 0) = 2.f / (right - left);
  matrix(1, 1) = 2.f / (top - bottom);
  matrix(2, 2) = -2.f / (zfar - znear);
  matrix(0, 3) = -(right + left) / (right - left);
  matrix(1, 3) = -(top + bottom) / (top - bottom);
  matrix(2, 3) = -(zfar + znear) / (zfar - znear);
  matrix(3, 3) = 1.f;

  vtkm::Matrix<vtkm::Float32, 4, 4> T, Z;
  T = vtkm::Transform3DTranslate(this->XPan, this->YPan, 0.f);
  Z = vtkm::Transform3DScale(this->Zoom, this->Zoom, 1.f);
  matrix = vtkm::MatrixMultiply(Z, vtkm::MatrixMultiply(T, matrix));
  return matrix;
}

//---------------------------------------------------------------------------

vtkm::Matrix<vtkm::Float32, 4, 4> Camera::CreateViewMatrix() const
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

vtkm::Matrix<vtkm::Float32, 4, 4> Camera::CreateProjectionMatrix(vtkm::Id screenWidth,
                                                                 vtkm::Id screenHeight) const
{
  if (this->Mode == Camera::MODE_3D)
  {
    return this->Camera3D.CreateProjectionMatrix(
      screenWidth, screenHeight, this->NearPlane, this->FarPlane);
  }
  else
  {
    vtkm::Float32 size = vtkm::Abs(this->Camera2D.Top - this->Camera2D.Bottom);
    vtkm::Float32 left, right, bottom, top;
    this->GetRealViewport(screenWidth, screenHeight, left, right, bottom, top);
    vtkm::Float32 aspect = (static_cast<vtkm::Float32>(screenWidth) * (right - left)) /
      (static_cast<vtkm::Float32>(screenHeight) * (top - bottom));

    return this->Camera2D.CreateProjectionMatrix(size, this->NearPlane, this->FarPlane, aspect);
  }
}

void Camera::GetRealViewport(vtkm::Id screenWidth,
                             vtkm::Id screenHeight,
                             vtkm::Float32& left,
                             vtkm::Float32& right,
                             vtkm::Float32& bottom,
                             vtkm::Float32& top) const
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
    vtkm::Float32 maxvw =
      (this->ViewportRight - this->ViewportLeft) * static_cast<vtkm::Float32>(screenWidth);
    vtkm::Float32 maxvh =
      (this->ViewportTop - this->ViewportBottom) * static_cast<vtkm::Float32>(screenHeight);
    vtkm::Float32 waspect = maxvw / maxvh;
    vtkm::Float32 daspect =
      (this->Camera2D.Right - this->Camera2D.Left) / (this->Camera2D.Top - this->Camera2D.Bottom);
    daspect *= this->Camera2D.XScale;
//cerr << "waspect="<<waspect << "   \tdaspect="<<daspect<<endl;

//needed as center is a constant value
#if defined(VTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) // conditional expression is constant
#endif

    const bool center = true; // if false, anchor to bottom-left
    if (waspect > daspect)
    {
      vtkm::Float32 new_w = (this->ViewportRight - this->ViewportLeft) * daspect / waspect;
      if (center)
      {
        left = (this->ViewportLeft + this->ViewportRight) / 2.f - new_w / 2.f;
        right = (this->ViewportLeft + this->ViewportRight) / 2.f + new_w / 2.f;
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
      vtkm::Float32 new_h = (this->ViewportTop - this->ViewportBottom) * waspect / daspect;
      if (center)
      {
        bottom = (this->ViewportBottom + this->ViewportTop) / 2.f - new_h / 2.f;
        top = (this->ViewportBottom + this->ViewportTop) / 2.f + new_h / 2.f;
      }
      else
      {
        bottom = this->ViewportBottom;
        top = this->ViewportBottom + new_h;
      }
      left = this->ViewportLeft;
      right = this->ViewportRight;
    }
#if defined(VTKM_MSVC)
#pragma warning(pop)
#endif
  }
}

void Camera::Pan(vtkm::Float32 dx, vtkm::Float32 dy)
{
  this->Camera3D.XPan += dx;
  this->Camera3D.YPan += dy;
  this->Camera2D.XPan += dx;
  this->Camera2D.YPan += dy;
}

void Camera::Zoom(vtkm::Float32 zoom)
{
  vtkm::Float32 factor = vtkm::Pow(4.0f, zoom);
  this->Camera3D.Zoom *= factor;
  this->Camera3D.XPan *= factor;
  this->Camera3D.YPan *= factor;
  this->Camera2D.Zoom *= factor;
  this->Camera2D.XPan *= factor;
  this->Camera2D.YPan *= factor;
}

void Camera::TrackballRotate(vtkm::Float32 startX,
                             vtkm::Float32 startY,
                             vtkm::Float32 endX,
                             vtkm::Float32 endY)
{
  vtkm::Matrix<vtkm::Float32, 4, 4> rotate =
    MatrixHelpers::TrackballMatrix(startX, startY, endX, endY);

  //Translate matrix
  vtkm::Matrix<vtkm::Float32, 4, 4> translate = vtkm::Transform3DTranslate(-this->Camera3D.LookAt);

  //Translate matrix
  vtkm::Matrix<vtkm::Float32, 4, 4> inverseTranslate =
    vtkm::Transform3DTranslate(this->Camera3D.LookAt);

  vtkm::Matrix<vtkm::Float32, 4, 4> view = this->CreateViewMatrix();
  view(0, 3) = 0;
  view(1, 3) = 0;
  view(2, 3) = 0;

  vtkm::Matrix<vtkm::Float32, 4, 4> inverseView = vtkm::MatrixTranspose(view);

  //fullTransform = inverseTranslate * inverseView * rotate * view * translate
  vtkm::Matrix<vtkm::Float32, 4, 4> fullTransform;
  fullTransform = vtkm::MatrixMultiply(
    inverseTranslate,
    vtkm::MatrixMultiply(inverseView,
                         vtkm::MatrixMultiply(rotate, vtkm::MatrixMultiply(view, translate))));
  this->Camera3D.Position = vtkm::Transform3DPoint(fullTransform, this->Camera3D.Position);
  this->Camera3D.LookAt = vtkm::Transform3DPoint(fullTransform, this->Camera3D.LookAt);
  this->Camera3D.ViewUp = vtkm::Transform3DVector(fullTransform, this->Camera3D.ViewUp);
}

void Camera::ResetToBounds(const vtkm::Bounds& dataBounds,
                           const vtkm::Float64 XDataViewPadding,
                           const vtkm::Float64 YDataViewPadding,
                           const vtkm::Float64 ZDataViewPadding)
{
  // Save camera mode
  ModeEnum saveMode = this->GetMode();

  // Pad view around data extents
  vtkm::Bounds db = dataBounds;
  vtkm::Float64 viewPadAmount = XDataViewPadding * (db.X.Max - db.X.Min);
  db.X.Max += viewPadAmount;
  db.X.Min -= viewPadAmount;
  viewPadAmount = YDataViewPadding * (db.Y.Max - db.Y.Min);
  db.Y.Max += viewPadAmount;
  db.Y.Min -= viewPadAmount;
  viewPadAmount = ZDataViewPadding * (db.Z.Max - db.Z.Min);
  db.Z.Max += viewPadAmount;
  db.Z.Min -= viewPadAmount;

  // Reset for 3D camera
  vtkm::Vec3f_32 directionOfProjection = this->GetPosition() - this->GetLookAt();
  vtkm::Normalize(directionOfProjection);

  vtkm::Vec3f_32 center = db.Center();
  this->SetLookAt(center);

  vtkm::Vec3f_32 totalExtent;
  totalExtent[0] = vtkm::Float32(db.X.Length());
  totalExtent[1] = vtkm::Float32(db.Y.Length());
  totalExtent[2] = vtkm::Float32(db.Z.Length());
  vtkm::Float32 diagonalLength = vtkm::Magnitude(totalExtent);
  this->SetPosition(center + directionOfProjection * diagonalLength * 1.0f);
  this->SetFieldOfView(60.0f);
  this->SetClippingRange(0.1f * diagonalLength, diagonalLength * 10.0f);

  // Reset for 2D camera
  this->SetViewRange2D(db);

  // Reset pan and zoom
  this->Camera3D.XPan = 0;
  this->Camera3D.YPan = 0;
  this->Camera3D.Zoom = 1;
  this->Camera2D.XPan = 0;
  this->Camera2D.YPan = 0;
  this->Camera2D.Zoom = 1;

  // Restore camera mode
  this->SetMode(saveMode);
}

// Enable the ability to pad the data extents in the final view
void Camera::ResetToBounds(const vtkm::Bounds& dataBounds, const vtkm::Float64 dataViewPadding)
{
  Camera::ResetToBounds(dataBounds, dataViewPadding, dataViewPadding, dataViewPadding);
}

void Camera::ResetToBounds(const vtkm::Bounds& dataBounds)
{
  Camera::ResetToBounds(dataBounds, 0);
}

void Camera::Roll(vtkm::Float32 angleDegrees)
{
  vtkm::Vec3f_32 directionOfProjection = this->GetLookAt() - this->GetPosition();
  vtkm::Matrix<vtkm::Float32, 4, 4> rotateTransform =
    vtkm::Transform3DRotate(angleDegrees, directionOfProjection);

  this->SetViewUp(vtkm::Transform3DVector(rotateTransform, this->GetViewUp()));
}

void Camera::Azimuth(vtkm::Float32 angleDegrees)
{
  // Translate to the focal point (LookAt), rotate about view up, and
  // translate back again.
  vtkm::Matrix<vtkm::Float32, 4, 4> transform = vtkm::Transform3DTranslate(this->GetLookAt());
  transform =
    vtkm::MatrixMultiply(transform, vtkm::Transform3DRotate(angleDegrees, this->GetViewUp()));
  transform = vtkm::MatrixMultiply(transform, vtkm::Transform3DTranslate(-this->GetLookAt()));

  this->SetPosition(vtkm::Transform3DPoint(transform, this->GetPosition()));
}

void Camera::Elevation(vtkm::Float32 angleDegrees)
{
  vtkm::Vec3f_32 axisOfRotation =
    vtkm::Cross(this->GetPosition() - this->GetLookAt(), this->GetViewUp());

  // Translate to the focal point (LookAt), rotate about the defined axis,
  // and translate back again.
  vtkm::Matrix<vtkm::Float32, 4, 4> transform = vtkm::Transform3DTranslate(this->GetLookAt());
  transform =
    vtkm::MatrixMultiply(transform, vtkm::Transform3DRotate(angleDegrees, axisOfRotation));
  transform = vtkm::MatrixMultiply(transform, vtkm::Transform3DTranslate(-this->GetLookAt()));

  this->SetPosition(vtkm::Transform3DPoint(transform, this->GetPosition()));
}

void Camera::Dolly(vtkm::Float32 value)
{
  if (value <= vtkm::Epsilon32())
  {
    return;
  }

  vtkm::Vec3f_32 lookAtToPos = this->GetPosition() - this->GetLookAt();

  this->SetPosition(this->GetLookAt() + (1.0f / value) * lookAtToPos);
}

void Camera::Print() const
{
  if (Mode == MODE_3D)
  {
    std::cout << "Camera: 3D" << std::endl;
    std::cout << "  LookAt: " << Camera3D.LookAt << std::endl;
    std::cout << "  Pos   : " << Camera3D.Position << std::endl;
    std::cout << "  Up    : " << Camera3D.ViewUp << std::endl;
    std::cout << "  FOV   : " << GetFieldOfView() << std::endl;
    std::cout << "  Clip  : " << GetClippingRange() << std::endl;
    std::cout << "  XyZ   : " << Camera3D.XPan << " " << Camera3D.YPan << " " << Camera3D.Zoom
              << std::endl;
    vtkm::Matrix<vtkm::Float32, 4, 4> pm, vm;
    pm = CreateProjectionMatrix(512, 512);
    vm = CreateViewMatrix();
    std::cout << " PM: " << std::endl;
    std::cout << pm[0][0] << " " << pm[0][1] << " " << pm[0][2] << " " << pm[0][3] << std::endl;
    std::cout << pm[1][0] << " " << pm[1][1] << " " << pm[1][2] << " " << pm[1][3] << std::endl;
    std::cout << pm[2][0] << " " << pm[2][1] << " " << pm[2][2] << " " << pm[2][3] << std::endl;
    std::cout << pm[3][0] << " " << pm[3][1] << " " << pm[3][2] << " " << pm[3][3] << std::endl;
    std::cout << " VM: " << std::endl;
    std::cout << vm[0][0] << " " << vm[0][1] << " " << vm[0][2] << " " << vm[0][3] << std::endl;
    std::cout << vm[1][0] << " " << vm[1][1] << " " << vm[1][2] << " " << vm[1][3] << std::endl;
    std::cout << vm[2][0] << " " << vm[2][1] << " " << vm[2][2] << " " << vm[2][3] << std::endl;
    std::cout << vm[3][0] << " " << vm[3][1] << " " << vm[3][2] << " " << vm[3][3] << std::endl;
  }
  else if (Mode == MODE_2D)
  {
    std::cout << "Camera: 2D" << std::endl;
    std::cout << "  LRBT: " << Camera2D.Left << " " << Camera2D.Right << " " << Camera2D.Bottom
              << " " << Camera2D.Top << std::endl;
    std::cout << "  XY  : " << Camera2D.XPan << " " << Camera2D.YPan << std::endl;
    std::cout << "  SZ  : " << Camera2D.XScale << " " << Camera2D.Zoom << std::endl;
    vtkm::Matrix<vtkm::Float32, 4, 4> pm, vm;
    pm = CreateProjectionMatrix(512, 512);
    vm = CreateViewMatrix();
    std::cout << " PM: " << std::endl;
    std::cout << pm[0][0] << " " << pm[0][1] << " " << pm[0][2] << " " << pm[0][3] << std::endl;
    std::cout << pm[1][0] << " " << pm[1][1] << " " << pm[1][2] << " " << pm[1][3] << std::endl;
    std::cout << pm[2][0] << " " << pm[2][1] << " " << pm[2][2] << " " << pm[2][3] << std::endl;
    std::cout << pm[3][0] << " " << pm[3][1] << " " << pm[3][2] << " " << pm[3][3] << std::endl;
    std::cout << " VM: " << std::endl;
    std::cout << vm[0][0] << " " << vm[0][1] << " " << vm[0][2] << " " << vm[0][3] << std::endl;
    std::cout << vm[1][0] << " " << vm[1][1] << " " << vm[1][2] << " " << vm[1][3] << std::endl;
    std::cout << vm[2][0] << " " << vm[2][1] << " " << vm[2][2] << " " << vm[2][3] << std::endl;
    std::cout << vm[3][0] << " " << vm[3][1] << " " << vm[3][2] << " " << vm[3][3] << std::endl;
  }
}
}
} // namespace vtkm::rendering
