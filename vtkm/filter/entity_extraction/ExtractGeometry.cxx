//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/entity_extraction/ExtractGeometry.h>
#include <vtkm/filter/entity_extraction/worklet/ExtractGeometry.h>

namespace
{
bool DoMapField(vtkm::cont::DataSet& result,
                const vtkm::cont::Field& field,
                const vtkm::worklet::ExtractGeometry& worklet)
{
  if (field.IsPointField())
  {
    result.AddField(field);
    return true;
  }
  else if (field.IsCellField())
  {
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = worklet.GetValidCellIds();
    return vtkm::filter::MapFieldPermutation(field, permutation, result);
  }
  else if (field.IsWholeDataSetField())
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
namespace entity_extraction
{
//-----------------------------------------------------------------------------
vtkm::cont::DataSet ExtractGeometry::DoExecute(const vtkm::cont::DataSet& input)
{
  // extract the input cell set and coordinates
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::worklet::ExtractGeometry worklet;
  vtkm::cont::UnknownCellSet outCells;

  cells.CastAndCallForTypes<VTKM_DEFAULT_CELL_SET_LIST>([&](const auto& concrete) {
    outCells = worklet.Run(concrete,
                           coords,
                           this->Function,
                           this->ExtractInside,
                           this->ExtractBoundaryCells,
                           this->ExtractOnlyBoundaryCells);
  });

  // create the output dataset
  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, outCells, mapper);
}

} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
