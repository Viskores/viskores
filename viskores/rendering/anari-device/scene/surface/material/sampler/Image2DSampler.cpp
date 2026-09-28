//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#include "Image2DSampler.h"
#include "array/ArrayConversion.h"
// Viskores
#include <viskores/TypeTraits.h>
#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/ArrayExtractComponent.h>
#include <viskores/cont/ArrayHandleConstant.h>
// std
#include <limits>
namespace viskores_device
{

Image2DSampler::Image2DSampler(ViskoresDeviceGlobalState* d)
  : Sampler(d)
  , m_colorArray(this)
{
}

void Image2DSampler::commitParameters()
{
  this->Sampler::commitParameters();

  this->m_inAttribute = this->getParamString("inAttribute", "attribute0");

  mat4 inTransform = this->getParam("inTransform", mat4(linalg::identity));
  this->m_inTransform = toViskoresMatrix(inTransform);
  anari::math::float4 inOffset =
    this->getParam("inOffset", anari::math::float4(0.f, 0.f, 0.f, 0.f));
  this->m_inOffset = { inOffset[0], inOffset[1], inOffset[2], inOffset[3] };

  this->m_colorArray = this->getParamObject<Array2D>("image");

  this->m_wrapMode = helium::wrapModeFromString(this->getParamString("wrapMode", "clampToEdge"));
  if (this->m_wrapMode == helium::WrapMode::DEFAULT)
  {
    this->m_wrapMode = helium::WrapMode::CLAMP_TO_EDGE;
  }
}

void Image2DSampler::finalize()
{
  this->Sampler::finalize();

  if (this->m_colorArray)
  {
    this->m_colorMap = ANARIColorsToViskoresColors(*this->m_colorArray.get());
  }
  else
  {
    this->reportMessage(ANARI_SEVERITY_WARNING,
                        "image2D sampling requested, but no color array given");
    this->m_colorMap.Allocate(1);
    this->m_colorMap.WritePortal().Set(0, { 1, 1, 1, 1 });
  }
}

bool Image2DSampler::getColors(const viskores::cont::DataSet& data,
                               viskores::cont::Field& field,
                               ColorMap& colorMap) const
{
  if (!data.HasField(this->inAttribute()))
  {
    this->reportMessage(
      ANARI_SEVERITY_WARNING, "sampler attribute %s not found", this->inAttribute().c_str());
    return false;
  }

  viskores::cont::Field attribField = data.GetField(this->inAttribute());
  viskores::cont::UnknownArrayHandle attribArray = attribField.GetData();
  if (!attribArray.CanConvert<viskores::cont::ArrayHandle<viskores::Vec2f_32>>())
  {
    if (!attribArray.IsBaseComponentType<viskores::Float32>())
    {
      this->reportMessage(ANARI_SEVERITY_WARNING,
                          "attribute array type not currently supported for image2D sampler.");
      return false;
    }
    this->reportMessage(ANARI_SEVERITY_PERFORMANCE_WARNING,
                        "todo: handle vector attributes more efficiently");
    viskores::cont::UnknownArrayHandle newArray;
    viskores::cont::ArrayHandleRecombineVec<viskores::Float32> recombinedArray;
    recombinedArray.AppendComponentArray(attribArray.ExtractComponent<viskores::Float32>(0));
    recombinedArray.AppendComponentArray(attribArray.ExtractComponent<viskores::Float32>(1));
    viskores::cont::ArrayCopy(recombinedArray, newArray);
    attribArray = newArray;
  }

  field = viskores::cont::Field{ attribField.GetName(), attribField.GetAssociation(), attribArray };
  colorMap.colors = this->m_colorMap;
  colorMap.size = { viskores::IdComponent(this->m_colorArray->size(0)),
                    viskores::IdComponent(this->m_colorArray->size(1)) };
  colorMap.inFieldTransform = this->m_inTransform;
  colorMap.inFieldOffset = this->m_inOffset;
  return true;
}

} // namespace viskores_device
