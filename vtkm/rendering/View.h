//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_View_h
#define vtk_m_rendering_View_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Color.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/TextAnnotation.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT View
{
public:
  View(const vtkm::rendering::Scene& scene,
       const vtkm::rendering::Mapper& mapper,
       const vtkm::rendering::Canvas& canvas,
       const vtkm::rendering::Color& backgroundColor = vtkm::rendering::Color(0, 0, 0, 1),
       const vtkm::rendering::Color& foregroundColor = vtkm::rendering::Color(1, 1, 1, 1));

  View(const vtkm::rendering::Scene& scene,
       const vtkm::rendering::Mapper& mapper,
       const vtkm::rendering::Canvas& canvas,
       const vtkm::rendering::Camera& camera,
       const vtkm::rendering::Color& backgroundColor = vtkm::rendering::Color(0, 0, 0, 1),
       const vtkm::rendering::Color& foregroundColor = vtkm::rendering::Color(1, 1, 1, 1));

  virtual ~View();

  VTKM_CONT
  const vtkm::rendering::Scene& GetScene() const { return this->Scene; }
  VTKM_CONT
  vtkm::rendering::Scene& GetScene() { return this->Scene; }
  VTKM_CONT
  void SetScene(const vtkm::rendering::Scene& scene) { this->Scene = scene; }

  VTKM_CONT
  const vtkm::rendering::Mapper& GetMapper() const { return *this->MapperPointer; }
  VTKM_CONT
  vtkm::rendering::Mapper& GetMapper() { return *this->MapperPointer; }

  VTKM_CONT
  const vtkm::rendering::Canvas& GetCanvas() const { return *this->CanvasPointer; }
  VTKM_CONT
  vtkm::rendering::Canvas& GetCanvas() { return *this->CanvasPointer; }

  VTKM_CONT
  const vtkm::rendering::WorldAnnotator& GetWorldAnnotator() const
  {
    return *this->WorldAnnotatorPointer;
  }

  VTKM_CONT
  const vtkm::rendering::Camera& GetCamera() const { return this->Camera; }
  VTKM_CONT
  vtkm::rendering::Camera& GetCamera() { return this->Camera; }
  VTKM_CONT
  void SetCamera(const vtkm::rendering::Camera& camera) { this->Camera = camera; }

  VTKM_CONT
  const vtkm::rendering::Color& GetBackgroundColor() const
  {
    return this->CanvasPointer->GetBackgroundColor();
  }

  VTKM_CONT
  void SetBackgroundColor(const vtkm::rendering::Color& color)
  {
    this->CanvasPointer->SetBackgroundColor(color);
  }

  VTKM_CONT
  void SetForegroundColor(const vtkm::rendering::Color& color)
  {
    this->CanvasPointer->SetForegroundColor(color);
  }

  virtual void Initialize();

  virtual void Paint() = 0;
  virtual void RenderScreenAnnotations() = 0;
  virtual void RenderWorldAnnotations() = 0;

  void SaveAs(const std::string& fileName) const;

  VTKM_CONT
  void SetAxisColor(vtkm::rendering::Color c) { this->AxisColor = c; }

  VTKM_CONT
  void ClearAnnotations() { Annotations.clear(); }

  VTKM_CONT
  void AddAnnotation(vtkm::rendering::TextAnnotation* ann) { Annotations.push_back(ann); }

protected:
  void SetupForWorldSpace(bool viewportClip = true);

  void SetupForScreenSpace(bool viewportClip = false);

  void RenderAnnotations();

  vtkm::rendering::Color AxisColor = vtkm::rendering::Color::white;

private:
  vtkm::rendering::Scene Scene;
  std::shared_ptr<vtkm::rendering::Mapper> MapperPointer;
  std::shared_ptr<vtkm::rendering::Canvas> CanvasPointer;
  std::shared_ptr<vtkm::rendering::WorldAnnotator> WorldAnnotatorPointer;
  std::vector<vtkm::rendering::TextAnnotation*> Annotations;
  vtkm::rendering::Camera Camera;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_View_h
