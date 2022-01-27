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
struct CallWorker
{
  vtkm::cont::UnknownCellSet& Output;
  vtkm::worklet::ExtractGeometry& Worklet;
  const vtkm::cont::CoordinateSystem& Coords;
  const vtkm::ImplicitFunctionGeneral& Function;
  bool ExtractInside;
  bool ExtractBoundaryCells;
  bool ExtractOnlyBoundaryCells;

  CallWorker(vtkm::cont::UnknownCellSet& output,
             vtkm::worklet::ExtractGeometry& worklet,
             const vtkm::cont::CoordinateSystem& coords,
             const vtkm::ImplicitFunctionGeneral& function,
             bool extractInside,
             bool extractBoundaryCells,
             bool extractOnlyBoundaryCells)
    : Output(output)
    , Worklet(worklet)
    , Coords(coords)
    , Function(function)
    , ExtractInside(extractInside)
    , ExtractBoundaryCells(extractBoundaryCells)
    , ExtractOnlyBoundaryCells(extractOnlyBoundaryCells)
  {
  }

  template <typename CellSetType>
  void operator()(const CellSetType& cellSet) const
  {
    this->Output = this->Worklet.Run(cellSet,
                                     this->Coords,
                                     this->Function,
                                     this->ExtractInside,
                                     this->ExtractBoundaryCells,
                                     this->ExtractOnlyBoundaryCells);
  }
};

bool DoMapField(vtkm::cont::DataSet& result,
                const vtkm::cont::Field& field,
                const vtkm::worklet::ExtractGeometry& worklet)
{
  if (field.IsFieldPoint())
  {
    result.AddField(field);
    return true;
  }
  else if (field.IsFieldCell())
  {
    vtkm::cont::ArrayHandle<vtkm::Id> permutation = worklet.GetValidCellIds();
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
  CallWorker worker(outCells,
                    worklet,
                    coords,
                    this->Function,
                    this->ExtractInside,
                    this->ExtractBoundaryCells,
                    this->ExtractOnlyBoundaryCells);
  cells.CastAndCallForTypes<VTKM_DEFAULT_CELL_SET_LIST>(worker);

  // create the output dataset
  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, worklet); };
  return this->CreateResult(input, outCells, input.GetCoordinateSystems(), mapper);
}

} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
