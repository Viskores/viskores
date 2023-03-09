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
VTKM_CONT void DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& CellFieldMap,
                          const vtkm::cont::ArrayHandle<vtkm::Id>& PointFieldMap,
                          const vtkm::worklet::ExtractStructured& worklet)
{
  if (field.IsPointField())
  {
    vtkm::cont::UnknownArrayHandle array = field.GetData();
    using UniformCoordinatesArrayHandle =
      vtkm::worklet::ExtractStructured::UniformCoordinatesArrayHandle;
    using RectilinearCoordinatesArrayHandle =
      vtkm::worklet::ExtractStructured::RectilinearCoordinatesArrayHandle;
    if (array.CanConvert<UniformCoordinatesArrayHandle>())
    {
      // Special case that is more efficient for uniform coordinate arrays.
      UniformCoordinatesArrayHandle newCoords =
        worklet.MapCoordinatesUniform(array.AsArrayHandle<UniformCoordinatesArrayHandle>());
      result.AddField(vtkm::cont::Field(field.GetName(), field.GetAssociation(), newCoords));
    }
    else if (array.CanConvert<RectilinearCoordinatesArrayHandle>())
    {
      // Special case that is more efficient for uniform coordinate arrays.
      RectilinearCoordinatesArrayHandle newCoords =
        worklet.MapCoordinatesRectilinear(array.AsArrayHandle<RectilinearCoordinatesArrayHandle>());
      result.AddField(vtkm::cont::Field(field.GetName(), field.GetAssociation(), newCoords));
    }
    else
    {
      vtkm::filter::MapFieldPermutation(field, PointFieldMap, result);
    }
  }
  else if (field.IsCellField())
  {
    vtkm::filter::MapFieldPermutation(field, CellFieldMap, result);
  }
  else if (field.IsWholeDataSetField())
  {
    result.AddField(field);
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

  vtkm::worklet::ExtractStructured worklet;
  auto cellset = worklet.Run(cells.ResetCellSetList<VTKM_DEFAULT_CELL_SET_LIST_STRUCTURED>(),
                             this->VOI,
                             this->SampleRate,
                             this->IncludeBoundary,
                             this->IncludeOffset);

  // Create map arrays for mapping fields. Could potentially save some time to first check to see
  // if these arrays would be used.
  auto CellFieldMap =
    worklet.ProcessCellField(vtkm::cont::ArrayHandleIndex(input.GetNumberOfCells()));
  auto PointFieldMap =
    worklet.ProcessPointField(vtkm::cont::ArrayHandleIndex(input.GetNumberOfPoints()));

  auto mapper = [&](auto& result, const auto& f) {
    DoMapField(result, f, CellFieldMap, PointFieldMap, worklet);
  };
  return this->CreateResult(input, cellset, mapper);
}

} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
