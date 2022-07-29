//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayHandleIndex.h>

#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/entity_extraction/ExtractStructured.h>
#include <vtkm/filter/entity_extraction/worklet/ExtractStructured.h>

namespace
{
VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& CellFieldMap,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& PointFieldMap)
{
  if (field.IsFieldPoint())
  {
    return vtkm::filter::MapFieldPermutation(field, PointFieldMap, result);
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, CellFieldMap, result);
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
namespace entity_extraction
{
//-----------------------------------------------------------------------------
vtkm::cont::DataSet ExtractStructured::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coordinates = input.GetCoordinateSystem();

  vtkm::worklet::ExtractStructured worklet;
  auto cellset = worklet.Run(cells.ResetCellSetList<VTKM_DEFAULT_CELL_SET_LIST_STRUCTURED>(),
                             this->VOI,
                             this->SampleRate,
                             this->IncludeBoundary,
                             this->IncludeOffset);

  auto coords = worklet.MapCoordinates(coordinates);
  vtkm::cont::CoordinateSystem outputCoordinates(coordinates.GetName(), coords);

  // Create map arrays for mapping fields. Could potentially save some time to first check to see
  // if these arrays would be used.
  auto CellFieldMap =
    worklet.ProcessCellField(vtkm::cont::ArrayHandleIndex(input.GetNumberOfCells()));
  auto PointFieldMap =
    worklet.ProcessPointField(vtkm::cont::ArrayHandleIndex(input.GetNumberOfPoints()));

  auto mapper = [&](auto& result, const auto& f) {
    DoMapField(result, f, CellFieldMap, PointFieldMap);
  };
  return this->CreateResult(input, cellset, outputCoordinates, mapper);
}

} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
