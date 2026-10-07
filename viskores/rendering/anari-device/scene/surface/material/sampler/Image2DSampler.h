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

#include "Sampler.h"

#include "array/Array2D.h"
// helium
#include <helium/utility/ChangeObserverPtr.h>

namespace viskores_device
{

struct Image2DSampler : public Sampler
{
  Image2DSampler(ViskoresDeviceGlobalState* d);

  void commitParameters() override;
  void finalize() override;

  const Mat4f_32& inTransform() const { return this->m_inTransform; }
  const viskores::Vec4f_32& inOffset() const { return this->m_inOffset; }

  const std::string& inAttribute() const { return this->m_inAttribute; }

  helium::WrapMode wrapMode() const { return this->m_wrapMode; }

  bool getColors(const viskores::cont::DataSet& data,
                 viskores::cont::Field& field,
                 ColorMap& colorMap) const override;

private:
  Mat4f_32 m_inTransform;
  viskores::Vec4f_32 m_inOffset;
  std::string m_inAttribute;
  helium::ChangeObserverPtr<Array2D> m_colorArray;
  helium::WrapMode m_wrapMode;

  viskores::cont::ArrayHandle<viskores::Vec4f_32> m_colorMap;
};

} // namespace viskores_device
