//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "MatteMaterial.h"

#include <viskores/cont/ArrayHandleConstant.h>

namespace viskores_device
{

MatteMaterial::MatteMaterial(ViskoresDeviceGlobalState* d)
  : Material(d)
  , m_sampler(this)
{
}

void MatteMaterial::commitParameters()
{
  this->m_sampler = this->getParamObject<Sampler>("color");
  this->m_colorAttribute = helium::attributeFromString(this->getParamString("color", "none"));
  this->m_color = this->getParam("color", viskores::Vec3f_32{ 0.8, 0.8, 0.8 });
  this->m_color = viskores::Min(viskores::Max(this->m_color, { 0.f, 0.f, 0.f }), { 1.f, 1.f, 1.f });
  // TODO: Implement sampler

  this->m_opacityAttribute = helium::attributeFromString(this->getParamString("color", "none"));
  this->m_opacity = this->getParam("opacity", viskores::Float32{ 1.0f });
  // TODO: Implement sampler

  this->m_alphaMode = helium::alphaModeFromString(this->getParamString("alphaMode", "opaque"));

  this->m_alphaCutoff = this->getParam("alphaCutoff", viskores::Float32{ 0.5f });
}

void MatteMaterial::finalize()
{
  // no-op
}

std::shared_ptr<viskores::rendering::Actor> MatteMaterial::createActor(
  const viskores::cont::DataSet& data)
{
  std::shared_ptr<viskores::rendering::Actor> actor;
  if (this->m_sampler && this->m_sampler->isValid())
  {
    actor = this->m_sampler->createActor(data);
    if (!actor)
    {
      this->reportMessage(ANARI_SEVERITY_WARNING, "could not create actor for sampler");
    }
  }

  if (!actor)
  {
    viskores::cont::ColorTable colorTable(viskores::ColorSpace::RGB);
    viskores::cont::Field colorField;
    // TODO: Implement sampling and attributes.
    // This should be the fallback when other coloring is missing.
    colorTable.AddPoint(0, this->color());
    colorTable.AddPointAlpha(0, this->opacity());
    colorField = viskores::cont::Field{ "data",
                                        viskores::cont::Field::Association::Points,
                                        viskores::cont::make_ArrayHandleConstant(
                                          viskores::Float32{ 0.0f }, data.GetNumberOfPoints()) };

    actor = std::make_shared<viskores::rendering::Actor>(
      data.GetCellSet(), data.GetCoordinateSystem(), colorField, colorTable);
  }

  return actor;
}

} // namespace viskores_device
