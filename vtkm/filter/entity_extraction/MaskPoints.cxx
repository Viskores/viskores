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
#include <vtkm/filter/clean_grid/CleanGrid.h>
#include <vtkm/filter/entity_extraction/MaskPoints.h>
#include <vtkm/filter/entity_extraction/worklet/MaskPoints.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet MaskPoints::DoExecute(const vtkm::cont::DataSet& input)
{
  // extract the input cell set
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();

  // run the worklet on the cell set and input field
  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::worklet::MaskPoints worklet;

  outCellSet = worklet.Run(cells, this->Stride);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  auto mapper = [&, this](auto& result, const auto& f) { this->MapFieldOntoOutput(result, f); };
  MapFieldsOntoOutput(input, output, mapper);

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    vtkm::filter::clean_grid::CleanGrid Compactor;
    Compactor.SetCompactPointFields(true);
    Compactor.SetMergePoints(false);
    return Compactor.Execute(output);
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
VTKM_CONT bool MaskPoints::MapFieldOntoOutput(vtkm::cont::DataSet& result,
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
