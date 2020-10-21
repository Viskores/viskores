//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particle_density_cic_hxx
#define vtk_m_filter_particle_density_cic_hxx

#include "ParticleDensityCloudInCell.h"
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/filter/PolicyBase.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{
class CICWorklet : public vtkm::worklet::WorkletVisitCellsWithPoints
{
public:
  using ControlSignature = void(FieldIn coords, ExecObject locator, AtomicArrayInOut density);
  using ExecutionSignature = void(_1, _2, _3, PointIndices);
};
} // worklet
} // vtkm

namespace vtkm
{
namespace filter
{
inline VTKM_CONT ParticleDensityCloudInCell::ParticleDensityCloudInCell(const vtkm::Id3& dimension,
                                                                        const vtkm::Vec3f& origin,
                                                                        const vtkm::Vec3f& spacing)
  : Dimension(dimension)
  , Origin(origin)
  , Spacing(spacing)
{
}
template <typename T, typename StorageType, typename Policy>
vtkm::cont::DataSet ParticleDensityCloudInCell::DoExecute(
  const cont::DataSet& input,
  const cont::ArrayHandle<T, StorageType>& field,
  const FieldMetadata& fieldMeta,
  PolicyBase<Policy> policy)
{
  // Unlike ParticleDensityNGP, particle deposit mass on the grid points, thus it is natural to
  // return the density as PointField;
  auto uniform = vtkm::cont::DataSetBuilderUniform(this->Dimension, this->Origin, this->Spacing);

  vtkm::cont::CellLocatorUniformGrid locator;
  locator.SetCellSet(uniform.GetCellSet());
  locator.SetCoordinates(uniform.GetCoordinateSystem());
  locator.Update();

  // FIXME: change this to vtkm::DefaultFloat once floating point atomics is there.
  vtkm::cont::ArrayHandle<vtkm::Id> density;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, uniform.GetNumberOfPoints()),
                        density);

  this->Invoke(vtkm::worklet::CICWorklet{}, field, locator, density);

  uniform.AddField(vtkm::cont::make_FieldPoint("density", density));

  return uniform;
}

}
}
#endif // vtk_m_filter_particle_density_cic_hxx
