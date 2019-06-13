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

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

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

  struct BoolType : vtkm::ListTagBase<bool>
  {
  };

  template <typename UnaryPredicate>
  class ThresholdByPointField : public vtkm::worklet::WorkletMapPointToCell
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

  template <typename UnaryPredicate>
  class ThresholdByCellField : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    using ControlSignature = void(CellSetIn cellset, FieldInTo scalars, FieldOut passFlags);

    using ExecutionSignature = _3(_2);

    VTKM_CONT
    ThresholdByCellField()
      : Predicate()
    {
    }

    VTKM_CONT
    explicit ThresholdByCellField(const UnaryPredicate& predicate)
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

  template <typename CellSetType, typename ValueType, typename StorageType, typename UnaryPredicate>
  vtkm::cont::CellSetPermutation<CellSetType> Run(
    const CellSetType& cellSet,
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& field,
    const vtkm::cont::Field::Association fieldType,
    const UnaryPredicate& predicate)
  {
    using OutputType = vtkm::cont::CellSetPermutation<CellSetType>;

    vtkm::cont::ArrayHandle<bool> passFlags;
    switch (fieldType)
    {
      case vtkm::cont::Field::Association::POINTS:
      {
        using ThresholdWorklet = ThresholdByPointField<UnaryPredicate>;

        ThresholdWorklet worklet(predicate);
        DispatcherMapTopology<ThresholdWorklet> dispatcher(worklet);
        dispatcher.Invoke(cellSet, field, passFlags);
        break;
      }
      case vtkm::cont::Field::Association::CELL_SET:
      {
        using ThresholdWorklet = ThresholdByCellField<UnaryPredicate>;

        ThresholdWorklet worklet(predicate);
        DispatcherMapTopology<ThresholdWorklet> dispatcher(worklet);
        dispatcher.Invoke(cellSet, field, passFlags);
        break;
      }

      default:
        throw vtkm::cont::ErrorBadValue("Expecting point or cell field.");
    }

    vtkm::cont::Algorithm::CopyIf(
      vtkm::cont::ArrayHandleIndex(passFlags.GetNumberOfValues()), passFlags, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet, cellSet.GetName());
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
      this->Output = this->Worklet.Run(cellSet, this->Field, this->FieldType, this->Predicate);
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
    const vtkm::cont::ArrayHandle<ValueType, StorageTag> in) const
  {
    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, in);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    vtkm::cont::ArrayCopy(tmp, result);

    return result;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Threshold_h
