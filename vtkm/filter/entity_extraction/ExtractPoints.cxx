//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/UncertainCellSet.h>
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/filter/clean_grid/CleanGrid.h>
#include <vtkm/filter/entity_extraction/ExtractPoints.h>
#include <vtkm/filter/entity_extraction/worklet/ExtractPoints.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::DataSet ExtractPoints::DoExecute(const vtkm::cont::DataSet& input)
{
  // extract the input cell set and coordinates
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  // run the worklet on the cell set
  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::worklet::ExtractPoints worklet;

  // FIXME: is the other overload of .Run ever used?
  outCellSet = worklet.Run(cells, coords.GetData(), this->Function, this->ExtractInside);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  auto mapper = [&, this](auto& result, const auto& f) { this->MapFieldOntoOutput(result, f); };
  this->MapFieldsOntoOutput(input, output, mapper);

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    vtkm::filter::clean_grid::CleanGrid compactor;
    compactor.SetCompactPointFields(true);
    compactor.SetMergePoints(false);
    return compactor.Execute(output);
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
VTKM_CONT bool ExtractPoints::MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                                 const vtkm::cont::Field& field)
{
  // point data is copied as is because it was not collapsed
  if (field.IsFieldPoint())
  {
    result.AddField(field);
    return true;
  }
  else if (field.IsFieldGlobal())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    // cell data does not apply
    return false;
  }
}
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
