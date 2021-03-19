//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particle_density_ngp_h
#define vtk_m_filter_particle_density_ngp_h

#include <vtkm/filter/ParticleDensityBase.h>

namespace vtkm
{
namespace filter
{
/// \brief Estimate the density of particles using the Nearest Grid Point method
/// This filter treats the CoordinateSystem of a DataSet as positions of particles.
/// The particles are infinitesimal in size with finite mass (or other scalar attributes
/// such as charge). The filter estimates density by imposing a regular grid as
/// specified in the constructor and summing the mass of particles within each cell
/// in the grid.
/// The mass of particles is established by setting the active field (using SetActiveField).
/// Note that the "mass" can actually be another quantity. For example, you could use
/// electrical charge in place of mass to compute the charge density.
/// Once the sum of the mass is computed for each grid cell, the mass is divided by the
/// volume of the cell. Thus, the density will be computed as the units of the mass field
/// per the cubic units of the coordinate system. If you just want a sum of the mass in each
/// cell, turn off the DivideByVolume feature of this filter.
/// In addition, you can also simply count the number of particles in each cell by calling
/// SetComputeNumberDensity(true).
class ParticleDensityNearestGridPoint : public ParticleDensityBase<ParticleDensityNearestGridPoint>
{
public:
  using Superclass = ParticleDensityBase<ParticleDensityNearestGridPoint>;

  ParticleDensityNearestGridPoint(const vtkm::Id3& dimension,
                                  const vtkm::Vec3f& origin,
                                  const vtkm::Vec3f& spacing);

  ParticleDensityNearestGridPoint(const vtkm::Id3& dimension, const vtkm::Bounds& bounds);

  template <typename T, typename StorageType, typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<Policy> policy);
};
}
}

#include <vtkm/filter/ParticleDensityNearestGridPoint.hxx>

#endif //vtk_m_filter_particle_density_ngp_h
