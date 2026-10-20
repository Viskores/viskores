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
// Viskores
#include <viskores/Math.h>
#include <viskores/Range.h>

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

  viskores::IdComponent numTextureDims = this->m_field.GetData().GetNumberOfComponentsFlat();
  if (numTextureDims > 2)
  {
    reportMessage(ANARI_SEVERITY_WARNING,
                  "Only fields of 1 or 2 dimensions supported for texture lookup.");
    numTextureDims = 2;
  }
  this->m_fieldRanges.Allocate(numTextureDims);
  auto rangesPortal = this->m_fieldRanges.WritePortal();

  // The sampler maps a field value to the texture coordinate
  // inTransform * value + inOffset. The field range is the values that map to
  // texture coordinates 0 and 1. As inTransform only scales and translates,
  // invert it per axis in closed form, in double precision. (Inverting the
  // Float32 matrix rejects scales below Float32 epsilon, so it fails for
  // value ranges wider than about 8e6.)
  bool inverseValid = true;
  for (viskores::IdComponent dim = 0; dim < numTextureDims; ++dim)
  {
    const viskores::Float64 scale = colorMap.inFieldTransform(dim, dim);
    const viskores::Float64 translate =
      static_cast<viskores::Float64>(colorMap.inFieldTransform(dim, 3)) +
      static_cast<viskores::Float64>(colorMap.inFieldOffset[dim]);
    const viskores::Range range{ (0. - translate) / scale, (1. - translate) / scale };
    if ((scale != 0) && viskores::IsFinite(scale) && viskores::IsFinite(range.Min) &&
        viskores::IsFinite(range.Max))
    {
      rangesPortal.Set(dim, range);
    }
    else
    {
      inverseValid = false;
      rangesPortal.Set(dim, { 0, 1 });
    }
  }
  if (!inverseValid)
  {
    reportMessage(ANARI_SEVERITY_WARNING, "inTransform for sampler is not invertible");
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
  return this->geometry()->bounds();
}

bool Surface::isValid() const
{
  return m_geometry && m_material && m_geometry->isValid() && m_material->isValid();
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_DEFINITION(viskores_device::Surface*);
