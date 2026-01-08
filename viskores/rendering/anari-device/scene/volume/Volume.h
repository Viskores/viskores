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

#include "Object.h"
#include "ViskoresTypes.h"
// Viskores
#include <viskores/Bounds.h>
#include <viskores/cont/DataSet.h>
#include <viskores/rendering/Actor.h>
#include <viskores/rendering/Mapper.h>

namespace viskores_device
{

struct Volume : public Object
{
  Volume(ViskoresDeviceGlobalState* d);
  virtual ~Volume();
  static Volume* createInstance(std::string_view subtype, ViskoresDeviceGlobalState* d);

  void commitParameters() override;
  void finalize() override;

  uint32_t id() const;

  virtual viskores::Bounds bounds() const = 0;
  virtual viskores::rendering::Actor* actor() const = 0;
  virtual viskores::rendering::Mapper* mapper() const = 0;

private:
  uint32_t m_id{ ~0u };
};

struct UnknownVolume : public Volume
{
  UnknownVolume(ViskoresDeviceGlobalState* d);

  viskores::Bounds bounds() const override;
  viskores::rendering::Actor* actor() const override;
  viskores::rendering::Mapper* mapper() const override;
  bool isValid() const override;
};

// Inlined definitions ////////////////////////////////////////////////////////

inline uint32_t Volume::id() const
{
  return m_id;
}

} // namespace viskores_device

VISKORES_ANARI_TYPEFOR_SPECIALIZATION(viskores_device::Volume*, ANARI_VOLUME);
