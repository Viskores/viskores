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
#include "array/Array1D.h"

// viskores
#include <viskores/rendering/Actor.h>
#include <viskores/rendering/MapperPoint.h>

namespace viskores_device
{

struct Sphere : public Geometry
{
  Sphere(ViskoresDeviceGlobalState* s);

  void commitParameters() override;
  void finalize() override;

  virtual viskores::rendering::Mapper* mapper() const override { return this->m_mapper.get(); }

private:
  void SetupIndexBased();

  helium::ChangeObserverPtr<Array1D> m_index;
  helium::ChangeObserverPtr<Array1D> m_vertexPosition;
  // optional- radius per position
  helium::ChangeObserverPtr<Array1D> m_vertexRadius;

  std::shared_ptr<viskores::rendering::Mapper> m_mapper;
  viskores::cont::ColorTable m_colorTable;

  // TODO: Add these later.
  // std::array<helium::IntrusivePtr<Array1D>, 5> m_vertexAttributes;
  // std::vector<uint32_t> m_attributeIndex;

  float m_globalRadius{ 0.f }; // fallback radius if m_vertexRadius not there.
};

} // namespace viskores_device
