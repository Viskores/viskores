//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ExtractGeometry_hxx
#define vtk_m_filter_ExtractGeometry_hxx

#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>

namespace
{

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
                                     this->ExtractOnlyBoundaryCells);
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
template <typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ExtractGeometry::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  // extract the input cell set and coordinates
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  const vtkm::cont::CoordinateSystem& coords =
    input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  vtkm::cont::DynamicCellSet outCells;
  CallWorker worker(outCells,
                    this->Worklet,
                    coords,
                    this->Function,
                    this->ExtractInside,
                    this->ExtractBoundaryCells,
                    this->ExtractOnlyBoundaryCells);
  vtkm::filter::ApplyPolicyCellSet(cells, policy).CastAndCall(worker);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  output.SetCellSet(outCells);
  return output;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool ExtractGeometry::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  const vtkm::filter::PolicyBase<DerivedPolicy>&)
{
  vtkm::cont::VariantArrayHandle output;

  if (fieldMeta.IsPointField())
  {
    output = input; // pass through, points aren't changed.
  }
  else if (fieldMeta.IsCellField())
  {
    output = this->Worklet.ProcessCellField(input);
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

#endif
