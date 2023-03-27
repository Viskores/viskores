//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/density_estimate/ParticleDensityBase.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace
{
class DivideByVolumeWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldInOut field);
  using ExecutionSignature = void(_1);

  VTKM_EXEC_CONT
  explicit DivideByVolumeWorklet(vtkm::Float64 volume)
    : Volume(volume)
  {
  }

  template <typename T>
  VTKM_EXEC void operator()(T& value) const
  {
    value = static_cast<T>(value / Volume);
  }

private:
  vtkm::Float64 Volume;
}; // class DivideByVolumeWorklet
}

namespace vtkm
{
namespace filter
{
namespace density_estimate
{

VTKM_CONT void ParticleDensityBase::DoDivideByVolume(
  const vtkm::cont::UnknownArrayHandle& density) const
{
  auto volume = this->Spacing[0] * this->Spacing[1] * this->Spacing[2];
  auto resolve = [&](const auto& concreteDensity) {
    this->Invoke(DivideByVolumeWorklet{ volume }, concreteDensity);
  };
  this->CastAndCallScalarField(density, resolve);
}
} // namespace density_estimate
} // namespace filter
} // namespace vtkm
