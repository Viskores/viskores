//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_density_estimate_ParticleDensityCIC_h
#define vtk_m_filter_density_estimate_ParticleDensityCIC_h

#include <vtkm/filter/density_estimate/ParticleDensityBase.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
/// \brief Estimate the density of particles using the Cloud-in-Cell method.
///
/// This filter takes a collection of particles.
/// The particles are infinitesimal in size with finite mass (or other scalar attributes
/// such as charge). The filter estimates density by imposing a regular grid (as
/// specified by `SetDimensions`, `SetOrigin`, and `SetSpacing`) and summing the mass
/// of particles within each cell in the grid.
/// The particle's mass is divided among the 8 nearest neighboring bins. This differs from
/// `ParticleDensityNearestGridPoint`, which just finds the nearest containing bin.
///
/// The mass of particles is established by setting the active field (using `SetActiveField`).
/// Note that the "mass" can actually be another quantity. For example, you could use
/// electrical charge in place of mass to compute the charge density.
/// Once the sum of the mass is computed for each grid cell, the mass is divided by the
/// volume of the cell. Thus, the density will be computed as the units of the mass field
/// per the cubic units of the coordinate system. If you just want a sum of the mass in each
/// cell, turn off the `DivideByVolume` feature of this filter.
/// In addition, you can also simply count the number of particles in each cell by calling
/// `SetComputeNumberDensity(true)`.
///
/// This operation is helpful in the analysis of particle-based simulation where the data
/// often requires conversion or deposition of particles' attributes, such as mass, to an
/// overlaying mesh. This allows further identification of regions of interest based on the
/// spatial distribution of particles attributes, for example, high density regions could be
/// considered as clusters or halos while low density regions could be considered as bubbles
/// or cavities in the particle data.
///
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT ParticleDensityCloudInCell : public ParticleDensityBase
{
public:
  using Superclass = ParticleDensityBase;

  ParticleDensityCloudInCell() = default;

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace density_estimate
} // namespace filter
} // namespace vtkm

#endif // vtk_m_filter_density_estimate_ParticleDensityCIC_h
