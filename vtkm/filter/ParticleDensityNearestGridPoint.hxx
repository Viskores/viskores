//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_particle_density_ngp_hxx
#define vtk_m_filter_particle_density_ngp_hxx

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/filter/PolicyBase.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
class NGPWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn coords,
                                FieldIn field,
                                ExecObject locator,
                                AtomicArrayInOut density);
  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename Point, typename T, typename CellLocatorExecObj, typename AtomicArray>
  VTKM_EXEC void operator()(const Point& point,
                            const T value,
                            const CellLocatorExecObj& locator,
                            AtomicArray& density) const
  {
    vtkm::Id cellId{};
    vtkm::Vec3f parametric;

    // Find the cell containing the point
    if (locator.FindCell(point, cellId, parametric) == vtkm::ErrorCode::Success)
    {
      // deposit field value to density
      density.Add(cellId, value);
    }

    // We simply ignore that particular particle when it is not in the mesh.
  }
}; //NGPWorklet
} //worklet
} //vtkm

namespace vtkm
{
namespace filter
{
inline VTKM_CONT ParticleDensityNearestGridPoint::ParticleDensityNearestGridPoint(
  const vtkm::Id3& dimension,
  const vtkm::Vec3f& origin,
  const vtkm::Vec3f& spacing)
  : Superclass(dimension, origin, spacing)
{
}

inline VTKM_CONT ParticleDensityNearestGridPoint::ParticleDensityNearestGridPoint(
  const Id3& dimension,
  const vtkm::Bounds& bounds)
  : Superclass(dimension, bounds)
{
}

template <typename T, typename StorageType, typename Policy>
inline VTKM_CONT vtkm::cont::DataSet ParticleDensityNearestGridPoint::DoExecute(
  const vtkm::cont::DataSet& dataSet,
  const vtkm::cont::ArrayHandle<T, StorageType>&
    field, // particles' scala field to be deposited to the mesh, e.g. mass or charge
  const vtkm::filter::FieldMetadata&,
  vtkm::filter::PolicyBase<Policy>)
{
  // TODO: it really doesn't need to be a UniformGrid, any CellSet with CellLocator will work.
  //  Make it another input rather an output generated.

  // We stores density as CellField which conforms to physicists' idea of particle density
  // better. However, VTK/VTKm's idea of "Image" Dataset and the ImageConnectivity filter
  // expect a PointField. For better separation of concerns, we create a uniform dataset
  // that has the cell dimension as expected and later convert the dataset to its dual.
  auto uniform = vtkm::cont::DataSetBuilderUniform::Create(
    this->Dimension + vtkm::Id3{ 1, 1, 1 }, this->Origin, this->Spacing);

  // Create a CellLocator
  vtkm::cont::CellLocatorUniformGrid locator;
  locator.SetCellSet(uniform.GetCellSet());
  locator.SetCoordinates(uniform.GetCoordinateSystem());
  locator.Update();

  auto coords = dataSet.GetCoordinateSystem().GetDataAsMultiplexer();

  // We create an ArrayHandle and pass it to the Worklet as AtomicArrayInOut.
  // However the ArrayHandle needs to be allocated and initialized first. The
  // easiest way to do it is to copy from an ArrayHandleConstant
  vtkm::cont::ArrayHandle<T> density;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<T>(0, uniform.GetNumberOfCells()), density);

  this->Invoke(vtkm::worklet::NGPWorklet{}, coords, field, locator, density);

  if (DivideByVolume)
  {
    auto volume = this->Spacing[0] * this->Spacing[1] * this->Spacing[2];
    this->Invoke(DivideByVolumeWorklet{ volume }, density);
  }

  uniform.AddField(vtkm::cont::make_FieldCell("density", density));

  return uniform;
}

}
}
#endif //vtk_m_filter_particle_density_ngp_hxx
