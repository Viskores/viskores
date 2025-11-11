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

#include "Volume.h"
#include "array/Array1D.h"
#include "spatial_field/SpatialField.h"
// helium
#include <helium/utility/ChangeObserverPtr.h>
// Viskores
#include <viskores/Range.h>
#include <viskores/cont/ColorTable.h>
#include <viskores/rendering/MapperVolume.h>

namespace viskores_device
{

struct TransferFunction1D : public Volume
{
  TransferFunction1D(ViskoresDeviceGlobalState* d);

  void commitParameters() override;
  void finalize() override;

  const SpatialField* spatialField() const;
  viskores::Bounds bounds() const override;
  viskores::rendering::Actor* actor() const override;
  viskores::rendering::MapperVolume* mapper() const override;

  bool isValid() const override;

private:
  helium::ChangeObserverPtr<SpatialField> m_spatialField;
  viskores::Range m_valueRange;
  helium::ChangeObserverPtr<Array1D> m_colorArray;
  helium::ChangeObserverPtr<Array1D> m_opacityArray;
  float4 m_color;
  viskores::Float32 m_alpha;
  viskores::Float32 m_unitDistance;
  viskores::cont::ColorTable m_colorTable;

  std::shared_ptr<viskores::rendering::Actor> m_actor;
  std::shared_ptr<viskores::rendering::MapperVolume> m_mapper;
};

} // namespace viskores_device
