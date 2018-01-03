//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/rendering/View.h>

namespace vtkm
{
namespace rendering
{

View::View(const vtkm::rendering::Scene& scene,
           const vtkm::rendering::Mapper& mapper,
           const vtkm::rendering::Canvas& canvas,
           const vtkm::rendering::Color& backgroundColor,
           const vtkm::rendering::Color& foregroundColor)
  : Scene(scene)
  , MapperPointer(mapper.NewCopy())
  , CanvasPointer(canvas.NewCopy())
  , WorldAnnotatorPointer(canvas.CreateWorldAnnotator())
{
  this->CanvasPointer->SetBackgroundColor(backgroundColor);
  this->CanvasPointer->SetForegroundColor(foregroundColor);
  this->AxisColor = foregroundColor;

  vtkm::Bounds spatialBounds = this->Scene.GetSpatialBounds();
  this->Camera.ResetToBounds(spatialBounds);
  if (spatialBounds.Z.Length() > 0.0)
  {
    this->Camera.SetModeTo3D();
  }
  else
  {
    this->Camera.SetModeTo2D();
  }
}

View::View(const vtkm::rendering::Scene& scene,
           const vtkm::rendering::Mapper& mapper,
           const vtkm::rendering::Canvas& canvas,
           const vtkm::rendering::Camera& camera,
           const vtkm::rendering::Color& backgroundColor,
           const vtkm::rendering::Color& foregroundColor)
  : Scene(scene)
  , MapperPointer(mapper.NewCopy())
  , CanvasPointer(canvas.NewCopy())
  , WorldAnnotatorPointer(CanvasPointer->CreateWorldAnnotator())
  , Camera(camera)
{
  this->CanvasPointer->SetBackgroundColor(backgroundColor);
  this->CanvasPointer->SetForegroundColor(foregroundColor);
  this->AxisColor = foregroundColor;
}

View::~View()
{
}

void View::Initialize()
{
  this->GetCanvas().Initialize();
}

void View::SaveAs(const std::string& fileName) const
{
  this->GetCanvas().SaveAs(fileName);
}

void View::RenderAnnotations()
{
  for (unsigned int i = 0; i < Annotations.size(); ++i)
    Annotations[i]->Render(this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
}

void View::SetupForWorldSpace(bool viewportClip)
{
  //this->Camera.SetupMatrices();
  this->GetCanvas().SetViewToWorldSpace(this->Camera, viewportClip);
}

void View::SetupForScreenSpace(bool viewportClip)
{
  //this->Camera.SetupMatrices();
  this->GetCanvas().SetViewToScreenSpace(this->Camera, viewportClip);
}
}
} // namespace vtkm::rendering
