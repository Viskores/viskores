//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace
{

template <typename DeviceTag>
struct CallWorker
{
  vtkm::cont::DynamicCellSet& Output;
  vtkm::worklet::ExtractGeometry& Worklet;
  const vtkm::cont::CoordinateSystem& Coords;
  const vtkm::cont::ImplicitFunctionHandle& Function;
  bool ExtractInside;
  bool ExtractBoundaryCells;
  bool ExtractOnlyBoundaryCells;

  CallWorker(vtkm::cont::DynamicCellSet& output,
             vtkm::worklet::ExtractGeometry& worklet,
             const vtkm::cont::CoordinateSystem& coords,
             const vtkm::cont::ImplicitFunctionHandle& function,
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
                                     this->ExtractOnlyBoundaryCells,
                                     DeviceTag());
  }
};

} // end anon namespace

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
inline VTKM_CONT ExtractGeometry::ExtractGeometry()
  : vtkm::filter::FilterDataSet<ExtractGeometry>()
  , ExtractInside(true)
  , ExtractBoundaryCells(false)
  , ExtractOnlyBoundaryCells(false)
{
}

//-----------------------------------------------------------------------------
template <typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet ExtractGeometry::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
  const DeviceAdapter&)
{
  // extract the input cell set and coordinates
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet(this->GetActiveCellSetIndex());
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::DynamicCellSet outCells;
  CallWorker<DeviceAdapter> worker(outCells,
                                   this->Worklet,
                                   coords,
                                   this->Function,
                                   this->ExtractInside,
                                   this->ExtractBoundaryCells,
                                   this->ExtractOnlyBoundaryCells);
  vtkm::filter::ApplyPolicy(cells, policy).CastAndCall(worker);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  output.AddCellSet(outCells);
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT bool ExtractGeometry::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter& device)
{
  vtkm::cont::DynamicArrayHandle output;

  if (fieldMeta.IsPointField())
  {
    output = input; // pass through, points aren't changed.
  }
  else if (fieldMeta.IsCellField())
  {
    output = this->Worklet.ProcessCellField(input, device);
  }
  else
  {
    return false;
  }

  // use the same meta data as the input so we get the same field name, etc.
  result.AddField(fieldMeta.AsField(output));
  return true;
}
}
}
