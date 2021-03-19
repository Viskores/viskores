//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particle_density_cic_h
#define vtk_m_filter_particle_density_cic_h

#include <vtkm/filter/ParticleDensityBase.h>

namespace vtkm
{
namespace filter
{
/// \brief Estimate the density of particles using the Cloud-in-Cell method
/// This filter treats the CoordinateSystem of a DataSet as positions of particles.
/// The particles are infinitesimal in size with finite mass (or other scalar attributes
/// such as charge). The filter estimates density by imposing a regular grid as
/// specified in the constructor. It spreads the mass of each particle to its 8 nearest
/// neighboring grid points and summing the contribution of particles for each point
/// in the grid.
/// The mass of particles is established by setting the active field (using SetActiveField).
/// Note that the "mass" can actually be another quantity. For example, you could use
/// electrical charge in place of mass to compute the charge density.
/// Once the sum of the mass is computed for each grid point, the mass is divided by the
/// volume of the cell. Thus, the density will be computed as the units of the mass field
/// per the cubic units of the coordinate system. If you just want a sum of the mass in each
/// cell, turn off the DivideByVolume feature of this filter.
/// In addition, you can also simply count the number of particles in each cell by calling
/// SetComputeNumberDensity(true).
class ParticleDensityCloudInCell : public ParticleDensityBase<ParticleDensityCloudInCell>
{
public:
  using Superclass = ParticleDensityBase<ParticleDensityCloudInCell>;

  ParticleDensityCloudInCell(const vtkm::Id3& dimension,
                             const vtkm::Vec3f& origin,
                             const vtkm::Vec3f& spacing);

  ParticleDensityCloudInCell(const Id3& dimension, const vtkm::Bounds& bounds);

  template <typename T, typename StorageType, typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<Policy> policy);
};
} // filter
} // vtkm

#include <vtkm/filter/ParticleDensityCloudInCell.hxx>
#endif // vtk_m_filter_particle_density_cic_h
