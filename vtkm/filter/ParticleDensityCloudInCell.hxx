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

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/filter/PolicyBase.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
class CICWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn coords,
                                FieldIn field,
                                ExecObject locator,
                                WholeCellSetIn<Cell, Point> cellSet,
                                AtomicArrayInOut density);
  using ExecutionSignature = void(_1, _2, _3, _4, _5);

  template <typename Point,
            typename T,
            typename CellLocatorExecObj,
            typename CellSet,
            typename AtomicArray>
  VTKM_EXEC void operator()(const Point& point,
                            const T value,
                            const CellLocatorExecObj& locator,
                            const CellSet& cellSet,
                            AtomicArray& density) const
  {
    vtkm::Id cellId{};
    vtkm::Vec3f parametric;

    if (locator.FindCell(point, cellId, parametric) == vtkm::ErrorCode::Success)
    {
      // iterate through all the points of the cell and deposit with correct weight.
      auto indices = cellSet.GetIndices(cellId);
      auto rparametric = vtkm::Vec3f{ 1, 1, 1 } - parametric;

      // deposit the scalar field value in proportion to the volume of the sub-hexahedron
      // the vertex is in.
      density.Add(indices[0], value * parametric[0] * parametric[1] * parametric[2]);
      density.Add(indices[1], value * rparametric[0] * parametric[1] * parametric[2]);
      density.Add(indices[2], value * rparametric[0] * rparametric[1] * parametric[2]);
      density.Add(indices[3], value * parametric[0] * rparametric[1] * parametric[2]);

      density.Add(indices[4], value * parametric[0] * parametric[1] * rparametric[2]);
      density.Add(indices[5], value * rparametric[0] * parametric[1] * rparametric[2]);
      density.Add(indices[6], value * rparametric[0] * rparametric[1] * rparametric[2]);
      density.Add(indices[7], value * parametric[0] * rparametric[1] * rparametric[2]);
    }

    // We simply ignore that particular particle when it is not in the mesh.
  }
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
  : Superclass(dimension, origin, spacing)
{
}

inline VTKM_CONT ParticleDensityCloudInCell::ParticleDensityCloudInCell(const Id3& dimension,
                                                                        const vtkm::Bounds& bounds)
  : Superclass(dimension, bounds)
{
}

template <typename T, typename StorageType, typename Policy>
inline VTKM_CONT vtkm::cont::DataSet ParticleDensityCloudInCell::DoExecute(
  const cont::DataSet& dataSet,
  const cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata&,
  PolicyBase<Policy>)
{
  // Unlike ParticleDensityNGP, particle deposit mass on the grid points, thus it is natural to
  // return the density as PointField;
  auto uniform = vtkm::cont::DataSetBuilderUniform::Create(
    this->Dimension + vtkm::Id3{ 1, 1, 1 }, this->Origin, this->Spacing);

  vtkm::cont::CellLocatorUniformGrid locator;
  locator.SetCellSet(uniform.GetCellSet());
  locator.SetCoordinates(uniform.GetCoordinateSystem());
  locator.Update();

  auto coords = dataSet.GetCoordinateSystem().GetDataAsMultiplexer();

  vtkm::cont::ArrayHandle<T> density;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<T>(0, uniform.GetNumberOfPoints()),
                        density);

  this->Invoke(vtkm::worklet::CICWorklet{},
               coords,
               field,
               locator,
               uniform.GetCellSet().template Cast<vtkm::cont::CellSetStructured<3>>(),
               density);

  if (DivideByVolume)
  {
    auto volume = this->Spacing[0] * this->Spacing[1] * this->Spacing[2];
    this->Invoke(DivideByVolumeWorklet{ volume }, density);
  }

  uniform.AddField(vtkm::cont::make_FieldPoint("density", density));

  return uniform;
}

}
}
#endif // vtk_m_filter_particle_density_cic_hxx
