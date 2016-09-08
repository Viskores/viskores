//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 Sandia Corporation.
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/View2D.h>

namespace vtkm {
namespace rendering {

View2D::View2D(const vtkm::rendering::Scene &scene,
               const vtkm::rendering::Mapper &mapper,
               const vtkm::rendering::Canvas &canvas,
               const vtkm::rendering::Color &backgroundColor)
  : View(scene, mapper, canvas, backgroundColor)
{
}

View2D::View2D(const vtkm::rendering::Scene &scene,
               const vtkm::rendering::Mapper &mapper,
               const vtkm::rendering::Canvas &canvas,
               const vtkm::rendering::Camera &camera,
               const vtkm::rendering::Color &backgroundColor)
  : View(scene, mapper, canvas, camera, backgroundColor)
{
}

View2D::~View2D()
{
}

void View2D::Paint()
{
  this->GetCanvas().Activate();
  this->GetCanvas().Clear();
  this->SetupForWorldSpace();

  this->GetScene().Render(
        this->GetMapper(), this->GetCanvas(), this->GetCamera());
  this->RenderWorldAnnotations();

  this->SetupForScreenSpace();
  this->RenderScreenAnnotations();

  this->GetCanvas().Finish();
}

void View2D::RenderScreenAnnotations()
{
  vtkm::Float32 viewportLeft;
  vtkm::Float32 viewportRight;
  vtkm::Float32 viewportTop;
  vtkm::Float32 viewportBottom;
  this->GetCamera().GetRealViewport(
        this->GetCanvas().GetWidth(), this->GetCanvas().GetHeight(),
        viewportLeft, viewportRight, viewportBottom, viewportTop);

  this->HorizontalAxisAnnotation.SetColor(vtkm::rendering::Color(1,1,1));
  this->HorizontalAxisAnnotation.SetScreenPosition(
        viewportLeft, viewportBottom, viewportRight, viewportBottom);
  vtkm::Bounds viewRange = this->GetCamera().GetViewRange2D();
  this->HorizontalAxisAnnotation.SetRangeForAutoTicks(viewRange.X.Min,
                                                      viewRange.X.Max);
  this->HorizontalAxisAnnotation.SetMajorTickSize(0, .05, 1.0);
  this->HorizontalAxisAnnotation.SetMinorTickSize(0, .02, 1.0);
  this->HorizontalAxisAnnotation.SetLabelAlignment(TextAnnotation::HCenter,
                                                   TextAnnotation::Top);
  this->HorizontalAxisAnnotation.Render(
        this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  vtkm::Float32 windowaspect =
      vtkm::Float32(this->GetCanvas().GetWidth()) /
      vtkm::Float32(this->GetCanvas().GetHeight());

  this->VerticalAxisAnnotation.SetColor(Color(1,1,1));
  this->VerticalAxisAnnotation.SetScreenPosition(
        viewportLeft, viewportBottom, viewportLeft, viewportTop);
  this->VerticalAxisAnnotation.SetRangeForAutoTicks(viewRange.Y.Min,
                                                    viewRange.Y.Max);
  this->VerticalAxisAnnotation.SetMajorTickSize(.05 / windowaspect, 0, 1.0);
  this->VerticalAxisAnnotation.SetMinorTickSize(.02 / windowaspect, 0, 1.0);
  this->VerticalAxisAnnotation.SetLabelAlignment(TextAnnotation::Right,
                                                 TextAnnotation::VCenter);
  this->VerticalAxisAnnotation.Render(
        this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());

  const vtkm::rendering::Scene &scene = this->GetScene();
  if (scene.GetNumberOfActors() > 0)
  {
    //this->ColorBarAnnotation.SetAxisColor(vtkm::rendering::Color(1,1,1));
    this->ColorBarAnnotation.SetRange(scene.GetActor(0).GetScalarRange().Min,
                                      scene.GetActor(0).GetScalarRange().Max,
                                      5);
    this->ColorBarAnnotation.SetColorTable(scene.GetActor(0).GetColorTable());
    this->ColorBarAnnotation.Render(
          this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
  }
}

void View2D::RenderWorldAnnotations()
{
  // 2D views don't have world annotations.
}

}
} // namespace vtkm::rendering
