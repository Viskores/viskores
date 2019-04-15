//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/View.h>

namespace vtkm
{
namespace rendering
{

struct View::InternalData
{
  ~InternalData()
  {
    delete MapperPointer;
    delete CanvasPointer;
    delete WorldAnnotatorPointer;
  }
  vtkm::rendering::Scene Scene;
  vtkm::rendering::Mapper* MapperPointer{ nullptr };
  vtkm::rendering::Canvas* CanvasPointer{ nullptr };
  vtkm::rendering::WorldAnnotator* WorldAnnotatorPointer{ nullptr };
  std::vector<std::unique_ptr<vtkm::rendering::TextAnnotation>> Annotations;
  vtkm::rendering::Camera Camera;
};

View::View(const vtkm::rendering::Scene& scene,
           const vtkm::rendering::Mapper& mapper,
           const vtkm::rendering::Canvas& canvas,
           const vtkm::rendering::Color& backgroundColor,
           const vtkm::rendering::Color& foregroundColor)
  : Internal(std::make_shared<InternalData>())
{
  this->Internal->Scene = scene;
  this->Internal->MapperPointer = mapper.NewCopy();
  this->Internal->CanvasPointer = canvas.NewCopy();
  this->Internal->WorldAnnotatorPointer = canvas.CreateWorldAnnotator();
  this->Internal->CanvasPointer->SetBackgroundColor(backgroundColor);
  this->Internal->CanvasPointer->SetForegroundColor(foregroundColor);
  this->AxisColor = foregroundColor;

  vtkm::Bounds spatialBounds = this->Internal->Scene.GetSpatialBounds();
  this->Internal->Camera.ResetToBounds(spatialBounds);
  if (spatialBounds.Z.Length() > 0.0)
  {
    this->Internal->Camera.SetModeTo3D();
  }
  else
  {
    this->Internal->Camera.SetModeTo2D();
  }
}

View::View(const vtkm::rendering::Scene& scene,
           const vtkm::rendering::Mapper& mapper,
           const vtkm::rendering::Canvas& canvas,
           const vtkm::rendering::Camera& camera,
           const vtkm::rendering::Color& backgroundColor,
           const vtkm::rendering::Color& foregroundColor)
  : Internal(std::make_shared<InternalData>())
{
  this->Internal->Scene = scene;
  this->Internal->MapperPointer = mapper.NewCopy();
  this->Internal->CanvasPointer = canvas.NewCopy();
  this->Internal->WorldAnnotatorPointer = canvas.CreateWorldAnnotator();
  this->Internal->Camera = camera;
  this->Internal->CanvasPointer->SetBackgroundColor(backgroundColor);
  this->Internal->CanvasPointer->SetForegroundColor(foregroundColor);
  this->AxisColor = foregroundColor;
}

View::~View()
{
}

const vtkm::rendering::Scene& View::GetScene() const
{
  return this->Internal->Scene;
}

vtkm::rendering::Scene& View::GetScene()
{
  return this->Internal->Scene;
}

void View::SetScene(const vtkm::rendering::Scene& scene)
{
  this->Internal->Scene = scene;
}

const vtkm::rendering::Mapper& View::GetMapper() const
{
  return *this->Internal->MapperPointer;
}

vtkm::rendering::Mapper& View::GetMapper()
{
  return *this->Internal->MapperPointer;
}

const vtkm::rendering::Canvas& View::GetCanvas() const
{
  return *this->Internal->CanvasPointer;
}

vtkm::rendering::Canvas& View::GetCanvas()
{
  return *this->Internal->CanvasPointer;
}

const vtkm::rendering::WorldAnnotator& View::GetWorldAnnotator() const
{
  return *this->Internal->WorldAnnotatorPointer;
}

const vtkm::rendering::Camera& View::GetCamera() const
{
  return this->Internal->Camera;
}

vtkm::rendering::Camera& View::GetCamera()
{
  return this->Internal->Camera;
}

void View::SetCamera(const vtkm::rendering::Camera& camera)
{
  this->Internal->Camera = camera;
}

const vtkm::rendering::Color& View::GetBackgroundColor() const
{
  return this->Internal->CanvasPointer->GetBackgroundColor();
}

void View::SetBackgroundColor(const vtkm::rendering::Color& color)
{
  this->Internal->CanvasPointer->SetBackgroundColor(color);
}

void View::SetForegroundColor(const vtkm::rendering::Color& color)
{
  this->Internal->CanvasPointer->SetForegroundColor(color);
}

void View::Initialize()
{
  this->GetCanvas().Initialize();
}

void View::SaveAs(const std::string& fileName) const
{
  this->GetCanvas().SaveAs(fileName);
}

void View::SetAxisColor(vtkm::rendering::Color c)
{
  this->AxisColor = c;
}

void View::ClearAnnotations()
{
  this->Internal->Annotations.clear();
}

void View::AddAnnotation(std::unique_ptr<vtkm::rendering::TextAnnotation> ann)
{
  this->Internal->Annotations.push_back(std::move(ann));
}

void View::RenderAnnotations()
{
  for (unsigned int i = 0; i < this->Internal->Annotations.size(); ++i)
    this->Internal->Annotations[i]->Render(
      this->GetCamera(), this->GetWorldAnnotator(), this->GetCanvas());
}

void View::SetupForWorldSpace(bool viewportClip)
{
  //this->Camera.SetupMatrices();
  this->GetCanvas().SetViewToWorldSpace(this->Internal->Camera, viewportClip);
}

void View::SetupForScreenSpace(bool viewportClip)
{
  //this->Camera.SetupMatrices();
  this->GetCanvas().SetViewToScreenSpace(this->Internal->Camera, viewportClip);
}
}
} // namespace vtkm::rendering
