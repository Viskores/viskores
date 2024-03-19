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

#include <functional>
#include <memory>

namespace vtkm
{
namespace rendering
{

/// @brief The abstract class representing the view of a rendering scene.
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

  /// @brief Specify the scene object holding the objects to render.
  VTKM_CONT
  const vtkm::rendering::Scene& GetScene() const;
  /// @copydoc GetScene
  VTKM_CONT
  vtkm::rendering::Scene& GetScene();
  /// @copydoc GetScene
  VTKM_CONT
  void SetScene(const vtkm::rendering::Scene& scene);

  /// @brief Specify the mapper object determining how objects are rendered.
  VTKM_CONT
  const vtkm::rendering::Mapper& GetMapper() const;
  /// @copydoc GetMapper
  VTKM_CONT
  vtkm::rendering::Mapper& GetMapper();

  /// @brief Specify the canvas object that holds the buffer to render into.
  VTKM_CONT
  const vtkm::rendering::Canvas& GetCanvas() const;
  /// @copydoc GetCanvas
  VTKM_CONT
  vtkm::rendering::Canvas& GetCanvas();

  VTKM_CONT
  const vtkm::rendering::WorldAnnotator& GetWorldAnnotator() const;

  /// @brief Specify the perspective from which to render a scene.
  VTKM_CONT
  const vtkm::rendering::Camera& GetCamera() const;
  /// @copydoc GetCamera
  VTKM_CONT
  vtkm::rendering::Camera& GetCamera();
  /// @copydoc GetCamera
  VTKM_CONT
  void SetCamera(const vtkm::rendering::Camera& camera);

  /// @brief Specify the color used where nothing is rendered.
  VTKM_CONT
  const vtkm::rendering::Color& GetBackgroundColor() const;
  /// @copydoc GetBackgroundColor
  VTKM_CONT
  void SetBackgroundColor(const vtkm::rendering::Color& color);

  /// @brief Specify the color of foreground elements.
  ///
  /// The foreground is typically used for annotation elements.
  /// The foreground should contrast well with the background.
  VTKM_CONT
  void SetForegroundColor(const vtkm::rendering::Color& color);

  VTKM_CONT
  bool GetWorldAnnotationsEnabled() const { return this->WorldAnnotationsEnabled; }

  VTKM_CONT
  void SetWorldAnnotationsEnabled(bool val) { this->WorldAnnotationsEnabled = val; }

  VTKM_CONT void SetRenderAnnotationsEnabled(bool val) { this->RenderAnnotationsEnabled = val; }
  VTKM_CONT bool GetRenderAnnotationsEnabled() const { return this->RenderAnnotationsEnabled; }

  /// @brief Render a scene and store the result in the canvas' buffers.
  virtual void Paint() = 0;
  virtual void RenderScreenAnnotations() = 0;
  virtual void RenderWorldAnnotations() = 0;

  void RenderAnnotations();

  /// @copydoc vtkm::rendering::Canvas::SaveAs
  void SaveAs(const std::string& fileName) const;

  VTKM_CONT
  void SetAxisColor(vtkm::rendering::Color c);

  VTKM_CONT
  void ClearTextAnnotations();

  VTKM_CONT
  void AddTextAnnotation(std::unique_ptr<vtkm::rendering::TextAnnotation> ann);

  VTKM_CONT
  void ClearAdditionalAnnotations();

  VTKM_CONT
  void AddAdditionalAnnotation(std::function<void(void)> ann);

protected:
  void SetupForWorldSpace(bool viewportClip = true);

  void SetupForScreenSpace(bool viewportClip = false);


  vtkm::rendering::Color AxisColor = vtkm::rendering::Color::white;
  bool WorldAnnotationsEnabled = true;
  bool RenderAnnotationsEnabled = true;

private:
  std::unique_ptr<InternalData> Internal;
};

} // namespace vtkm::rendering
} // namespace vtkm

#endif //vtk_m_rendering_View_h
