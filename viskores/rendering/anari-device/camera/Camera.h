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

#include "../Object.h"
#include "../ViskoresTypes.h"
#include <viskores/interop/anari/ViskoresANARITypes.h>

#include <viskores/rendering/Camera.h>

namespace viskores_device
{

struct Camera : public Object
{
  Camera(ViskoresDeviceGlobalState* s);
  ~Camera() override;

  static Camera* createInstance(std::string_view type, ViskoresDeviceGlobalState* state);

  virtual void commitParameters() override;
  virtual void finalize() override;

  float4 imageRegion() const
  {
    return float4(this->m_imageRegion[0],
                  this->m_imageRegion[1],
                  this->m_imageRegion[2],
                  this->m_imageRegion[3]);
  }

  virtual viskores::rendering::Camera camera(const viskores::Bounds& bounds) const = 0;

protected:
  // helium calls commitParameters() only for an object whose parameters changed
  // since its last commit, so a camera committed without any parameters set
  // keeps these values. They must match the defaults in commitParameters().
  viskores::Vec3f_32 m_position{ 0.f, 0.f, 0.f };
  viskores::Vec3f_32 m_direction{ 0.f, 0.f, -1.f };
  viskores::Vec3f_32 m_up{ 0.f, 1.f, 0.f };
  viskores::Vec4f_32 m_imageRegion{ 0.f, 0.f, 1.f, 1.f };

  viskores::Float32 imageRegionToViewport(viskores::Float32 value) const;
};

struct UnknownCamera : public Camera
{
  UnknownCamera(ViskoresDeviceGlobalState* s);

  viskores::rendering::Camera camera(const viskores::Bounds&) const override;

  bool isValid() const override;
};

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_SPECIALIZATION(viskores_device::Camera*, ANARI_CAMERA);
