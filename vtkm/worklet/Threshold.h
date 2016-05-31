//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Field.h>

namespace vtkm {
namespace worklet {

class Threshold
{
public:
  struct BoolType : vtkm::ListTagBase<bool> { };

  template <typename UnaryPredicate>
  class ThresholdByPointField : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldInPoint<Scalar> scalars,
                                  FieldOutCell<BoolType> passFlags);

    typedef _3 ExecutionSignature(_2, PointCount);

    VTKM_CONT_EXPORT
    ThresholdByPointField() : Predicate() { }

    VTKM_CONT_EXPORT
    explicit ThresholdByPointField(const UnaryPredicate &predicate)
      : Predicate(predicate)
    { }

    template<typename ScalarsVecType>
    VTKM_EXEC_EXPORT
    bool operator()(const ScalarsVecType &scalars, vtkm::Id count) const
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
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldInTo<Scalar> scalars,
                                  FieldOut<BoolType> passFlags);

    typedef _3 ExecutionSignature(_2);

    VTKM_CONT_EXPORT
    ThresholdByCellField() : Predicate() { }

    VTKM_CONT_EXPORT
    explicit ThresholdByCellField(const UnaryPredicate &predicate)
      : Predicate(predicate)
    { }

    template<typename ScalarType>
    VTKM_EXEC_EXPORT
    bool operator()(const ScalarType &scalar) const
    {
      return this->Predicate(scalar);
    }

  private:
    UnaryPredicate Predicate;
  };

  template <typename CellSetType, typename UnaryPredicate, typename DeviceAdapter>
  vtkm::cont::CellSetPermutation< CellSetType >
  Run(const CellSetType &cellSet,
      const vtkm::cont::Field &field,
      const UnaryPredicate &predicate,
      DeviceAdapter)
  {
    typedef vtkm::cont::CellSetPermutation< CellSetType > OutputType;

    vtkm::cont::ArrayHandle<bool> passFlags;
    switch(field.GetAssociation())
    {
    case vtkm::cont::Field::ASSOC_POINTS:
      {
      typedef ThresholdByPointField<UnaryPredicate> ThresholdWorklet;

      ThresholdWorklet worklet(predicate);
      DispatcherMapTopology<ThresholdWorklet, DeviceAdapter> dispatcher(worklet);
      dispatcher.Invoke(cellSet, field.GetData(), passFlags);
      break;
      }

    case vtkm::cont::Field::ASSOC_CELL_SET:
      {
      typedef ThresholdByCellField<UnaryPredicate> ThresholdWorklet;

      ThresholdWorklet worklet(predicate);
      DispatcherMapTopology<ThresholdWorklet, DeviceAdapter> dispatcher(worklet);
      dispatcher.Invoke(cellSet, field.GetData(), passFlags);
      break;
      }

    default:
      throw vtkm::cont::ErrorControlBadValue("Expecting point or cell field.");
    }

    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>
        ::StreamCompact(passFlags, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet, cellSet.GetName());
  }

  class PermuteCellData
  {
  public:
    PermuteCellData(const vtkm::cont::ArrayHandle<vtkm::Id> validCellIds,
                    vtkm::cont::DynamicArrayHandle &data)
      : ValidCellIds(validCellIds), Data(&data)
    { }

    template <typename ArrayHandleType>
    void operator()(const ArrayHandleType &input) const
    {
      *(this->Data) = vtkm::cont::DynamicArrayHandle(
        vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, input));
    }

  private:
    vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
    vtkm::cont::DynamicArrayHandle *Data;
  };

  vtkm::cont::Field ProcessCellField(const vtkm::cont::Field field) const
  {
    if (field.GetAssociation() != vtkm::cont::Field::ASSOC_CELL_SET)
    {
      throw vtkm::cont::ErrorControlBadValue("Expecting cell field.");
    }

    vtkm::cont::DynamicArrayHandle data;
    field.GetData().CastAndCall(PermuteCellData(this->ValidCellIds, data));

    return vtkm::cont::Field(field.GetName(), field.GetAssociation(),
                             field.GetAssocCellSet(), data);
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};

}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Threshold_h
