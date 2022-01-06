//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_m_worklet_ThresholdPoints_h
#define vtkm_m_worklet_ThresholdPoints_h

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>

namespace vtkm
{
namespace worklet
{

class ThresholdPoints
{
public:
  template <typename UnaryPredicate>
  class ThresholdPointField : public vtkm::worklet::WorkletVisitPointsWithCells
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldInPoint scalars, FieldOutPoint passFlags);
    using ExecutionSignature = _3(_2);

    VTKM_CONT
    ThresholdPointField()
      : Predicate()
    {
    }

    VTKM_CONT
    explicit ThresholdPointField(const UnaryPredicate& predicate)
      : Predicate(predicate)
    {
    }

    template <typename ScalarType>
    VTKM_EXEC bool operator()(const ScalarType& scalar) const
    {
      return this->Predicate(scalar);
    }

  private:
    UnaryPredicate Predicate;
  };

  template <typename CellSetType, typename ScalarsArrayHandle, typename UnaryPredicate>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      const ScalarsArrayHandle& scalars,
                                      const UnaryPredicate& predicate)
  {
    vtkm::cont::ArrayHandle<bool> passFlags;

    using ThresholdWorklet = ThresholdPointField<UnaryPredicate>;

    ThresholdWorklet worklet(predicate);
    DispatcherMapTopology<ThresholdWorklet> dispatcher(worklet);
    dispatcher.Invoke(cellSet, scalars, passFlags);

    vtkm::cont::ArrayHandle<vtkm::Id> pointIds;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
    vtkm::cont::Algorithm::CopyIf(indices, passFlags, pointIds);

    // Make CellSetSingleType with VERTEX at each point id
    vtkm::cont::CellSetSingleType<> outCellSet;
    outCellSet.Fill(cellSet.GetNumberOfPoints(), vtkm::CellShapeTagVertex::Id, 1, pointIds);

    return outCellSet;
  }
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ThresholdPoints_h
