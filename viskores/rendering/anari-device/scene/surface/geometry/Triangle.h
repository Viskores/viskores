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
// viskores
#include <viskores/rendering/Actor.h>

namespace viskores_device
{

struct Triangle : Geometry
{
  Triangle(ViskoresDeviceGlobalState* s);

  void commitParameters() override;
  void finalize() override;

  virtual viskores::rendering::Mapper* mapper() const override { return this->m_mapper.get(); }

  bool isValid() const override;

private:
  helium::ChangeObserverPtr<Array1D> m_index;
  FieldArrayParameters m_vertexAttributes;
  FieldArrayParameters m_faceVaryingAttributes;

  std::shared_ptr<viskores::rendering::MapperRayTracer> m_mapper;

  viskores::cont::ColorTable m_colorTable;
  helium::ChangeObserverPtr<Array1D> m_vertexColor;
};

} // namespace viskores_device
