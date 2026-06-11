//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#pragma once

#include "Geometry.h"

#include <viskores/cont/ArrayHandle.h>

#include <string>

namespace viskores_device
{

struct Cone : Geometry
{
  Cone(ViskoresDeviceGlobalState* s);

  void commitParameters() override;
  void finalize() override;

  bool isValid() const override;

  virtual void render(
    viskores::rendering::Canvas& canvas,
    const viskores::rendering::Camera& camera,
    const viskores::cont::Field& field,
    const viskores::cont::ArrayHandle<viskores::Vec4f_32>& colorMap) const override;

private:
  viskores::UInt8 ParseCaps(const std::string& caps);
  void AddExpandedPrimitiveField(const std::string& fieldName,
                                 const viskores::cont::ArrayHandle<viskores::Id>& primitiveIds,
                                 viskores::Id numberOfCones);
  void AddExpandedVertexField(const std::string& fieldName,
                              const viskores::cont::ArrayHandle<viskores::Id>& vertexIds,
                              viskores::Id numberOfVertices);

  helium::ChangeObserverPtr<Array1D> m_index;
  helium::ChangeObserverPtr<Array1D> m_vertexRadius;
  helium::ChangeObserverPtr<Array1D> m_vertexCaps;
  FieldArrayParameters m_vertexAttributes;

  float m_globalRadius{ 1.f };
  viskores::UInt8 m_globalCapMask{ 0 };
};

} // namespace viskores_device
