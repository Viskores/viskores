//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Surface.h"

namespace viskores_device
{

Surface::Surface(ViskoresDeviceGlobalState* s)
  : Object(ANARI_SURFACE, s)
  , m_geometry(this)
  , m_material(this)
{
}

Surface::~Surface() = default;

void Surface::commitParameters()
{
  m_id = getParam<uint32_t>("id", ~0u);
  m_geometry = getParamObject<Geometry>("geometry");
  m_material = getParamObject<Material>("material");
}

void Surface::finalize()
{
  if (!this->m_material || !this->m_material->isValid())
  {
    reportMessage(ANARI_SEVERITY_WARNING, "missing 'material' on ANARISurface");
    return;
  }

  if (!this->m_geometry || !this->m_geometry->isValid())
  {
    reportMessage(ANARI_SEVERITY_WARNING, "missing 'geometry' on ANARISurface");
    return;
  }

  this->m_dataSet = this->m_geometry->getDataSet();

  ColorMap colorMap;
  this->m_material->getColors(this->m_dataSet, this->m_field, colorMap);
  this->m_colorMap = colorMap.colors;
  this->m_colorMapSize = colorMap.size;

  if ((colorMap.inFieldTransform(0, 1) != 0) || (colorMap.inFieldTransform(0, 2) != 0) ||
      (colorMap.inFieldTransform(1, 0) != 0) || (colorMap.inFieldTransform(1, 2) != 0))
  {
    reportMessage(ANARI_SEVERITY_WARNING,
                  "inTransform for sampler only supports scaling and translating texture "
                  "coordinates (no rotations)");
  }

  bool inverseValid;
  Mat4f_32 inverseTransform = viskores::MatrixInverse(colorMap.inFieldTransform, inverseValid);

  viskores::IdComponent numTextureDims = this->m_field.GetData().GetNumberOfComponentsFlat();
  if (numTextureDims > 2)
  {
    reportMessage(ANARI_SEVERITY_WARNING,
                  "Only fields of 1 or 2 dimensions supported for texture lookup.");
    numTextureDims = 2;
  }
  this->m_fieldRanges.Allocate(numTextureDims);
  auto rangesPortal = this->m_fieldRanges.WritePortal();

  if (inverseValid)
  {
    auto transformRange = [&](viskores::Float32 x, viskores::IdComponent component)
    {
      viskores::Vec4f_32 v = { 0, 0, 0, 1 };
      v[component] = x;
      viskores::Vec4f_32 transformed =
        viskores::MatrixMultiply(inverseTransform, v) - colorMap.inFieldOffset;
      return transformed[component] / transformed[3];
    };
    for (viskores::IdComponent dim = 0; dim < numTextureDims; ++dim)
    {
      rangesPortal.Set(dim, { transformRange(0, 0), transformRange(1, 0) });
    }
  }
  else
  {
    reportMessage(ANARI_SEVERITY_WARNING, "inTransform for sampler is not invertible");
    for (viskores::IdComponent dim = 0; dim < numTextureDims; ++dim)
    {
      rangesPortal.Set(dim, { 0, 1 });
    }
  }
}

const Geometry* Surface::geometry() const
{
  return m_geometry.get();
}

const Material* Surface::material() const
{
  return m_material.get();
}

void Surface::render(viskores::rendering::Canvas& canvas,
                     const viskores::rendering::Camera& camera) const
{
  this->m_geometry->render(
    canvas, camera, this->m_field, this->m_colorMap, this->m_colorMapSize, this->m_fieldRanges);
}

viskores::Bounds Surface::bounds() const
{
  return this->geometry()->getDataSet().GetCoordinateSystem().GetBounds();
}

bool Surface::isValid() const
{
  return m_geometry && m_material && m_geometry->isValid() && m_material->isValid();
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Surface*);
