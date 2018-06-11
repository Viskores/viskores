//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_m_worklet_Threshold_h
#define vtkm_m_worklet_Threshold_h

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>
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
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<ScalarAll> scalars,
                                  FieldOutCell<BoolType> passFlags);

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
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInTo<ScalarAll> scalars,
                                  FieldOut<BoolType> passFlags);

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

  template <typename CellSetType,
            typename ValueType,
            typename StorageType,
            typename UnaryPredicate,
            typename DeviceAdapter>
  vtkm::cont::CellSetPermutation<CellSetType> Run(
    const CellSetType& cellSet,
    const vtkm::cont::ArrayHandle<ValueType, StorageType>& field,
    const vtkm::cont::Field::Association fieldType,
    const UnaryPredicate& predicate,
    DeviceAdapter)
  {
    using OutputType = vtkm::cont::CellSetPermutation<CellSetType>;

    vtkm::cont::ArrayHandle<bool> passFlags;
    switch (fieldType)
    {
      case vtkm::cont::Field::Association::POINTS:
      {
        using ThresholdWorklet = ThresholdByPointField<UnaryPredicate>;

        ThresholdWorklet worklet(predicate);
        DispatcherMapTopology<ThresholdWorklet, DeviceAdapter> dispatcher(worklet);
        dispatcher.Invoke(cellSet, field, passFlags);
        break;
      }
      case vtkm::cont::Field::Association::CELL_SET:
      {
        using ThresholdWorklet = ThresholdByCellField<UnaryPredicate>;

        ThresholdWorklet worklet(predicate);
        DispatcherMapTopology<ThresholdWorklet, DeviceAdapter> dispatcher(worklet);
        dispatcher.Invoke(cellSet, field, passFlags);
        break;
      }

      default:
        throw vtkm::cont::ErrorBadValue("Expecting point or cell field.");
    }

    vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::CopyIf(
      indices, passFlags, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet, cellSet.GetName());
  }

  template <typename CellSetList, typename FieldArrayType, typename UnaryPredicate, typename Device>
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
      this->Output =
        this->Worklet.Run(cellSet, this->Field, this->FieldType, this->Predicate, Device());
    }
  };

  template <typename CellSetList,
            typename ValueType,
            typename StorageType,
            typename UnaryPredicate,
            typename Device>
  vtkm::cont::DynamicCellSet Run(const vtkm::cont::DynamicCellSetBase<CellSetList>& cellSet,
                                 const vtkm::cont::ArrayHandle<ValueType, StorageType>& field,
                                 const vtkm::cont::Field::Association fieldType,
                                 const UnaryPredicate& predicate,
                                 Device)
  {
    using Worker = CallWorklet<CellSetList,
                               vtkm::cont::ArrayHandle<ValueType, StorageType>,
                               UnaryPredicate,
                               Device>;

    vtkm::cont::DynamicCellSet output;
    Worker worker(output, *this, field, fieldType, predicate);
    cellSet.CastAndCall(worker);

    return output;
  }

  template <typename ValueType, typename StorageTag, typename DeviceTag>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageTag> in,
    DeviceTag) const
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>;

    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, in);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    Algo::Copy(tmp, result);

    return result;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Threshold_h
