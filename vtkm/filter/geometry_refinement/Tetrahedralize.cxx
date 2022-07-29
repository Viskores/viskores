//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/geometry_refinement/Tetrahedralize.h>
#include <vtkm/filter/geometry_refinement/worklet/Tetrahedralize.h>

namespace
{
struct DeduceCellSet
{
  template <typename CellSetType>
  void operator()(const CellSetType& cellset,
                  vtkm::worklet::Tetrahedralize& worklet,
                  vtkm::cont::CellSetSingleType<>& outCellSet) const
  {
    outCellSet = worklet.Run(cellset);
  }
};

VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          const vtkm::worklet::Tetrahedralize& worklet)
{
  if (field.IsFieldPoint())
  {
    // point data is copied as is because it was not collapsed
    result.AddField(field);
    return true;
  }
  else if (field.IsFieldCell())
  {
    // cell data must be scattered to the cells created per input cell
    vtkm::cont::ArrayHandle<vtkm::Id> permutation =
      worklet.GetOutCellScatter().GetOutputToInputMap();
    return vtkm::filter::MapFieldPermutation(field, permutation, result);
  }
  else if (field.IsFieldGlobal())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    return false;
  }
}
} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{
VTKM_CONT vtkm::cont::DataSet Tetrahedralize::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();

  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::worklet::Tetrahedralize worklet;
  vtkm::cont::CastAndCall(cells, DeduceCellSet{}, worklet, outCellSet);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  // create the output dataset (without a CoordinateSystem).
  vtkm::cont::DataSet output = this->CreateResult(input, outCellSet, mapper);

  // We did not change the geometry of the input dataset at all. Just attach coordinate system
  // of input dataset to output dataset.
  for (vtkm::IdComponent coordSystemId = 0; coordSystemId < input.GetNumberOfCoordinateSystems();
       ++coordSystemId)
  {
    output.AddCoordinateSystem(input.GetCoordinateSystem(coordSystemId));
  }
  return output;
}
} // namespace geometry_refinement
} // namespace filter
} // namespace vtkm
