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

#include "ParticleDensityNGP.h"
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
  using ControlSignature = void(FieldIn coords, ExecObject locator, AtomicArrayInOut density);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename Point, typename CellLocatorExecObj, typename AtomicArray>
  VTKM_EXEC void operator()(const Point& point,
                            const CellLocatorExecObj& locator,
                            AtomicArray& density)
  {
    vtkm::Id cellId;
    vtkm::Vec3f parametric;

    // Find the cell containing the point
    locator.FindCell(point, cellId, parametric, *this);

    // increment density
    density.Add(cellId, 1);
  }
}; //NGPWorklet
} //worklet
} //vtkm


namespace vtkm
{
namespace filter
{
inline VTKM_CONT ParticleDensityNGP::ParticleDensityNGP(vtkm::Id3& dimension,
                                                        vtkm::Vec3f& origin,
                                                        vtkm::Vec3f& spacing)
  : Dimension(dimension)
  , Origin(origin)
  , Spacing(spacing)
{
}

template <typename T, typename StorageType, typename Policy>
inline VTKM_CONT vtkm::cont::DataSet ParticleDensityNGP::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<Policy> policy)
{
  // TODO: it really doesn't need to be a UniformGrid, any CellSet with CellLocator will work.
  // Make it another input rather an output generated.

  // We want to stores density as PointField which conforms to VTK/VTKm's idea of ImageDataset
  // and works with the ImageConnectivity for segmentation purpose. We thus create a surrogate
  // uniform dataset that has the cell dimension as the point dimension of the output. We use
  // this dataset only for point in cell lookup purpose. This is a somewhat convolved way of
  // doing some simple math.
  auto surrogate = vtkm::cont::DataSetBuilderUniform::Create(
    this->Dimension - vtkm::Id3{ 1, 1, 1 }, this->Origin, this->Spacing);

  // Create a CellSetLocator
  vtkm::cont::CellLocatorUniformGrid locator;
  locator.SetCellSet(surrogate.GetCellSet());
  locator.SetCoordinates(surrogate.GetCoordinateSystem());
  locator.Update();

  // We still use an ArrayHandle<T> and pass it to the Worklet as AtomicArrayInOut
  // it will be type converted automatically. However the ArrayHandle needs to be
  // allocated and initialized. The easily way to do it is to copy from an
  // ArrayHandleConstant
  vtkm::cont::ArrayHandle<vtkm::Id> density;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<vtkm::Id>(0, surrogate.GetNumberOfCells()),
                        density);

  this->Invoke(input, locator, density);

  surrogate.AddField(vtkm::cont::make_FieldCell("density", density));

  return surrogate;
}
}
}
#endif //vtk_m_filter_particle_density_ngp_hxx
