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

#include <vtkm/worklet/tetrahedralize/TetrahedralizeExplicit.h>
#include <vtkm/worklet/tetrahedralize/TetrahedralizeStructured.h>

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
    : OutCellsPerCell()
  {
  }

  // Tetrahedralize explicit data set, save number of tetra cells per input
  template <typename CellSetType>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet)
  {
    TetrahedralizeExplicit worklet;
    return worklet.Run(cellSet, this->OutCellsPerCell);
  }

  // Tetrahedralize structured data set, save number of tetra cells per input
  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<3>& cellSet)
  {
    TetrahedralizeStructured worklet;
    return worklet.Run(cellSet, this->OutCellsPerCell);
  }

  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<2>&)
  {
    throw vtkm::cont::ErrorBadType("CellSetStructured<2> can't be tetrahedralized");
  }

  // Using the saved input to output cells, expand cell data
  template <typename T, typename StorageType>
  vtkm::cont::ArrayHandle<T> ProcessCellField(
    const vtkm::cont::ArrayHandle<T, StorageType>& input) const
  {
    vtkm::cont::ArrayHandle<T> output;

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

#endif // vtkm_m_worklet_Tetrahedralize_h
