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

#include "array/Array1D.h"
// helium
#include <helium/utility/ChangeObserverPtr.h>

namespace viskores_device
{

struct Image1DSampler : public Sampler
{
  Image1DSampler(ViskoresDeviceGlobalState* d);

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
  // helium calls commitParameters() only for an object whose parameters changed
  // since its last commit, so a sampler committed without any parameters set
  // keeps these values. They must match the defaults in commitParameters().
  Mat4f_32 m_inTransform{ viskores::MatrixIdentity<viskores::Float32, 4>() };
  viskores::Vec4f_32 m_inOffset{ 0.f, 0.f, 0.f, 0.f };
  helium::WrapMode m_wrapMode{ helium::WrapMode::CLAMP_TO_EDGE };

  // Empty until commitParameters() sets it, so a sampler without parameters
  // matches no field and the material falls back to its own color.
  std::string m_inAttribute;
  helium::ChangeObserverPtr<Array1D> m_colorArray;

  viskores::cont::ArrayHandle<viskores::Vec4f_32> m_colorMap;
};

} // namespace viskores_device
