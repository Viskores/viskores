//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/density_estimate/ContinuousScatterPlot.h>
#include <vtkm/filter/density_estimate/worklet/ContinuousScatterPlot.h>
#include <vtkm/filter/geometry_refinement/Tetrahedralize.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{

VTKM_CONT vtkm::cont::DataSet ContinuousScatterPlot::DoExecute(const vtkm::cont::DataSet& input)
{
  // This algorithm only operate on tetra cells, we need to apply the tetrahedralize filter first.
  auto tetrahedralizeFilter = vtkm::filter::geometry_refinement::Tetrahedralize();
  auto tetraInput = tetrahedralizeFilter.Execute(input);
  vtkm::cont::CellSetSingleType<> tetraCellSet;
  tetraInput.GetCellSet().AsCellSet(tetraCellSet);

  vtkm::cont::Field scalarField1 = input.GetField(GetActiveFieldName(0));
  vtkm::cont::Field scalarField2 = input.GetField(GetActiveFieldName(1));

  if (!(scalarField1.IsPointField() && scalarField2.IsPointField()))
  {
    throw vtkm::cont::ErrorFilterExecution("Point fields expected.");
  }

  const auto& coords = input.GetCoordinateSystem().GetDataAsMultiplexer();
  vtkm::cont::CoordinateSystem activeCoordSystem = input.GetCoordinateSystem();

  vtkm::cont::DataSet scatterplotDataSet;
  vtkm::worklet::ContinuousScatterPlot worklet;

  auto resolveFieldType = [&](const auto& resolvedScalar) {
    using FieldType = typename std::decay_t<decltype(resolvedScalar)>::ValueType;

    vtkm::cont::CellSetSingleType<> scatterplotCellSet;
    vtkm::cont::ArrayHandle<FieldType> density;
    vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> newCoords;

    // Both fields need to resolve to the same type in order to perform calculations
    vtkm::cont::ArrayHandle<FieldType> resolvedScalar2;
    vtkm::cont::ArrayCopyShallowIfPossible(scalarField2.GetData(), resolvedScalar2);

    worklet.Run(tetraCellSet,
                coords,
                newCoords,
                density,
                resolvedScalar,
                resolvedScalar2,
                scatterplotCellSet);

    // Populate the new dataset representing the continuous scatterplot
    // Using the density field and coordinates calculated by the worklet
    activeCoordSystem = vtkm::cont::CoordinateSystem(activeCoordSystem.GetName(), newCoords);
    scatterplotDataSet.AddCoordinateSystem(activeCoordSystem);
    scatterplotDataSet.SetCellSet(scatterplotCellSet);
    scatterplotDataSet.AddPointField(this->GetOutputFieldName(), density);
  };

  this->CastAndCallScalarField(scalarField1.GetData(), resolveFieldType);

  return scatterplotDataSet;
}

} // namespace density_estimate
} // namespace filter
} // namespace vtkm
