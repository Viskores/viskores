//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_rendering_Scene_h
#define viskores_rendering_Scene_h

#include <viskores/rendering/viskores_rendering_export.h>

#include <viskores/rendering/Actor.h>
#include <viskores/rendering/Camera.h>
#include <viskores/rendering/Canvas.h>
#include <viskores/rendering/Mapper.h>

#include <memory>

namespace viskores
{
namespace rendering
{

/// @brief A simple collection of things to render.
///
/// The `Scene` is a simple collection of `Actor` objects.
class VISKORES_RENDERING_EXPORT Scene
{
public:
  Scene();

  /// @brief Add an `Actor` to the scene.
  void AddActor(viskores::rendering::Actor actor);

  /// @brief Get one of the `Actor`s from the scene.
  const viskores::rendering::Actor& GetActor(viskores::IdComponent index) const;

  /// @brief Get the number of `Actor`s in the scene.
  viskores::IdComponent GetNumberOfActors() const;

  void Render(viskores::rendering::Mapper& mapper,
              viskores::rendering::Canvas& canvas,
              const viskores::rendering::Camera& camera) const;

  /// @brief The computed spatial bounds of combined data from all contained `Actor`s.
  viskores::Bounds GetSpatialBounds() const;

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
};
}
} //namespace viskores::rendering

#endif //viskores_rendering_Scene_h
