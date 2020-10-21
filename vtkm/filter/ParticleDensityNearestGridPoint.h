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

#include <vtkm/filter/FilterField.h>

namespace vtkm
{
namespace filter
{
/// \brief Estimate the density of particles using the Nearest Grid Point method

// We only need the CoordinateSystem of the input dataset thus a FilterField
class ParticleDensityNearestGridPoint
  : public vtkm::filter::FilterField<ParticleDensityNearestGridPoint>
{
public:
  // ParticleDensity only support turning 2D/3D particle positions into density
  using SupportedTypes = vtkm::TypeListFieldVec3;

  //
  ParticleDensityNearestGridPoint(const vtkm::Id3& dimension,
                                  const vtkm::Vec3f& origin,
                                  const vtkm::Vec3f& spacing);

  ParticleDensityNearestGridPoint(const vtkm::Id3& dimension, const vtkm::Bounds& bounds);

  template <typename T, typename StorageType, typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<Policy> policy);

private:
  vtkm::Id3 Dimension; // Cell dimension
  vtkm::Vec3f Origin;
  vtkm::Vec3f Spacing;
};
}
}

#include <vtkm/filter/ParticleDensityNearestGridPoint.hxx>

#endif //vtk_m_filter_particle_density_ngp_h
