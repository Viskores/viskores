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
  const vtkm::ImplicitFunctionGeneral& Function;
  bool ExtractInside;
  bool ExtractBoundaryCells;
  bool ExtractOnlyBoundaryCells;

  CallWorker(vtkm::cont::DynamicCellSet& output,
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

} // end anon namespace

namespace vtkm
{
namespace filter
{

//-----------------------------------------------------------------------------
template <typename DerivedPolicy>
vtkm::cont::DataSet ExtractGeometry::DoExecute(const vtkm::cont::DataSet& input,
                                               vtkm::filter::PolicyBase<DerivedPolicy> policy)
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
  vtkm::filter::ApplyPolicyCellSet(cells, policy, *this).CastAndCall(worker);

  // create the output dataset
  vtkm::cont::DataSet output;
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));
  output.SetCellSet(outCells);
  return output;
}
}
}

#endif
