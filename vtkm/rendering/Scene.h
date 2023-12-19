//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_Scene_h
#define vtk_m_rendering_Scene_h

#include <vtkm/rendering/vtkm_rendering_export.h>

#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Mapper.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

/// @brief A simple collection of things to render.
///
/// The `Scene` is a simple collection of `Actor` objects.
class VTKM_RENDERING_EXPORT Scene
{
public:
  Scene();

  /// @brief Add an `Actor` to the scene.
  void AddActor(vtkm::rendering::Actor actor);

  /// @brief Get one of the `Actor`s from the scene.
  const vtkm::rendering::Actor& GetActor(vtkm::IdComponent index) const;

  /// @brief Get the number of `Actor`s in the scene.
  vtkm::IdComponent GetNumberOfActors() const;

  void Render(vtkm::rendering::Mapper& mapper,
              vtkm::rendering::Canvas& canvas,
              const vtkm::rendering::Camera& camera) const;

  /// @brief The computed spatial bounds of combined data from all contained `Actor`s.
  vtkm::Bounds GetSpatialBounds() const;

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_Scene_h
