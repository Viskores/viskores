//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_density_estimate_ParticleDensityNGP_h
#define vtk_m_filter_density_estimate_ParticleDensityNGP_h

#include <vtkm/filter/density_estimate/ParticleDensityBase.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
/// \brief Estimate the density of particles using the Nearest Grid Point method.
///
/// This filter takes a collection of particles.
/// The particles are infinitesimal in size with finite mass (or other scalar attributes
/// such as charge). The filter estimates density by imposing a regular grid (as
/// specified by `SetDimensions`, `SetOrigin`, and `SetSpacing`) and summing the mass
/// of particles within each cell in the grid.
/// Each input particle is assigned to one bin that it falls in.
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
/// Since there is no specific `vtkm::cont::CellSet` for particles in VTK-m, this filter
/// treats the `vtkm::cont::CoordinateSystem` of the `vtkm::cont::DataSet` as the positions
/// of the particles while ignoring the details of the `vtkm::cont::CellSet`.
///
class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT ParticleDensityNearestGridPoint
  : public ParticleDensityBase
{
public:
  using Superclass = ParticleDensityBase;

  ParticleDensityNearestGridPoint() = default;

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace density_estimate
} // namespace filter
} // namespace vtkm
#endif //vtk_m_filter_density_estimate_ParticleDensityNGP_h
