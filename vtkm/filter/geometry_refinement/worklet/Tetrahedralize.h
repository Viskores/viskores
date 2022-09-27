//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_m_worklet_Tetrahedralize_h
#define vtkm_m_worklet_Tetrahedralize_h

#include <vtkm/filter/geometry_refinement/worklet/tetrahedralize/TetrahedralizeExplicit.h>
#include <vtkm/filter/geometry_refinement/worklet/tetrahedralize/TetrahedralizeStructured.h>

namespace vtkm
{
namespace worklet
{

class Tetrahedralize
{
public:
  //
  // Distribute multiple copies of cell data depending on cells create from original
  //
  struct DistributeCellData : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn inIndices, FieldOut outIndices);
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

  Tetrahedralize()
    : OutCellScatter(vtkm::cont::ArrayHandle<vtkm::IdComponent>{})
  {
  }

  // Tetrahedralize explicit data set, save number of tetra cells per input
  template <typename CellSetType>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet)
  {
    TetrahedralizeExplicit worklet;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> outCellsPerCell;
    vtkm::cont::CellSetSingleType<> result = worklet.Run(cellSet, outCellsPerCell);
    this->OutCellScatter = DistributeCellData::MakeScatter(outCellsPerCell);
    return result;
  }

  // Tetrahedralize structured data set, save number of tetra cells per input
  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<3>& cellSet)
  {
    TetrahedralizeStructured worklet;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> outCellsPerCell;
    vtkm::cont::CellSetSingleType<> result = worklet.Run(cellSet, outCellsPerCell);
    this->OutCellScatter = DistributeCellData::MakeScatter(outCellsPerCell);
    return result;
  }

  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<2>&)
  {
    throw vtkm::cont::ErrorBadType("CellSetStructured<2> can't be tetrahedralized");
  }

  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<1>&)
  {
    throw vtkm::cont::ErrorBadType("CellSetStructured<1> can't be tetrahedralized");
  }

  DistributeCellData::ScatterType GetOutCellScatter() const { return this->OutCellScatter; }

private:
  DistributeCellData::ScatterType OutCellScatter;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Tetrahedralize_h
