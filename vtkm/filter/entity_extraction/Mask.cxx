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
#include <vtkm/filter/entity_extraction/Mask.h>
#include <vtkm/filter/entity_extraction/worklet/Mask.h>

namespace
{
VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          const vtkm::worklet::Mask& worklet)
{
  if (field.IsPointField() || field.IsWholeDataSetField())
  {
    result.AddField(field); // pass through
    return true;
  }
  else if (field.IsCellField())
  {
    return vtkm::filter::MapFieldPermutation(field, worklet.GetValidCellIds(), result);
  }
  else
  {
    return false;
  }
}
} // end anon namespace

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet Mask::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  vtkm::cont::UnknownCellSet cellOut;
  vtkm::worklet::Mask worklet;

  cells.CastAndCallForTypes<VTKM_DEFAULT_CELL_SET_LIST>(
    [&](const auto& concrete) { cellOut = worklet.Run(concrete, this->Stride); });

  // create the output dataset
  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, cellOut, mapper);
}
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
