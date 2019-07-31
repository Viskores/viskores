//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/TextAnnotationBillboard.h>

#include <vtkm/Matrix.h>

namespace vtkm
{
namespace rendering
{

TextAnnotationBillboard::TextAnnotationBillboard(const std::string& text,
                                                 const vtkm::rendering::Color& color,
                                                 vtkm::Float32 scalar,
                                                 const vtkm::Vec3f_32& position,
                                                 vtkm::Float32 angleDegrees)
  : TextAnnotation(text, color, scalar)
  , Position(position)
  , Angle(angleDegrees)
{
}

TextAnnotationBillboard::~TextAnnotationBillboard()
{
}

void TextAnnotationBillboard::SetPosition(const vtkm::Vec3f_32& position)
{
  this->Position = position;
}

void TextAnnotationBillboard::SetPosition(vtkm::Float32 xpos,
                                          vtkm::Float32 ypos,
                                          vtkm::Float32 zpos)
{
  this->SetPosition(vtkm::make_Vec(xpos, ypos, zpos));
}

void TextAnnotationBillboard::Render(const vtkm::rendering::Camera& camera,
                                     const vtkm::rendering::WorldAnnotator& worldAnnotator,
                                     vtkm::rendering::Canvas& canvas) const
{
  using MatrixType = vtkm::Matrix<vtkm::Float32, 4, 4>;
  using VectorType = vtkm::Vec3f_32;

  MatrixType viewMatrix = camera.CreateViewMatrix();
  MatrixType projectionMatrix =
    camera.CreateProjectionMatrix(canvas.GetWidth(), canvas.GetHeight());

  VectorType screenPos = vtkm::Transform3DPointPerspective(
    vtkm::MatrixMultiply(projectionMatrix, viewMatrix), this->Position);

  canvas.SetViewToScreenSpace(camera, true);
  MatrixType translateMatrix =
    vtkm::Transform3DTranslate(screenPos[0], screenPos[1], -screenPos[2]);

  vtkm::Float32 windowAspect = vtkm::Float32(canvas.GetWidth()) / vtkm::Float32(canvas.GetHeight());

  MatrixType scaleMatrix = vtkm::Transform3DScale(1.f / windowAspect, 1.f, 1.f);

  MatrixType viewportMatrix;
  vtkm::MatrixIdentity(viewportMatrix);
  //if view type == 2D?
  {
    vtkm::Float32 vl, vr, vb, vt;
    camera.GetRealViewport(canvas.GetWidth(), canvas.GetHeight(), vl, vr, vb, vt);
    vtkm::Float32 xs = (vr - vl);
    vtkm::Float32 ys = (vt - vb);
    viewportMatrix = vtkm::Transform3DScale(2.f / xs, 2.f / ys, 1.f);
  }

  MatrixType rotateMatrix = vtkm::Transform3DRotateZ(this->Angle * vtkm::Pi_180f());

  vtkm::Matrix<vtkm::Float32, 4, 4> fullTransformMatrix = vtkm::MatrixMultiply(
    translateMatrix,
    vtkm::MatrixMultiply(scaleMatrix, vtkm::MatrixMultiply(viewportMatrix, rotateMatrix)));

  VectorType origin = vtkm::Transform3DPointPerspective(fullTransformMatrix, VectorType(0, 0, 0));
  VectorType right = vtkm::Transform3DVector(fullTransformMatrix, VectorType(1, 0, 0));
  VectorType up = vtkm::Transform3DVector(fullTransformMatrix, VectorType(0, 1, 0));

  // scale depth from (1, -1) to (0, 1);
  vtkm::Float32 depth = screenPos[2] * .5f + .5f;
  worldAnnotator.AddText(
    origin, right, up, this->Scale, this->Anchor, this->TextColor, this->Text, depth);

  canvas.SetViewToWorldSpace(camera, true);
}
}
} // vtkm::rendering
