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
#ifndef vtkm_m_worklet_Triangulate_h
#define vtkm_m_worklet_Triangulate_h

#include <vtkm/worklet/triangulate/TriangulateExplicit.h>
#include <vtkm/worklet/triangulate/TriangulateStructured.h>

namespace vtkm
{
namespace worklet
{

class Triangulate
{
public:
  //
  // Distribute multiple copies of cell data depending on cells create from original
  //
  struct DistributeCellData : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn<> inIndices, FieldOut<> outIndices);
    using ExecutionSignature = void(_1, _2);

    using ScatterType = vtkm::worklet::ScatterCounting;

    template <typename CountArrayType>
    VTKM_CONT static ScatterType MakeScatter(const CountArrayType& countArray)
    {
      return ScatterType(countArray);
    }

    template <typename T>
    VTKM_EXEC void operator()(T inputIndex, T& outputIndex) const
    {
      outputIndex = inputIndex;
    }
  };

  Triangulate()
    : OutCellsPerCell()
  {
  }

  // Triangulate explicit data set, save number of triangulated cells per input
  template <typename CellSetType>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet)
  {
    TriangulateExplicit worklet;
    return worklet.Run(cellSet, this->OutCellsPerCell);
  }

  // Triangulate structured data set, save number of triangulated cells per input
  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<2>& cellSet)
  {
    TriangulateStructured worklet;
    return worklet.Run(cellSet, this->OutCellsPerCell);
  }

  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<3>&)
  {
    throw vtkm::cont::ErrorBadType("CellSetStructured<3> can't be triangulated");
  }

  // Using the saved input to output cells, expand cell data
  template <typename ValueType, typename StorageType>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& input) const
  {
    vtkm::cont::ArrayHandle<ValueType> output;

    vtkm::worklet::DispatcherMapField<DistributeCellData> dispatcher(
      DistributeCellData::MakeScatter(this->OutCellsPerCell));
    dispatcher.Invoke(input, output);

    return output;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::IdComponent> OutCellsPerCell;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Triangulate_h
