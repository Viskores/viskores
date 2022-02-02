//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/CellLocatorUniformGrid.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/filter/density_estimate/ParticleDensityCloudInCell.h>
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
namespace density_estimate
{
VTKM_CONT ParticleDensityCloudInCell::ParticleDensityCloudInCell(const vtkm::Id3& dimension,
                                                                 const vtkm::Vec3f& origin,
                                                                 const vtkm::Vec3f& spacing)
  : Superclass(dimension, origin, spacing)
{
}

VTKM_CONT ParticleDensityCloudInCell::ParticleDensityCloudInCell(const Id3& dimension,
                                                                 const vtkm::Bounds& bounds)
  : Superclass(dimension, bounds)
{
}

VTKM_CONT vtkm::cont::DataSet ParticleDensityCloudInCell::DoExecute(const cont::DataSet& input)
{
  // Unlike ParticleDensityNGP, particle deposit mass on the grid points, thus it is natural to
  // return the density as PointField;
  auto uniform = vtkm::cont::DataSetBuilderUniform::Create(
    this->Dimension + vtkm::Id3{ 1, 1, 1 }, this->Origin, this->Spacing);

  vtkm::cont::CellLocatorUniformGrid locator;
  locator.SetCellSet(uniform.GetCellSet());
  locator.SetCoordinates(uniform.GetCoordinateSystem());
  locator.Update();

  auto coords = input.GetCoordinateSystem().GetDataAsMultiplexer();

  auto resolveType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;

    // We create an ArrayHandle and pass it to the Worklet as AtomicArrayInOut.
    // However, the ArrayHandle needs to be allocated and initialized first.
    vtkm::cont::ArrayHandle<T> density;
    density.AllocateAndFill(uniform.GetNumberOfPoints(), 0);

    this->Invoke(vtkm::worklet::CICWorklet{},
                 coords,
                 concrete,
                 locator,
                 uniform.GetCellSet().template AsCellSet<vtkm::cont::CellSetStructured<3>>(),
                 density);

    if (DivideByVolume)
    {
      this->DoDivideByVolume(density);
    }

    uniform.AddField(vtkm::cont::make_FieldPoint("density", density));
  };

  // Note: This is the so called Immediately-Invoked Function Expression (IIFE). Here we define
  // a lambda expression and immediately call it at the end. This allows us to not declare an
  // UnknownArrayHandle first and then assign it in the if-else statement. If I really want to
  // show-off, I can even inline the `fieldArray` variable and turn it into a long expression.
  auto fieldArray = [&]() -> vtkm::cont::UnknownArrayHandle {
    if (this->ComputeNumberDensity)
    {
      return vtkm::cont::make_ArrayHandleConstant(vtkm::FloatDefault{ 1 },
                                                  input.GetNumberOfPoints());
    }
    else
    {
      return this->GetFieldFromDataSet(input).GetData();
    }
  }();
  fieldArray.CastAndCallForTypes<
    vtkm::TypeListFieldScalar,
    vtkm::ListAppend<VTKM_DEFAULT_STORAGE_LIST, vtkm::List<vtkm::cont::StorageTagConstant>>>(
    resolveType);

  // Deposition of the input field to the output field is already mapping. No need to map other
  // fields.
  return uniform;
}
} // namespace density_estimate
} // namespace filter
} // namespace vtkm
