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

#include <vtkm/filter/FilterField.h>

namespace vtkm
{
namespace filter
{
/// \brief Estimate the density of particles using the Cloud-in-Cell method

// We only need the CoordinateSystem of the input dataset thus a FilterField
class ParticleDensityCloudInCell : public vtkm::filter::FilterField<ParticleDensityCloudInCell>
{
public:
  // ParticleDensity only support turning 2D/3D particle positions into density
  // FIXME: 2D?
  //using SupportedTypes = vtkm::TypeListFieldVec3;
  using SupportedTypes = vtkm::TypeListFieldScalar;

  ParticleDensityCloudInCell(const vtkm::Id3& dimension,
                             const vtkm::Vec3f& origin,
                             const vtkm::Vec3f& spacing);

  template <typename T, typename StorageType, typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<Policy> policy);

private:
  vtkm::Id3 Dimension; // Point dimension
  vtkm::Vec3f Origin;
  vtkm::Vec3f Spacing;
};
} // filter
} // vtkm

#include <vtkm/filter/ParticleDensityCloudInCell.hxx>
#endif // vtk_m_filter_particle_density_cic_h
