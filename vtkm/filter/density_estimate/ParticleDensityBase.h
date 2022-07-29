//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_density_estimate_ParticleDensityBase_h
#define vtk_m_filter_density_estimate_ParticleDensityBase_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/density_estimate/vtkm_filter_density_estimate_export.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT ParticleDensityBase : public vtkm::filter::NewFilterField
{
protected:
  ParticleDensityBase(const vtkm::Id3& dimension,
                      const vtkm::Vec3f& origin,
                      const vtkm::Vec3f& spacing)
    : Dimension(dimension)
    , Origin(origin)
    , Spacing(spacing)
    , ComputeNumberDensity(false)
    , DivideByVolume(true)
  {
  }

  ParticleDensityBase(const vtkm::Id3& dimension, const vtkm::Bounds& bounds)
    : Dimension(dimension)
    , Origin({ static_cast<vtkm::FloatDefault>(bounds.X.Min),
               static_cast<vtkm::FloatDefault>(bounds.Y.Min),
               static_cast<vtkm::FloatDefault>(bounds.Z.Min) })
    , Spacing(vtkm::Vec3f{ static_cast<vtkm::FloatDefault>(bounds.X.Length()),
                           static_cast<vtkm::FloatDefault>(bounds.Y.Length()),
                           static_cast<vtkm::FloatDefault>(bounds.Z.Length()) } /
              Dimension)
    , ComputeNumberDensity(false)
    , DivideByVolume(true)
  {
  }

public:
  VTKM_CONT void SetComputeNumberDensity(bool yes) { this->ComputeNumberDensity = yes; }

  VTKM_CONT bool GetComputeNumberDensity() const { return this->ComputeNumberDensity; }

  VTKM_CONT void SetDivideByVolume(bool yes) { this->DivideByVolume = yes; }

  VTKM_CONT bool GetDivideByVolume() const { return this->DivideByVolume; }

protected:
  // Note: we are using the paradoxical "const ArrayHandle&" parameter whose content can actually
  // be change by the function.
  VTKM_CONT void DoDivideByVolume(const vtkm::cont::UnknownArrayHandle& array) const;

  vtkm::Id3 Dimension; // Cell dimension
  vtkm::Vec3f Origin;
  vtkm::Vec3f Spacing;
  bool ComputeNumberDensity;
  bool DivideByVolume;
};
} // namespace density_estimate
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_density_estimate_ParticleDensityBase_h
