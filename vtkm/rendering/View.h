//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
  struct InternalData;

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
  const vtkm::rendering::Scene& GetScene() const;
  VTKM_CONT
  vtkm::rendering::Scene& GetScene();
  VTKM_CONT
  void SetScene(const vtkm::rendering::Scene& scene);

  VTKM_CONT
  const vtkm::rendering::Mapper& GetMapper() const;
  VTKM_CONT
  vtkm::rendering::Mapper& GetMapper();

  VTKM_CONT
  const vtkm::rendering::Canvas& GetCanvas() const;
  VTKM_CONT
  vtkm::rendering::Canvas& GetCanvas();

  VTKM_CONT
  const vtkm::rendering::WorldAnnotator& GetWorldAnnotator() const;

  VTKM_CONT
  const vtkm::rendering::Camera& GetCamera() const;
  VTKM_CONT
  vtkm::rendering::Camera& GetCamera();
  VTKM_CONT
  void SetCamera(const vtkm::rendering::Camera& camera);

  VTKM_CONT
  const vtkm::rendering::Color& GetBackgroundColor() const;

  VTKM_CONT
  void SetBackgroundColor(const vtkm::rendering::Color& color);

  VTKM_CONT
  void SetForegroundColor(const vtkm::rendering::Color& color);

  virtual void Initialize();

  virtual void Paint() = 0;
  virtual void RenderScreenAnnotations() = 0;
  virtual void RenderWorldAnnotations() = 0;

  void SaveAs(const std::string& fileName) const;

  VTKM_CONT
  void SetAxisColor(vtkm::rendering::Color c);

  VTKM_CONT
  void ClearAnnotations();

  VTKM_CONT
  void AddAnnotation(std::unique_ptr<vtkm::rendering::TextAnnotation> ann);

protected:
  void SetupForWorldSpace(bool viewportClip = true);

  void SetupForScreenSpace(bool viewportClip = false);

  void RenderAnnotations();

  vtkm::rendering::Color AxisColor = vtkm::rendering::Color::white;

private:
  std::shared_ptr<InternalData> Internal;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_View_h
