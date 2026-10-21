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

#include <optional>

#include "Camera.h"

namespace viskores_device
{

struct Perspective : public Camera
{
  Perspective(ViskoresDeviceGlobalState* s);
  void commitParameters() override;
  void finalize() override;

  viskores::rendering::Camera camera(const viskores::Bounds& bounds) const override;

private:
  // helium calls commitParameters() only for an object whose parameters changed
  // since its last commit, so a camera committed without any parameters set
  // keeps these values. They must match the defaults in commitParameters().
  viskores::Float32 m_fovy{ viskores::Pi_3f() };
  std::optional<viskores::Float32> m_aspect;
  viskores::Float32 m_near{ -1.f };
  viskores::Float32 m_far{ -1.f };
};

} // namespace viskores_device
