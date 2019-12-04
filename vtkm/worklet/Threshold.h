//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtkm_m_worklet_Threshold_h
#define vtkm_m_worklet_Threshold_h

#include <vtkm/worklet/CellDeepCopy.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/Field.h>

namespace vtkm
{
namespace worklet
{

class Threshold
{
public:
  enum class FieldType
  {
    Point,
    Cell
  };

  template <typename UnaryPredicate>
  class ThresholdByPointField : public vtkm::worklet::WorkletVisitCellsWithPoints
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldInPoint scalars, FieldOutCell passFlags);

    using ExecutionSignature = _3(_2, PointCount);

    VTKM_CONT
    ThresholdByPointField()
      : Predicate()
    {
    }

    VTKM_CONT
    explicit ThresholdByPointField(const UnaryPredicate& predicate)
      : Predicate(predicate)
    {
    }

    template <typename ScalarsVecType>
    VTKM_EXEC bool operator()(const ScalarsVecType& scalars, vtkm::Id count) const
    {
      bool pass = false;
      for (vtkm::IdComponent i = 0; i < count; ++i)
      {
        pass |= this->Predicate(scalars[i]);
      }
      return pass;
    }

  private:
    UnaryPredicate Predicate;
  };

  struct ThresholdCopy : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn, FieldOut, WholeArrayIn);

    template <typename ScalarType, typename WholeFieldIn>
    VTKM_EXEC void operator()(vtkm::Id& index,
                              ScalarType& output,
                              const WholeFieldIn& inputField) const
    {
      output = inputField.Get(index);
    }
  };


  template <typename CellSetType, typename ValueType, typename StorageType, typename UnaryPredicate>
  vtkm::cont::CellSetPermutation<CellSetType> Run(
    const CellSetType& cellSet,
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& field,
    const vtkm::cont::Field::Association fieldType,
    const UnaryPredicate& predicate)
  {
    using OutputType = vtkm::cont::CellSetPermutation<CellSetType>;

    switch (fieldType)
    {
      case vtkm::cont::Field::Association::POINTS:
      {
        using ThresholdWorklet = ThresholdByPointField<UnaryPredicate>;
        vtkm::cont::ArrayHandle<bool> passFlags;

        ThresholdWorklet worklet(predicate);
        DispatcherMapTopology<ThresholdWorklet> dispatcher(worklet);
        dispatcher.Invoke(cellSet, field, passFlags);

        vtkm::cont::Algorithm::CopyIf(vtkm::cont::ArrayHandleIndex(passFlags.GetNumberOfValues()),
                                      passFlags,
                                      this->ValidCellIds);

        break;
      }
      case vtkm::cont::Field::Association::CELL_SET:
      {
        vtkm::cont::Algorithm::CopyIf(vtkm::cont::ArrayHandleIndex(field.GetNumberOfValues()),
                                      field,
                                      this->ValidCellIds,
                                      predicate);
        break;
      }

      default:
        throw vtkm::cont::ErrorBadValue("Expecting point or cell field.");
    }

    return OutputType(this->ValidCellIds, cellSet);
  }

  template <typename FieldArrayType, typename UnaryPredicate>
  struct CallWorklet
  {
    vtkm::cont::DynamicCellSet& Output;
    vtkm::worklet::Threshold& Worklet;
    const FieldArrayType& Field;
    const vtkm::cont::Field::Association FieldType;
    const UnaryPredicate& Predicate;

    CallWorklet(vtkm::cont::DynamicCellSet& output,
                vtkm::worklet::Threshold& worklet,
                const FieldArrayType& field,
                const vtkm::cont::Field::Association fieldType,
                const UnaryPredicate& predicate)
      : Output(output)
      , Worklet(worklet)
      , Field(field)
      , FieldType(fieldType)
      , Predicate(predicate)
    {
    }

    template <typename CellSetType>
    void operator()(const CellSetType& cellSet) const
    {
      // Copy output to an explicit grid so that other units can guess what this is.
      this->Output = vtkm::worklet::CellDeepCopy::Run(
        this->Worklet.Run(cellSet, this->Field, this->FieldType, this->Predicate));
    }
  };

  template <typename CellSetList, typename ValueType, typename StorageType, typename UnaryPredicate>
  vtkm::cont::DynamicCellSet Run(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
                                 const vtkm::cont::ArrayHandle<ValueType, StorageType>& field,
                                 const vtkm::cont::Field::Association fieldType,
                                 const UnaryPredicate& predicate)
  {
    using Worker = CallWorklet<vtkm::cont::ArrayHandle<ValueType, StorageType>, UnaryPredicate>;

    vtkm::cont::DynamicCellSet output;
    Worker worker(output, *this, field, fieldType, predicate);
    cellSet.CastAndCall(worker);

    return output;
  }

  template <typename ValueType, typename StorageTag>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageTag>& in) const
  {
    vtkm::cont::ArrayHandle<ValueType> result;
    DispatcherMapField<ThresholdCopy> dispatcher;
    dispatcher.Invoke(this->ValidCellIds, result, in);

    return result;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Threshold_h
