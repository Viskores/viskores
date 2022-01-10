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

namespace vtkm
{
namespace filter
{
namespace
{

struct CallWorklet
{
  vtkm::Id Stride;
  vtkm::cont::UnknownCellSet& Output;
  vtkm::worklet::Mask& Worklet;

  CallWorklet(vtkm::Id stride, vtkm::cont::UnknownCellSet& output, vtkm::worklet::Mask& worklet)
    : Stride(stride)
    , Output(output)
    , Worklet(worklet)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cells) const
  {
    this->Output = this->Worklet.Run(cells, this->Stride);
  }
};

VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                          const vtkm::cont::Field& field,
                          const vtkm::worklet::Mask& Worklet)
{
  if (field.IsFieldPoint() || field.IsFieldGlobal())
  {
    result.AddField(field); // pass through
    return true;
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, Worklet.GetValidCellIds(), result);
  }
  else
  {
    return false;
  }
}
} // end anon namespace

namespace entity_extraction
{
//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet Mask::DoExecute(const vtkm::cont::DataSet& input)
{
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  vtkm::cont::UnknownCellSet cellOut;
  vtkm::worklet::Mask Worklet;

  CallWorklet workletCaller(this->Stride, cellOut, Worklet);
  cells.CastAndCallForTypes<VTKM_DEFAULT_CELL_SET_LIST>(workletCaller);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  output.SetCellSet(cellOut);

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, Worklet); };
  MapFieldsOntoOutput(input, output, mapper);

  return output;
}
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
